"""Confere, de forma INDEPENDENTE do programa em C, os números e as previsões teóricas
citados em COMO_CHEGAMOS.md. Só usa a biblioteca padrão do Python.

Uso (na pasta do TDE):  python tools/verificar_teoria.py
"""
import collections
import csv
import math
import random
import sys
from pathlib import Path

sys.stdout.reconfigure(encoding="utf-8")  # console do Windows: acentos e "Erdős"

PASTA = Path(__file__).resolve().parents[3] / "atividade de grafos"

usuarios = list(csv.DictReader(open(PASTA / "usuários rede social.txt", encoding="utf-8")))
topicos = {int(u["UsuarioID"]) - 1: frozenset(t.strip() for t in u["TopicosFavoritos"].split(",")) for u in usuarios}
postes = {int(u["UsuarioID"]) - 1: int(u["Postes"]) for u in usuarios}

adj = collections.defaultdict(set)
linhas = 0
for a, b in csv.reader(open(PASTA / "amizades rede social.txt", encoding="utf-8")):
    if not a.isdigit():
        continue
    linhas += 1
    a, b = int(a) - 1, int(b) - 1
    adj[a].add(b)
    adj[b].add(a)

n = len(usuarios)
m = sum(len(s) for s in adj.values()) // 2
grau = [len(adj[v]) for v in range(n)]

print("== 1. Teoria dos grafos ==")
print(f"n = {n}, linhas de amizade = {linhas}, amizades únicas m = {m} (repetidas: {linhas - m})")
print(f"soma dos graus = {sum(grau)} = 2m = {2 * m}  (lema do aperto de mãos)")
print(f"grau médio = 2m/n = {2 * m / n:.4f}")
p = 2 * m / (n * (n - 1))
print(f"densidade p = 2m/(n(n-1)) = {p:.7f} = {100 * p:.4f}%")

visto, comps = [False] * n, []
for s in range(n):
    if visto[s]:
        continue
    pilha, c = [s], 0
    visto[s] = True
    while pilha:
        u = pilha.pop()
        c += 1
        for v in adj[u]:
            if not visto[v]:
                visto[v] = True
                pilha.append(v)
    comps.append(c)
print(f"componentes = {len(comps)}, maior = {max(comps)} ({100 * max(comps) / n:.1f}%), isolados = {grau.count(0)}")

print("\n== 2. Complexidade (contas antes de medir) ==")
print(f"memória matriz = n² bytes = {n * n / 2**20:.1f} MB")
print(f"memória lista  = n·(8+4) + 2m·16 bytes = {(n * 12 + 2 * m * 16) / 2**20:.2f} MB")
print(f"BFS: matriz olha n² = {n * n:.0e} células; lista olha n + 2m = {n + 2 * m}; razão teórica = {n * n / (n + 2 * m):.0f}x")

print("\n== 4. PageRank (método da potência, d = 0,85) ==")
d, pr = 0.85, [1 / n] * n
for it in range(1, 201):
    pend = sum(pr[v] for v in range(n) if grau[v] == 0)
    novo = [(1 - d) / n + d * pend / n] * n
    for u in range(n):
        if grau[u]:
            parte = d * pr[u] / grau[u]
            for v in adj[u]:
                novo[v] += parte
    dif = sum(abs(x - y) for x, y in zip(novo, pr))
    pr = novo
    if dif < 1e-10:
        break
top = sorted(range(n), key=lambda v: (-grau[v], -pr[v]))[:3]
print(f"convergiu em {it} iterações; soma = {sum(pr):.12f}")
print("top 3:", [(usuarios[v]["NomeUsuario"], grau[v], round(pr[v] * n, 3)) for v in top])
r = sorted(range(n), key=lambda v: -pr[v])
print("correlação grau × PageRank:", round(
    (lambda x, y: sum((a - sum(x) / n) * (b - sum(y) / n) for a, b in zip(x, y)) /
     math.sqrt(sum((a - sum(x) / n) ** 2 for a in x) * sum((b - sum(y) / n) ** 2 for b in y)))(grau, pr), 4))

print("\n== 5. Pearson grau × postes ==")
x, y = grau, [postes[v] for v in range(n)]
mx, my = sum(x) / n, sum(y) / n
rxy = sum((a - mx) * (b - my) for a, b in zip(x, y)) / math.sqrt(sum((a - mx) ** 2 for a in x) * sum((b - my) ** 2 for b in y))
print(f"r = {rxy:.4f}")

print("\n== 7. Homofilia: amigos x pares ao acaso (Monte Carlo) ==")
def jac(a, b):
    return len(a & b) / len(a | b)
pares = [(u, v) for u in adj for v in adj[u] if v > u]
p_amigos = sum(1 for u, v in pares if topicos[u] & topicos[v]) / m
j_amigos = sum(jac(topicos[u], topicos[v]) for u, v in pares) / m
rnd = random.Random(1)
amostra = [tuple(rnd.sample(range(n), 2)) for _ in range(200_000)]
p_acaso = sum(1 for u, v in amostra if topicos[u] & topicos[v]) / len(amostra)
j_acaso = sum(jac(topicos[u], topicos[v]) for u, v in amostra) / len(amostra)
ep = math.sqrt(p_amigos * (1 - p_amigos) / m)
print(f"amigos com tópico em comum: {100 * p_amigos:.1f}% | ao acaso: {100 * p_acaso:.1f}% | diferença {100 * (p_amigos - p_acaso):.2f} p.p.")
print(f"erro-padrão sqrt(p(1-p)/m) = {100 * ep:.2f} p.p. -> diferença/erro = {(p_amigos - p_acaso) / ep:.2f}")
print(f"Jaccard médio: amigos {j_amigos:.3f} | acaso {j_acaso:.3f}")

print("\n== 8. Modelo aleatório de Erdős–Rényi G(n, p): previsto x medido ==")
lam = 2 * m / n
S = 1.0
for _ in range(200):
    S = 1 - math.exp(-lam * S)
tri = sum(len(adj[u] & adj[v]) for u, v in pares) // 3
print(f"isolados: previsto n·e^(-λ) = {n * math.exp(-lam):.1f} | medido {grau.count(0)}")
print(f"maior componente: previsto S = 1 - e^(-λS) = {100 * S:.1f}% | medido {100 * max(comps) / n:.1f}%")
print(f"distância média: previsto ln n / ln λ = {math.log(n) / math.log(lam):.2f} | medido no rede.exe (amostra de 200 origens): 6,72")
print(f"triângulos: previsto C(n,3)·p³ = {math.comb(n, 3) * p ** 3:.1f} | medido {tri}")
hist = collections.Counter(grau)
print("grau  medido  Poisson(λ)")
for k in range(max(grau) + 1):
    print(f"{k:4d}  {hist[k]:6d}  {n * math.exp(-lam) * lam ** k / math.factorial(k):9.1f}")
qui2 = sum((hist[k] - n * math.exp(-lam) * lam ** k / math.factorial(k)) ** 2 / (n * math.exp(-lam) * lam ** k / math.factorial(k)) for k in range(0, 11))
print(f"qui-quadrado (graus 0..10 = 11 classes; λ estimado dos dados -> 11 - 1 - 1 = 9 graus de liberdade): {qui2:.1f}  (valor crítico a 5% = 16,9)")
