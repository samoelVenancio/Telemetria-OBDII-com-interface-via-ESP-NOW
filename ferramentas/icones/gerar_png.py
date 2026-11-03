#!/usr/bin/env python3
"""Gera os PNGs dos ícones (branco sobre transparente) a partir de icones.json.

Uso:  python gerar_png.py            -> png48/ e png32/ ao lado deste script
Requer rsvg-convert (librsvg). No Windows os PNGs já vêm prontos no pacote;
este script só é necessário se você editar um ícone.
"""
import json
import pathlib
import subprocess
import sys

AQUI = pathlib.Path(__file__).resolve().parent
TAMANHOS = (48, 32)


def svg(inner: str, tamanho: int) -> str:
    corpo = inner.replace("currentColor", "#FFFFFF")
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{tamanho}" height="{tamanho}" '
            f'viewBox="0 0 24 24" fill="none" stroke="#FFFFFF" stroke-width="2" '
            f'stroke-linecap="round" stroke-linejoin="round">{corpo}</svg>')


def main() -> int:
    icones = json.loads((AQUI / "icones.json").read_text(encoding="utf-8"))
    (AQUI / "svg").mkdir(exist_ok=True)
    for t in TAMANHOS:
        (AQUI / f"png{t}").mkdir(exist_ok=True)
    for ic in icones:
        (AQUI / "svg" / f"{ic['n']}.svg").write_text(svg(ic["i"], 24) + "\n", encoding="utf-8")
        for t in TAMANHOS:
            destino = AQUI / f"png{t}" / f"{ic['n']}.png"
            subprocess.run(["rsvg-convert", "-w", str(t), "-h", str(t), "-o", str(destino)],
                           input=svg(ic["i"], t).encode(), check=True)
    print(f"{len(icones)} ícones em {', '.join(f'png{t}' for t in TAMANHOS)} e svg/")
    return 0


if __name__ == "__main__":
    sys.exit(main())
