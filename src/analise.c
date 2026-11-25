#include "analise.h"
#include "bench.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int pagerank(const GrafoLista *g, double *pr, double d, double tol, int max_iter) {
    int n = g->n;
    double *novo = malloc((size_t)n * sizeof *novo);
    if (!novo) return 0;
    for (int i = 0; i < n; i++) pr[i] = 1.0 / n;
    int it;
    for (it = 1; it <= max_iter; it++) {
        double pendente = 0; /* nota dos vértices sem amigos: vai para todos */
        for (int i = 0; i < n; i++) if (g->grau[i] == 0) pendente += pr[i];
        double base = (1.0 - d) / n + d * pendente / n;
        for (int i = 0; i < n; i++) novo[i] = base;
        for (int u = 0; u < n; u++) {
            if (g->grau[u] == 0) continue;
            double parte = d * pr[u] / g->grau[u];
            for (const No *p = g->adj[u]; p; p = p->prox) novo[p->v] += parte;
        }
        double dif = 0;
        for (int i = 0; i < n; i++) { dif += fabs(novo[i] - pr[i]); pr[i] = novo[i]; }
        if (dif < tol) break;
    }
    free(novo);
    return it;
}

static const double *g_pr;
static const GrafoLista *g_g;
static int cmp_influencia(const void *a, const void *b) {
    int x = *(const int *)a, y = *(const int *)b;
    if (g_g->grau[x] != g_g->grau[y]) return g_g->grau[y] - g_g->grau[x];
    if (g_pr[x] != g_pr[y]) return g_pr[y] > g_pr[x] ? 1 : -1;
    return x - y;
}
static int cmp_pr(const void *a, const void *b) {
    int x = *(const int *)a, y = *(const int *)b;
    if (g_pr[x] != g_pr[y]) return g_pr[y] > g_pr[x] ? 1 : -1;
    return x - y;
}

static double pearson(const double *x, const double *y, int n) {
    double mx = 0, my = 0;
    for (int i = 0; i < n; i++) { mx += x[i]; my += y[i]; }
    mx /= n; my /= n;
    double sxy = 0, sxx = 0, syy = 0;
    for (int i = 0; i < n; i++) { sxy += (x[i] - mx) * (y[i] - my); sxx += (x[i] - mx) * (x[i] - mx); syy += (y[i] - my) * (y[i] - my); }
    return (sxx > 0 && syy > 0) ? sxy / sqrt(sxx * syy) : 0;
}

static void topicos_texto(uint16_t mask, char *buf, size_t tam) {
    buf[0] = '\0';
    for (int t = 0; t < topicos_total(); t++) {
        if (!(mask & (1u << t))) continue;
        if (buf[0]) strncat(buf, ", ", tam - strlen(buf) - 1);
        strncat(buf, topico_nome(t), tam - strlen(buf) - 1);
    }
}

typedef struct { int v; int comuns; double jac; } Sugestao;

/* Até k sugestões de amizade para u: amigos de amigos que ainda não são amigos,
   ordenados por amigos em comum e, no empate, por interesses parecidos (Jaccard). */
static int sugerir(const GrafoLista *g, const Usuario *us, int u, int *contagem, int *tocados, Sugestao *out, int k) {
    int nt = 0;
    for (const No *p = g->adj[u]; p; p = p->prox)
        for (const No *q = g->adj[p->v]; q; q = q->prox) {
            int w = q->v;
            if (w == u || gl_existe(g, u, w)) continue;
            if (contagem[w]++ == 0) tocados[nt++] = w;
        }
    int k_out = 0;
    for (int i = 0; i < nt; i++) {
        int w = tocados[i];
        Sugestao s = { w, contagem[w], jaccard(us[u].topicos, us[w].topicos) };
        /* inserção ordenada nas k melhores */
        int pos = k_out < k ? k_out++ : k;
        while (pos > 0 && (out[pos - 1].comuns < s.comuns || (out[pos - 1].comuns == s.comuns && out[pos - 1].jac < s.jac))) {
            if (pos < k) out[pos] = out[pos - 1];
            pos--;
        }
        if (pos < k) out[pos] = s;
    }
    for (int i = 0; i < nt; i++) contagem[tocados[i]] = 0;
    return k_out;
}


void analisar_rede(const GrafoLista *g, const Usuario *us, int duplicadas, const char *saida) {
    int n = g->n, m = g->arestas;
    char caminho[512], tbuf[160];
    int *dist = malloc((size_t)n * sizeof *dist), *fila = malloc((size_t)n * sizeof *fila);
    int *ordem = malloc((size_t)n * sizeof *ordem), *comp = malloc((size_t)n * sizeof *comp);
    double *pr = malloc((size_t)n * sizeof *pr);
    if (!dist || !fila || !ordem || !comp || !pr) { fprintf(stderr, "sem memoria\n"); return; }

    /* ---------- 1. visão geral ---------- */
    int isolados = 0, grau_max = 0;
    for (int v = 0; v < n; v++) { if (g->grau[v] == 0) isolados++; if (g->grau[v] > grau_max) grau_max = g->grau[v]; }
    int ncomp = 0, maior = 0, id_maior = -1;
    for (int v = 0; v < n; v++) comp[v] = -1;
    for (int s = 0; s < n; s++) {
        if (comp[s] >= 0) continue;
        int k = gl_bfs(g, s, dist, fila);
        for (int i = 0; i < k; i++) comp[fila[i]] = ncomp;
        if (k > maior) { maior = k; id_maior = ncomp; }
        ncomp++;
    }
    /* graus de separação: BFS a partir de 200 usuários sorteados do maior componente */
    unsigned long long sem = 7;
    long long soma_dist = 0, pares = 0; int diam = 0;
    for (int amostra = 0; amostra < 200; ) {
        int s = (int)(aleatorio(&sem) % (unsigned)n);
        if (comp[s] != id_maior) continue;
        int k = gl_bfs(g, s, dist, fila);
        for (int i = 1; i < k; i++) { soma_dist += dist[fila[i]]; pares++; if (dist[fila[i]] > diam) diam = dist[fila[i]]; }
        amostra++;
    }
    printf("\n=== 1. A rede ===\n");
    printf("Usuarios: %d | amizades unicas: %d (%d repetidas no arquivo foram ignoradas)\n", n, m, duplicadas);
    printf("Grau medio: %.2f amigos | maior grau: %d | sem nenhum amigo: %d (%.1f%%)\n", 2.0 * m / n, grau_max, isolados, 100.0 * isolados / n);
    printf("Densidade: %.5f%% das amizades possiveis existem -> grafo ESPARSO\n", 100.0 * 2.0 * m / ((double)n * (n - 1)));
    printf("Componentes conexos: %d | o maior tem %d usuarios (%.1f%%)\n", ncomp, maior, 100.0 * maior / n);
    printf("Graus de separacao no maior componente (amostra de 200 origens): media %.2f, maximo visto %d\n", (double)soma_dist / (double)pares, diam);

    /* distribuição de graus */
    int *hist = calloc((size_t)grau_max + 1, sizeof *hist);
    if (hist) {
        for (int v = 0; v < n; v++) hist[g->grau[v]]++;
        snprintf(caminho, sizeof caminho, "%s/distribuicao_graus.csv", saida);
        FILE *f = fopen(caminho, "w");
        if (f) { fprintf(f, "grau,usuarios\n"); for (int k = 0; k <= grau_max; k++) fprintf(f, "%d,%d\n", k, hist[k]); fclose(f); }
        free(hist);
    }

    /* ---------- 2. influência ---------- */
    int it = pagerank(g, pr, 0.85, 1e-10, 200);
    for (int i = 0; i < n; i++) ordem[i] = i;
    g_g = g; g_pr = pr;
    qsort(ordem, (size_t)n, sizeof *ordem, cmp_influencia);
    printf("\n=== 2. Usuarios mais influentes (grau; empate decidido pelo PageRank, %d iteracoes) ===\n", it);
    printf("%-4s %-22s %-16s %5s %9s %6s  %s\n", "#", "Nome", "Usuario", "Grau", "PageRank", "Postes", "Topicos");
    snprintf(caminho, sizeof caminho, "%s/ranking.csv", saida);
    FILE *fr = fopen(caminho, "w");
    if (fr) fprintf(fr, "posicao,id,nome,usuario,grau,pagerank_x_n,postes,anos,topicos\n");
    for (int i = 0; i < n; i++) {
        const Usuario *u = &us[ordem[i]];
        topicos_texto(u->topicos, tbuf, sizeof tbuf);
        if (i < 10) printf("%-4d %-22.22s %-16.16s %5d %9.3f %6d  %s\n", i + 1, u->nome, u->usuario, g->grau[ordem[i]], pr[ordem[i]] * n, u->postes, tbuf);
        if (fr && i < 100) fprintf(fr, "%d,%d,\"%s\",%s,%d,%.4f,%d,%.2f,\"%s\"\n", i + 1, u->id, u->nome, u->usuario, g->grau[ordem[i]], pr[ordem[i]] * n, u->postes, u->anos, tbuf);
    }
    if (fr) fclose(fr);
    printf("(PageRank x n: 1,0 = media; quem tem amigos bem conectados sobe mesmo com o mesmo grau)\n");

    qsort(ordem, (size_t)n, sizeof *ordem, cmp_pr);
    printf("Top 5 so por PageRank: ");
    for (int i = 0; i < 5; i++) printf("%s%s (grau %d, %.2f)", i ? "; " : "", us[ordem[i]].usuario, g->grau[ordem[i]], pr[ordem[i]] * n);
    printf("\n");

    double *x = malloc((size_t)n * sizeof *x), *y = malloc((size_t)n * sizeof *y), *z = malloc((size_t)n * sizeof *z);
    if (x && y && z) {
        for (int i = 0; i < n; i++) { x[i] = g->grau[i]; y[i] = us[i].postes; z[i] = us[i].anos; }
        printf("Correlacao (Pearson) grau x postes = %.3f | grau x tempo na rede = %.3f -> perto de 0: postar mais ou estar ha mais tempo NAO da mais amigos nesta base\n", pearson(x, y, n), pearson(x, z, n));
    }
    free(x); free(y); free(z);

    /* ---------- 3. interesses ---------- */
    printf("\n=== 3. Interesses ===\n");
    printf("Usuarios por topico: ");
    for (int t = 0; t < topicos_total(); t++) {
        int c = 0;
        for (int i = 0; i < n; i++) if (us[i].topicos & (1u << t)) c++;
        printf("%s%s %d", t ? " | " : "", topico_nome(t), c);
    }
    printf("\n");
    /* homofilia: amigos compartilham mais interesses do que pares ao acaso? */
    long long amigos_comum = 0; double jac_amigos = 0;
    for (int u = 0; u < n; u++)
        for (const No *p = g->adj[u]; p; p = p->prox) {
            if (p->v < u) continue; /* cada amizade uma vez */
            if (topicos_em_comum(us[u].topicos, us[p->v].topicos)) amigos_comum++;
            jac_amigos += jaccard(us[u].topicos, us[p->v].topicos);
        }
    long long acaso_comum = 0; double jac_acaso = 0; const int AMOSTRA = 200000;
    for (int i = 0; i < AMOSTRA; i++) {
        int a = (int)(aleatorio(&sem) % (unsigned)n), b = (int)(aleatorio(&sem) % (unsigned)n);
        if (a == b) { i--; continue; }
        if (topicos_em_comum(us[a].topicos, us[b].topicos)) acaso_comum++;
        jac_acaso += jaccard(us[a].topicos, us[b].topicos);
    }
    printf("Amizades com pelo menos 1 topico em comum: %.1f%% (pares ao acaso: %.1f%%)\n", 100.0 * amigos_comum / m, 100.0 * acaso_comum / AMOSTRA);
    printf("Similaridade media de interesses (Jaccard): amigos %.3f x ao acaso %.3f\n", jac_amigos / m, jac_acaso / AMOSTRA);
    printf("-> %s\n", fabs(jac_amigos / m - jac_acaso / AMOSTRA) < 0.02
        ? "sem homofilia: nesta base as amizades nao seguem os interesses (os dados parecem gerados ao acaso). Interesse em comum serve para SUGERIR amizades, nao para explicar as que existem."
        : "ha homofilia: amigos compartilham mais interesses do que pares ao acaso.");

    /* grupos com exatamente os mesmos interesses (2 ou mais tópicos) */
    int *grupos = calloc(1u << 16, sizeof *grupos);
    if (grupos) {
        for (int i = 0; i < n; i++) if (popcount16(us[i].topicos) >= 2) grupos[us[i].topicos]++;
        int mascaras[5] = { 0 }, tam[5] = { 0 };
        for (int msk = 0; msk < (1 << 16); msk++) {
            int c = grupos[msk];
            for (int k = 0; k < 5; k++) if (c > tam[k]) {
                for (int j = 4; j > k; j--) { tam[j] = tam[j - 1]; mascaras[j] = mascaras[j - 1]; }
                tam[k] = c; mascaras[k] = msk; break;
            }
        }
        printf("Maiores grupos com EXATAMENTE os mesmos interesses (2+ topicos):\n");
        for (int k = 0; k < 5 && tam[k]; k++) { topicos_texto((uint16_t)mascaras[k], tbuf, sizeof tbuf); printf("  %3d usuarios: %s\n", tam[k], tbuf); }
        free(grupos);
    }
    int gemeos = 0;
    for (int u = 0; u < n; u++)
        for (const No *p = g->adj[u]; p; p = p->prox)
            if (p->v > u && us[u].topicos == us[p->v].topicos && popcount16(us[u].topicos) >= 2) gemeos++;
    printf("Pares de AMIGOS com os mesmos interesses (2+ topicos): %d\n", gemeos);

    /* ---------- 4. sugestões de amizade ---------- */
    int *contagem = calloc((size_t)n, sizeof *contagem), *tocados = malloc((size_t)n * sizeof *tocados);
    if (contagem && tocados) {
        snprintf(caminho, sizeof caminho, "%s/recomendacoes.csv", saida);
        FILE *fs = fopen(caminho, "w");
        if (fs) fprintf(fs, "id,usuario,sugestao_id,sugestao_usuario,amigos_em_comum,jaccard_interesses\n");
        Sugestao sug[3];
        int com_sugestao = 0;
        for (int u = 0; u < n; u++) {
            int k = sugerir(g, us, u, contagem, tocados, sug, 3);
            if (k) com_sugestao++;
            for (int i = 0; fs && i < k; i++) fprintf(fs, "%d,%s,%d,%s,%d,%.3f\n", us[u].id, us[u].usuario, us[sug[i].v].id, us[sug[i].v].usuario, sug[i].comuns, sug[i].jac);
        }
        if (fs) fclose(fs);
        qsort(ordem, (size_t)n, sizeof *ordem, cmp_influencia);
        printf("\n=== 4. Sugestoes de amizade (amigos de amigos; desempate por interesses) ===\n");
        for (int i = 0; i < 3; i++) {
            int u = ordem[i];
            int k = sugerir(g, us, u, contagem, tocados, sug, 3);
            printf("%s:", us[u].usuario);
            for (int j = 0; j < k; j++) printf(" %s (%d em comum, Jaccard %.2f)%s", us[sug[j].v].usuario, sug[j].comuns, sug[j].jac, j + 1 < k ? ";" : "");
            printf("\n");
        }
        printf("%d usuarios receberam sugestoes (lista completa em recomendacoes.csv)\n", com_sugestao);
    }
    free(contagem); free(tocados);

    /* amigos em comum mais altos entre amigos (triângulos) */
    int *tri = calloc((size_t)n, sizeof *tri), *marca = calloc((size_t)n, sizeof *marca);
    if (tri && marca) {
        long long triangulos = 0;
        for (int u = 0; u < n; u++) {
            for (const No *p = g->adj[u]; p; p = p->prox) marca[p->v] = u + 1;
            for (const No *p = g->adj[u]; p; p = p->prox)
                for (const No *q = g->adj[p->v]; q; q = q->prox)
                    if (marca[q->v] == u + 1) tri[u]++;
        }
        for (int u = 0; u < n; u++) { tri[u] /= 2; triangulos += tri[u]; }
        printf("Triangulos (grupos de 3 amigos mutuos): %lld -> amizades quase nunca fecham grupo, outro sinal de rede gerada ao acaso\n", triangulos / 3);
    }
    free(tri); free(marca);

    free(dist); free(fila); free(ordem); free(comp); free(pr);
}
