# Telemetria OBD2 com display via ESP-NOW

Projeto Integrador II — sistema de telemetria veicular para o **Ford New
Fiesta hatch 2014 1.6 flex** com dois módulos ESP32 comunicando por ESP-NOW.

```
   [Veículo]                                        [Painel]
  ┌─────────┐   CAN 500k    ┌──────────────┐  ESP-NOW  ┌──────────────────┐
  │   ECM   │◄─────────────►│   Módulo A   │──────────►│    Módulo B      │
  │  (0x7E8)│  ISO 15765-4  │  ESP32-C3 +  │  10 Hz    │ ESP32-S3 Sunton  │
  └─────────┘               │  SN65HVD230  │  33 bytes │ RGB 800x480+GT911│
                            └──────────────┘           └──────────────────┘
```

O motor usa estratégia **speed-density** (sem MAF — PIDs 0x10 e 0x5E
confirmados ausentes por varredura real). O consumo instantâneo é **calculado**
a partir de RPM, MAP, IAT e teor de etanol ([consumo.c](modulo-a-aquisicao/src/consumo.c))
— essa é a contribuição central do projeto.

## Estrutura

| Caminho | O que é |
|---|---|
| [comum/telemetria_protocolo.h](comum/telemetria_protocolo.h) | Contrato do pacote (33 bytes, só inteiros, CRC-16) — incluído pelos dois módulos |
| [modulo-a-aquisicao/](modulo-a-aquisicao/) | ESP32-C3: TWAI/OBD2, cálculo de consumo, ESP-NOW TX, deep sleep |
| [modulo-b-display/](modulo-b-display/) | ESP32-S3: painel RGB + LVGL 9 (telas do EEZ Studio), ESP-NOW RX/TX, alarme, NVS |
| [ferramentas/simulador-ecu-mcp2515/](ferramentas/simulador-ecu-mcp2515/) | Arduino Uno/Nano + MCP2515: simula o ECM na bancada |

Ambos os módulos: **ESP-IDF v5.x puro** via PlatformIO (`framework = espidf`,
fork pioarduino), código do app em `src/`.

## Abrindo no VS Code

Abra **`telemetria.code-workspace`** (*File → Open Workspace from File*), não
a pasta `prog/`. A extensão do PlatformIO só enxerga projeto com
`platformio.ini` na raiz de uma pasta do workspace. Abrindo `prog/` direto, o
IntelliSense não acha `freertos/`, `esp_log.h`, `lvgl.h`, etc. e acusa erro
em tudo. O primeiro build de cada módulo baixa o ESP-IDF 5.5 e demora.

## Bancada (simulação com Arduino + MCP2515)

```
 Arduino Uno + MCP2515 (8 MHz) ──CAN 250k── SN65HVD230 + ESP32-C3 ··ESP-NOW·· ESP32-S3 (display)
   simula o ECM (0x7E8)                         Módulo A                          Módulo B
```

1. Grave o simulador: `ferramentas/simulador-ecu-mcp2515` (env `uno` ou `nano`).
2. Grave o Módulo A com o env **`modulo_a_bancada`** (é o padrão).
3. Grave o Módulo B.
4. Terminação: jumper J1 (120 Ω) do MCP2515 fechado + um 120 Ω entre CAN-H e
   CAN-L do lado do SN65HVD230 (a placa dele está sem o resistor).
5. Na tela **Config** do display, escolha a taxa do CAN do Módulo A (250k
   para o simulador). O Módulo A grava na NVS e reinicia. O padrão de fábrica é 500k.

Pelo monitor serial do simulador dá para forçar superaquecimento (`q`),
calar o ECM (`e`) ou silenciar o barramento (`s`), testando o alarme, o timeout
por PID e o deep sleep.

### Por que existe um firmware de bancada

Com só 2 nós, enquanto o Módulo A está em listen-only ninguém dá ACK. O
MCP2515 sinaliza erro de ACK e **nenhum quadro chega válido**. O firmware do
carro exige quadro válido para sair do listen-only; se vir só erros, testa a
outra taxa, ainda em escuta, e nunca transmite numa taxa não provada. O de
bancada aceita erro elétrico como prova de tráfego, e por isso **nunca deve ir
para o carro**: grave `modulo_a_carro` antes de plugar no Fiesta.

## Pendências (TODO)

1. **MAC do Módulo B/A**: os dois lados usam broadcast. Para unicast com ACK,
   colar o MAC que cada módulo imprime no boot em `enlace_espnow.c`.
2. **Capacidade do tanque**: 48,0 L assumido. Confirmar no manual
   (`TANQUE_CAPACIDADE_DL` no Módulo A e `PADRAO_TANQUE_DL` no B).
3. **UI no EEZ Studio**: seguir [modulo-b-display/eez/LEIA-ME.md](modulo-b-display/eez/LEIA-ME.md).

## Build e gravação

```sh
cd modulo-a-aquisicao
pio run -e modulo_a_bancada -t upload   # bancada (simulador)
pio run -e modulo_a_carro -t upload     # carro
pio device monitor

cd ../modulo-b-display
pio run -t upload && pio device monitor
```

## Calibração do VE (o ajuste principal do projeto)

O consumo depende da **eficiência volumétrica** (VE), gravada na NVS com
valor inicial 0,85. Um escalar único é aproximação de ordem zero e **precisa
de calibração empírica**: encher o tanque, rodar normalmente, encher de novo,
comparar os litros reais com o consumo integrado pelo firmware e corrigir o
VE proporcionalmente (`consumo_definir_ve()`). Repetir até convergir.
Evolução prevista: tabela VE(rpm, MAP).

## Decisões de arquitetura (resumo para a banca)

- **Boot em escuta**: o Módulo A entra no barramento em `TWAI_MODE_LISTEN_ONLY`
  (eletricamente invisível) e só transmite após confirmar tráfego por 3 s.
- **Cadência espaçada**: 1 requisição OBD2 por vez a cada 100 ms, timeout de
  200 ms por PID; nunca rajada. PIDs rápidos/médios/lentos em ciclos distintos.
- **Energia**: CAN em silêncio por 30 s → deep sleep, wakeup por timer a 60 s.
  O piso de consumo é o quiescente do LM2596 (~5 mA), não eliminável por software.
- **Anti-tearing no display**: `esp_lcd_rgb_panel` com `num_fbs = 2` (dois
  framebuffers em PSRAM, troca no vsync) + bounce buffer em SRAM interna.
- **Concorrência no B**: LVGL fixado no core 1; o callback do ESP-NOW (core 0)
  só valida CRC e copia 33 bytes sob spinlock — jamais chama LVGL. A UI (gerada
  pelo EEZ) recebe os dados pela ponte `ui_ponte.c`, num `lv_timer` de 10 Hz; o
  código gerado só redesenha o widget cujo texto mudou.
- **Canal de retorno B → A**: a tela de configuração envia `telem_comando_t`
  (taxa do CAN); o A grava na NVS e reinicia, refazendo a escuta obrigatória.
- **Alarme**: média móvel + histerese isolados em [alarme.c](modulo-b-display/src/alarme.c);
  a UI só pinta o estado.

## Fase 2 (declarada, não implementada)

- Modo 03 (DTCs): exige ISO-TP multi-frame com flow control — stub em
  `can_obd2_ler_dtcs()`; a tela de DTC já existe com lista vazia.
- Sincronização do VE B → A: o canal de retorno já existe (`telem_comando_t`),
  falta o comando `TELEM_CMD_DEFINIR_VE`.
- Detecção de corte de injeção (consumo zero em freio-motor).
- Wakeup por atividade no RX do CAN em vez de timer de 60 s.
- Autonomia estimada (litros × km/L médio) usando a capacidade do tanque na NVS.
