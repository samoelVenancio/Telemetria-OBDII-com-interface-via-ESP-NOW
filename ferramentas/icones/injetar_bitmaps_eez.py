#!/usr/bin/env python3
"""Injeta os ícones como bitmaps no projeto EEZ Studio do Módulo B.

    python ferramentas/icones/injetar_bitmaps_eez.py modulo-b-display/eez/projeto_display_carro.eez-project

FECHE O EEZ STUDIO ANTES. Se o projeto estiver aberto, o EEZ sobrescreve o
arquivo ao salvar e os bitmaps somem.

O que faz:
  - cria uma cópia de segurança  <projeto>.antes-icones.bak  (só na 1ª vez);
  - adiciona (ou atualiza, se já existir) cada bitmap da lista ABAIXO, no
    mesmo formato do btn_back que já funciona: A8 (só alfa, cor definida
    em tempo de execução pelo recolor);
  - marca "Always build", para o EEZ gerar img_ic_* mesmo que o bitmap não
    esteja colocado em nenhuma tela. Isso é obrigatório para os ícones que o
    firmware troca em tempo de execução (src/ui_icones.c), senão dá
    "undefined reference to img_ic_..." na compilação.

Nome no EEZ -> símbolo gerado:  ic_rpm -> img_ic_rpm,  ic_sinal_3_32 -> img_ic_sinal_3_32
"""
import base64
import json
import pathlib
import shutil
import sys
import uuid

AQUI = pathlib.Path(__file__).resolve().parent

# (ícone, tamanho). O sufixo _32 só entra no nome dos de 32 px.
BITMAPS = [
    # 48 px — ícone de cada campo do painel (trocado conforme o layout)
    *[(n, 48) for n in ("rpm", "velocidade", "consumo", "etanol", "temp_motor",
                        "temp_ar", "map", "tanque", "bateria")],
    # 48 px — menu principal e tela de alarmes (fixos, colocados no EEZ)
    *[(n, 48) for n in ("painel", "layout", "configuracoes", "alarmes",
                        "superaquecimento", "motor_frio", "eco")],
    # 32 px — linha de status e indicador de sinal (trocados em tempo de execução)
    *[(n, 32) for n in ("can_ok", "can_erro", "sem_sinal", "superaquecimento",
                        "motor_frio", "trocar_marcha", "alerta",
                        "sinal_0", "sinal_1", "sinal_2", "sinal_3", "tentar_denovo")],
]

# Mesmos campos do btn_back gerado pelo EEZ 9.5 deste projeto (saída A8)
MODELO = {"bpp": 4, "alwaysBuild": True, "lvglBinaryOutputFormat": 3, "lvglDither": False}


def nome_eez(icone: str, tamanho: int) -> str:
    return f"ic_{icone}" if tamanho == 48 else f"ic_{icone}_{tamanho}"


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    projeto = pathlib.Path(sys.argv[1])
    dados = json.loads(projeto.read_text(encoding="utf-8"))

    modelo = dict(MODELO)
    for b in dados.get("bitmaps", []):
        if b.get("name") == "btn_back":  # herda o formato que já está validado na placa
            modelo.update({k: b[k] for k in ("bpp", "lvglBinaryOutputFormat", "lvglDither") if k in b})
            modelo["alwaysBuild"] = True

    backup = projeto.with_suffix(projeto.suffix + ".antes-icones.bak")
    if not backup.exists():
        shutil.copy2(projeto, backup)
        print(f"backup: {backup.name}")

    existentes = {b["name"]: b for b in dados.setdefault("bitmaps", [])}
    novos = atualizados = 0
    for icone, tam in BITMAPS:
        png = AQUI / f"png{tam}" / f"{icone}.png"
        uri = "data:image/png;base64," + base64.b64encode(png.read_bytes()).decode()
        nome = nome_eez(icone, tam)
        if nome in existentes:
            existentes[nome].update({"image": uri, **modelo})
            atualizados += 1
        else:
            dados["bitmaps"].append({"objID": str(uuid.uuid4()), "name": nome, "image": uri, **modelo})
            novos += 1

    texto = json.dumps(dados, indent=2, ensure_ascii=False)
    json.loads(texto)  # sanidade antes de gravar
    projeto.write_text(texto, encoding="utf-8")
    print(f"{novos} bitmaps novos, {atualizados} atualizados -> {projeto.name}")
    print("Abra o projeto no EEZ, confira a aba Bitmaps e dê Build (F7).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
