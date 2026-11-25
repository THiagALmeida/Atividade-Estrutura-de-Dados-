"""Gráficos para o videocast a partir dos CSV gerados por bin/rede.exe.

Uso (na pasta do TDE): uv run --with matplotlib python tools/graficos.py
Gera resultados/benchmark.png e resultados/distribuicao_graus.png.
"""
import csv
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

R = Path(__file__).resolve().parent.parent / "resultados"

linhas = [l for l in csv.DictReader(open(R / "benchmark.csv", encoding="utf-8")) if l["operacao"] != "Memoria (MB)"]
nomes = [l["operacao"].replace(" (", "\n(") for l in linhas]
mat = [float(l["matriz_ms_media"]) for l in linhas]
lis = [float(l["lista_ms_media"]) for l in linhas]
emat = [float(l["matriz_ms_dp"]) for l in linhas]
elis = [float(l["lista_ms_dp"]) for l in linhas]

fig, ax = plt.subplots(figsize=(11, 5.5))
y = range(len(nomes))
ax.barh([i + 0.2 for i in y], mat, 0.4, xerr=emat, label="Matriz de adjacência", color="#d9485f")
ax.barh([i - 0.2 for i in y], lis, 0.4, xerr=elis, label="Lista de adjacência", color="#3b82f6")
for i in y:
    ax.text(mat[i] + emat[i] + 1, i + 0.2, f"{mat[i]:.1f} ms", va="center", fontsize=9)
    ax.text(lis[i] + elis[i] + 1, i - 0.2, f"{lis[i]:.1f} ms", va="center", fontsize=9)
ax.set_yticks(list(y), nomes, fontsize=9)
ax.invert_yaxis()
ax.set_xlabel("tempo médio de 10 execuções (ms) — menor é melhor")
ax.set_title("Rede social com 10.000 usuários e 19.996 amizades: matriz × lista")
ax.legend(loc="lower right")
ax.spines[["top", "right"]].set_visible(False)
fig.tight_layout()
fig.savefig(R / "benchmark.png", dpi=150)

graus = list(csv.DictReader(open(R / "distribuicao_graus.csv", encoding="utf-8")))
fig, ax = plt.subplots(figsize=(8, 4.5))
ax.bar([int(g["grau"]) for g in graus], [int(g["usuarios"]) for g in graus], color="#8b5cf6")
ax.set_xlabel("grau (número de amigos)")
ax.set_ylabel("usuários")
ax.set_title("Distribuição de graus: ninguém concentra a rede (máximo 14 amigos)")
ax.spines[["top", "right"]].set_visible(False)
fig.tight_layout()
fig.savefig(R / "distribuicao_graus.png", dpi=150)
print("ok:", R / "benchmark.png", R / "distribuicao_graus.png")
