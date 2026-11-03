#!/usr/bin/env python3
"""Gera src/ui_icones_k.c/.h: ícones A8 (só alfa) em 32 e 64 px para as telas
feitas em código, sem depender do EEZ.  Uso: python gerar_icones_c.py <icones.json> <saida_dir>
Requer rsvg-convert e Pillow."""
import io, json, subprocess, sys, pathlib
from PIL import Image

TAM_32 = None  # todos os ícones em 32 px
TAM_64 = ["painel", "rpm", "layout", "alarmes", "configuracoes", "sinal_0", "sinal_1",
          "sinal_2", "sinal_3", "sem_sinal"]

def svg(inner, t):
    corpo = inner.replace("currentColor", "#FFFFFF")
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{t}" height="{t}" viewBox="0 0 24 24" '
            f'fill="none" stroke="#FFFFFF" stroke-width="2" stroke-linecap="round" '
            f'stroke-linejoin="round">{corpo}</svg>')

def alfa(inner, t):
    png = subprocess.run(["rsvg-convert", "-w", str(t), "-h", str(t)], input=svg(inner, t).encode(),
                         capture_output=True, check=True).stdout
    return Image.open(io.BytesIO(png)).convert("RGBA").split()[3].tobytes()

def main():
    icones = {i["n"]: i["i"] for i in json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))}
    saida = pathlib.Path(sys.argv[2])
    pares = [(n, 32) for n in icones] + [(n, 64) for n in TAM_64]
    c = ['/* GERADO por ferramentas/gerar_icones_c.py — não editar à mão */\n#include "lvgl.h"\n#include "ui_icones_k.h"\n\n']
    h = ['/* GERADO por ferramentas/gerar_icones_c.py — não editar à mão */\n#pragma once\n#include "lvgl.h"\n\n'
         '/* Ícones A8 (só alfa): a cor vem de image_recolor no estilo do objeto */\n']
    for n, t in pares:
        sym = f"img_k{t}_{n}"
        dados = alfa(icones[n], t)
        linhas = ",\n".join(", ".join(f"0x{b:02x}" for b in dados[i:i + 24]) for i in range(0, len(dados), 24))
        c.append(f"static const uint8_t {sym}_map[] = {{\n{linhas}\n}};\n"
                 f"const lv_image_dsc_t {sym} = {{\n"
                 f"    .header = {{ .magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_A8, .w = {t}, .h = {t}, .stride = {t} }},\n"
                 f"    .data_size = sizeof({sym}_map),\n    .data = {sym}_map,\n}};\n\n")
        h.append(f"extern const lv_image_dsc_t {sym};\n")
    (saida / "ui_icones_k.c").write_text("".join(c), encoding="utf-8")
    (saida / "ui_icones_k.h").write_text("".join(h), encoding="utf-8")
    print(f"{len(pares)} ícones -> {saida}")

if __name__ == "__main__":
    main()
