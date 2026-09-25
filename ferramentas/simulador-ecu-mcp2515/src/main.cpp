/**
 * main.cpp — Simulador de ECM (bancada) para o Módulo A.
 *
 * Faz o papel do ECM do New Fiesta 1.6 no barramento:
 *  - responde requisições OBD2 Modo 01 em 0x7E0 (física) e 0x7DF (funcional)
 *    com resposta single frame em 0x7E8, para os 8 PIDs do projeto;
 *  - emite um quadro de "fundo" periódico (ID 0x420), imitando o tráfego que
 *    o carro tem mesmo sem ninguém perguntar nada. Sem isso, a escuta
 *    obrigatória do boot do Módulo A veria silêncio e ele iria dormir;
 *  - gera um ciclo de condução sintético de 60 s (marcha lenta, aceleração
 *    com trocas de marcha, cruzeiro, desaceleração com corte, parada) e um
 *    aquecimento do motor de 25 °C até a temperatura de trabalho.
 *
 * ATENÇÃO — bancada com 2 nós: enquanto o Módulo A está em listen-only
 * (primeiros 3 s do boot dele), NINGUÉM dá ACK nos quadros do MCP2515. Ele
 * fica retransmitindo e acusando erro de ACK — é esperado. Quando o A passa
 * para o modo normal, os ACKs aparecem e o tráfego normaliza. É por isso que
 * o Módulo A tem um firmware de bancada (env modulo_a_bancada).
 *
 * Ligações (Uno/Nano): MCP2515 CS->D10, SO->D12, SI->D11, SCK->D13,
 * VCC->5V, GND->GND. INT não é usado (leitura por polling).
 * CAN_H/CAN_L no SN65HVD230 do Módulo A. Deixe o jumper J1 (120 R) do
 * módulo MCP2515 fechado; como a placa do SN65 está sem terminação, ponha
 * outro 120 R entre CAN_H e CAN_L do lado do Módulo A.
 *
 * Comandos pelo monitor serial (115200):
 *   2 -> 250 kbit/s     5 -> 500 kbit/s
 *   q -> liga/desliga superaquecimento (108 °C) — testa o alarme do display
 *   e -> ECM mudo (não responde PIDs, mas mantém o fundo) — testa timeout
 *   s -> barramento mudo (nada é transmitido) — testa o deep sleep do A
 *   h -> ajuda
 */
#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h>

// ---------------------------------------------------------------------------
// Configuração
// ---------------------------------------------------------------------------
#define PINO_CS_MCP2515      10
#define CRISTAL_MCP2515      MCP_8MHZ   // cristal "8.000" no módulo

#define ID_REQ_FISICA        0x7E0
#define ID_REQ_FUNCIONAL     0x7DF
#define ID_RESPOSTA_ECM      0x7E8
#define ID_FUNDO             0x420      // tráfego de fundo (arbitrário)

#define PERIODO_FUNDO_MS     50UL
#define PERIODO_STATUS_MS    2000UL
#define CICLO_CONDUCAO_MS    60000UL
#define AQUECIMENTO_S        180UL      // 25 -> 90 °C em 3 min
#define PADDING_ISOTP        0x55
#define TENTATIVAS_TX        5

// Grandezas fixas do cenário
#define IAT_C                32
#define ETANOL_PCT           27         // gasolina comum brasileira (E27)
#define NIVEL_INICIAL_PCT    62
#define TEMP_SUPERAQUECIDO_C 108

MCP2515 mcp(PINO_CS_MCP2515);

static uint16_t g_kbps = 250;
static bool g_superaquecer = false;
static bool g_ecm_mudo = false;
static bool g_barramento_mudo = false;

static uint32_t g_respostas = 0;
static uint32_t g_requisicoes = 0;

struct Veiculo {
  uint16_t rpm;
  uint8_t vel_kmh;
  uint8_t map_kpa;
  int16_t temp_c;
  uint16_t tensao_mv;
  uint8_t nivel_pct;
};
static Veiculo g_v;

// ---------------------------------------------------------------------------
// Ciclo de condução sintético
// ---------------------------------------------------------------------------
static uint16_t rpm_por_marcha(uint8_t vel) {
  // Trocas a 20/40/60 km/h: dentro de cada faixa a rotação sobe e cai na troca
  if (vel < 20) return 900 + vel * 140;           // 1a: 900..3700
  uint8_t base = (vel / 20) * 20;
  return 1800 + (vel - base) * 100;               // 2a..4a: 1800..3800
}

static void atualizar_veiculo(void) {
  uint32_t agora = millis();
  uint32_t t = agora % CICLO_CONDUCAO_MS;
  int16_t ruido = (int16_t)random(-15, 16);

  if (t < 8000) {                                  // marcha lenta
    g_v.vel_kmh = 0;
    g_v.rpm = 850 + ruido;
    g_v.map_kpa = 33;
  } else if (t < 20000) {                          // aceleração
    g_v.vel_kmh = (uint8_t)((t - 8000) * 80UL / 12000UL);
    g_v.rpm = rpm_por_marcha(g_v.vel_kmh) + ruido;
    g_v.map_kpa = 88;
  } else if (t < 40000) {                          // cruzeiro a 80 km/h (5a)
    g_v.vel_kmh = 80;
    g_v.rpm = 2500 + ruido * 2;
    g_v.map_kpa = 45;
  } else if (t < 50000) {                          // desaceleração engatada (corte)
    g_v.vel_kmh = (uint8_t)(80 - (t - 40000) * 60UL / 10000UL);
    g_v.rpm = 2500 - (80 - g_v.vel_kmh) * 20;
    g_v.map_kpa = 22;
  } else {                                         // parando
    g_v.vel_kmh = (uint8_t)(20 - (t - 50000) * 20UL / 10000UL);
    g_v.rpm = 1100 - (20 - g_v.vel_kmh) * 12 + ruido;
    g_v.map_kpa = 30;
  }

  // Aquecimento: rampa até 90 °C, depois oscila 88..94 (ventoinha ciclando)
  uint32_t segundos = agora / 1000UL;
  if (g_superaquecer) {
    g_v.temp_c = TEMP_SUPERAQUECIDO_C;
  } else if (segundos < AQUECIMENTO_S) {
    g_v.temp_c = 25 + (int16_t)(segundos * 65UL / AQUECIMENTO_S);
  } else {
    g_v.temp_c = 88 + (int16_t)((agora / 3000UL) % 7);
  }

  g_v.tensao_mv = 14100 + ruido * 4;              // alternador carregando

  // Tanque: cai 1 % a cada 2 min de simulação
  uint8_t gasto = (uint8_t)(segundos / 120UL);
  g_v.nivel_pct = (gasto < NIVEL_INICIAL_PCT) ? NIVEL_INICIAL_PCT - gasto : 0;
}

// ---------------------------------------------------------------------------
// OBD2 Modo 01
// ---------------------------------------------------------------------------
static const uint8_t PIDS_SUPORTADOS[] = {
  0x05, 0x0B, 0x0C, 0x0D, 0x0F, 0x20, 0x2F, 0x40, 0x42, 0x52
};

// Bitmap J1979 dos PIDs suportados na faixa [base+1, base+0x20]
static uint32_t bitmap_suporte(uint8_t base) {
  uint32_t mapa = 0;
  for (uint8_t i = 0; i < sizeof(PIDS_SUPORTADOS); i++) {
    uint8_t p = PIDS_SUPORTADOS[i];
    if (p > base && p <= base + 0x20) {
      mapa |= 1UL << (0x20 - (p - base));
    }
  }
  return mapa;
}

// Preenche os bytes A, B, ... do PID. Retorna quantos (0 = não suportado).
static uint8_t codificar_pid(uint8_t pid, uint8_t *d) {
  switch (pid) {
    case 0x00: case 0x20: case 0x40: {
      uint32_t m = bitmap_suporte(pid);
      d[0] = m >> 24; d[1] = m >> 16; d[2] = m >> 8; d[3] = m;
      return 4;
    }
    case 0x05: d[0] = (uint8_t)(g_v.temp_c + 40); return 1;           // A-40
    case 0x0B: d[0] = g_v.map_kpa; return 1;                          // A
    case 0x0C: {                                                      // (256A+B)/4
      uint16_t bruto = g_v.rpm * 4;
      d[0] = bruto >> 8; d[1] = bruto & 0xFF;
      return 2;
    }
    case 0x0D: d[0] = g_v.vel_kmh; return 1;                          // A
    case 0x0F: d[0] = IAT_C + 40; return 1;                           // A-40
    case 0x2F: d[0] = (uint8_t)(g_v.nivel_pct * 255UL / 100UL); return 1;
    case 0x42: d[0] = g_v.tensao_mv >> 8; d[1] = g_v.tensao_mv & 0xFF; return 2;
    case 0x52: d[0] = (uint8_t)(ETANOL_PCT * 255UL / 100UL); return 1;
    default: return 0;
  }
}

static bool transmitir(struct can_frame *f) {
  for (uint8_t i = 0; i < TENTATIVAS_TX; i++) {
    if (mcp.sendMessage(f) == MCP2515::ERROR_OK) return true;
    delay(1);
  }
  return false;
}

static void tratar_requisicao(const struct can_frame *req) {
  // ISO-TP single frame: [len, modo, pid, ...]
  if ((req->data[0] & 0xF0) != 0x00 || req->data[0] < 2) return;
  if (req->data[1] != 0x01) return;               // só Modo 01
  g_requisicoes++;
  if (g_ecm_mudo || g_barramento_mudo) return;

  uint8_t pid = req->data[2];
  struct can_frame resp;
  resp.can_id = ID_RESPOSTA_ECM;
  resp.can_dlc = 8;
  memset(resp.data, PADDING_ISOTP, 8);
  uint8_t n = codificar_pid(pid, &resp.data[3]);
  if (n == 0) return;                             // PID não suportado: ECM se cala
  resp.data[0] = 2 + n;
  resp.data[1] = 0x41;
  resp.data[2] = pid;
  if (transmitir(&resp)) g_respostas++;
}

// ---------------------------------------------------------------------------
// MCP2515
// ---------------------------------------------------------------------------
static void configurar_mcp2515(uint16_t kbps) {
  mcp.reset();
  mcp.setBitrate(kbps == 500 ? CAN_500KBPS : CAN_250KBPS, CRISTAL_MCP2515);
  // Aceita só requisições OBD2 (física e funcional) nos dois buffers de RX
  mcp.setFilterMask(MCP2515::MASK0, false, 0x7FF);
  mcp.setFilter(MCP2515::RXF0, false, ID_REQ_FISICA);
  mcp.setFilter(MCP2515::RXF1, false, ID_REQ_FUNCIONAL);
  mcp.setFilterMask(MCP2515::MASK1, false, 0x7FF);
  mcp.setFilter(MCP2515::RXF2, false, ID_REQ_FISICA);
  mcp.setFilter(MCP2515::RXF3, false, ID_REQ_FUNCIONAL);
  mcp.setFilter(MCP2515::RXF4, false, ID_REQ_FISICA);
  mcp.setFilter(MCP2515::RXF5, false, ID_REQ_FUNCIONAL);
  mcp.setNormalMode();
  g_kbps = kbps;
  Serial.print(F("[can] MCP2515 a "));
  Serial.print(kbps);
  Serial.println(F(" kbit/s"));
}

static void ajuda(void) {
  Serial.println(F("comandos: 2=250k 5=500k q=superaquecer e=ECM mudo s=barramento mudo h=ajuda"));
}

static void tratar_serial(void) {
  if (!Serial.available()) return;
  char c = (char)Serial.read();
  switch (c) {
    case '2': configurar_mcp2515(250); break;
    case '5': configurar_mcp2515(500); break;
    case 'q': g_superaquecer = !g_superaquecer;
              Serial.println(g_superaquecer ? F("[sim] superaquecimento ON") : F("[sim] superaquecimento OFF"));
              break;
    case 'e': g_ecm_mudo = !g_ecm_mudo;
              Serial.println(g_ecm_mudo ? F("[sim] ECM mudo") : F("[sim] ECM respondendo"));
              break;
    case 's': g_barramento_mudo = !g_barramento_mudo;
              Serial.println(g_barramento_mudo ? F("[sim] barramento MUDO") : F("[sim] barramento ativo"));
              break;
    case 'h': ajuda(); break;
    default: break;
  }
}

static void imprimir_status(void) {
  uint8_t tec = mcp.errorCountTX();
  Serial.print(F("[sim] "));
  Serial.print(g_kbps);
  Serial.print(F("k rpm="));
  Serial.print(g_v.rpm);
  Serial.print(F(" vel="));
  Serial.print(g_v.vel_kmh);
  Serial.print(F(" map="));
  Serial.print(g_v.map_kpa);
  Serial.print(F(" temp="));
  Serial.print(g_v.temp_c);
  Serial.print(F(" req="));
  Serial.print(g_requisicoes);
  Serial.print(F(" resp="));
  Serial.print(g_respostas);
  Serial.print(F(" TEC="));
  Serial.print(tec);
  // TEC alto (>=128) com o A em listen-only é normal: ninguém dá ACK
  if (tec >= 128) Serial.print(F(" (sem ACK: Modulo A em escuta ou desligado?)"));
  Serial.println();
}

// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println(F("\n[sim] simulador de ECM - New Fiesta 1.6 (bancada)"));
  randomSeed(analogRead(A0));
  SPI.begin();
  configurar_mcp2515(g_kbps);
  ajuda();
}

void loop() {
  static uint32_t ultimo_fundo = 0;
  static uint32_t ultimo_status = 0;
  static uint8_t contador_fundo = 0;

  tratar_serial();
  atualizar_veiculo();

  struct can_frame f;
  while (mcp.readMessage(&f) == MCP2515::ERROR_OK) {
    if (f.can_id == ID_REQ_FISICA || f.can_id == ID_REQ_FUNCIONAL) {
      tratar_requisicao(&f);
    }
  }

  uint32_t agora = millis();
  if (!g_barramento_mudo && agora - ultimo_fundo >= PERIODO_FUNDO_MS) {
    ultimo_fundo = agora;
    struct can_frame fundo;
    fundo.can_id = ID_FUNDO;
    fundo.can_dlc = 8;
    memset(fundo.data, 0, 8);
    fundo.data[0] = contador_fundo++;
    fundo.data[1] = g_v.rpm >> 8;
    fundo.data[2] = g_v.rpm & 0xFF;
    mcp.sendMessage(&fundo);  // pode falhar sem ACK (A em escuta) — esperado
  }

  if (agora - ultimo_status >= PERIODO_STATUS_MS) {
    ultimo_status = agora;
    imprimir_status();
  }
}
