#include "bench.h"
#include "grafo_lista.h"
#include "grafo_matriz.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CONSULTAS 1000000
#define PARES_COMUNS 10000

unsigned long long aleatorio(unsigned long long *s) { /* xorshift64* */
    *s ^= *s >> 12; *s ^= *s << 25; *s ^= *s >> 27;
    return *s * 2685821657736338717ULL;
}

static double ms_desde(clock_t ini) { return 1000.0 * (double)(clock() - ini) / CLOCKS_PER_SEC; }

typedef struct { const char *nome; const char *complexidade_m; const char *complexidade_l; double t_m[32], t_l[32]; long long chk_m, chk_l; } Medida;

static void media_dp(const double *v, int r, double *media, double *dp) {
    double s = 0, s2 = 0;
    for (int i = 0; i < r; i++) s += v[i];
    *media = s / r;
    for (int i = 0; i < r; i++) s2 += (v[i] - *media) * (v[i] - *media);
    *dp = r > 1 ? sqrt(s2 / (r - 1)) : 0;
}

/* ---- operações medidas (cada uma devolve um checksum para o compilador não descartar o trabalho) ---- */

static long long graus_m(const GrafoMatriz *g) { long long s = 0; for (int v = 0; v < g->n; v++) s += gm_grau(g, v); return s; }
static long long graus_l(const GrafoLista *g)  { long long s = 0; for (int v = 0; v < g->n; v++) s += gl_grau_contando(g, v); return s; }

static long long consultas_m(const GrafoMatriz *g, const int *qa, const int *qb) { long long s = 0; for (int i = 0; i < CONSULTAS; i++) s += gm_existe(g, qa[i], qb[i]); return s; }
static long long consultas_l(const GrafoLista *g, const int *qa, const int *qb)  { long long s = 0; for (int i = 0; i < CONSULTAS; i++) s += gl_existe(g, qa[i], qb[i]); return s; }

/* componentes conexos: BFS a partir de cada vértice ainda não visitado; checksum = nº de componentes * 100000 + maior */
static long long componentes_m(const GrafoMatriz *g, int *dist, int *fila, char *visto) {
    memset(visto, 0, (size_t)g->n);
    int comp = 0, maior = 0;
    for (int s = 0; s < g->n; s++) {
        if (visto[s]) continue;
        int k = gm_bfs(g, s, dist, fila);
        for (int i = 0; i < k; i++) visto[fila[i]] = 1;
        comp++; if (k > maior) maior = k;
    }
    return (long long)comp * 100000 + maior;
}
static long long componentes_l(const GrafoLista *g, int *dist, int *fila, char *visto) {
    memset(visto, 0, (size_t)g->n);
    int comp = 0, maior = 0;
    for (int s = 0; s < g->n; s++) {
        if (visto[s]) continue;
        int k = gl_bfs(g, s, dist, fila);
        for (int i = 0; i < k; i++) visto[fila[i]] = 1;
        comp++; if (k > maior) maior = k;
    }
    return (long long)comp * 100000 + maior;
}

/* amigos em comum: matriz cruza duas linhas inteiras (O(n)); lista marca os vizinhos de a e percorre os de b (O(grau)) */
static long long comuns_m(const GrafoMatriz *g, const int *pa, const int *pb) {
    long long s = 0;
    size_t n = (size_t)g->n;
    for (int i = 0; i < PARES_COMUNS; i++) {
        const unsigned char *la = g->m + (size_t)pa[i] * n, *lb = g->m + (size_t)pb[i] * n;
        for (size_t j = 0; j < n; j++) s += la[j] & lb[j];
    }
    return s;
}
static long long comuns_l(const GrafoLista *g, const int *pa, const int *pb, char *marca) {
    long long s = 0;
    for (int i = 0; i < PARES_COMUNS; i++) {
        for (const No *p = g->adj[pa[i]]; p; p = p->prox) marca[p->v] = 1;
        for (const No *p = g->adj[pb[i]]; p; p = p->prox) s += marca[p->v];
        for (const No *p = g->adj[pa[i]]; p; p = p->prox) marca[p->v] = 0;
    }
    return s;
}

int executar_benchmark(int n, const Amizade *amz, int m, int r, const char *csv) {
    if (r > 32) r = 32;
    Medida md[5] = {
        { "Construir (inserir todas as amizades)", "O(m)", "O(m * grau)", {0}, {0}, 0, 0 },
        { "Grau de todos os usuarios", "O(n^2)", "O(n + m)", {0}, {0}, 0, 0 },
        { "1.000.000 consultas 'sao amigos?'", "O(1) cada", "O(grau) cada", {0}, {0}, 0, 0 },
        { "Componentes conexos (BFS em todos)", "O(n^2)", "O(n + m)", {0}, {0}, 0, 0 },
        { "Amigos em comum (10.000 pares)", "O(n) cada", "O(grau) cada", {0}, {0}, 0, 0 },
    };
    int *qa = malloc(CONSULTAS * sizeof *qa), *qb = malloc(CONSULTAS * sizeof *qb);
    int *pa = malloc(PARES_COMUNS * sizeof *pa), *pb = malloc(PARES_COMUNS * sizeof *pb);
    int *dist = malloc((size_t)n * sizeof *dist), *fila = malloc((size_t)n * sizeof *fila);
    char *visto = malloc((size_t)n), *marca = calloc((size_t)n, 1);
    if (!qa || !qb || !pa || !pb || !dist || !fila || !visto || !marca) { fprintf(stderr, "sem memoria\n"); return 0; }

    /* entradas aleatórias geradas UMA vez (semente fixa) e usadas nas duas estruturas.
       Metade das consultas usa amizades reais, para não medir só o caso "não existe". */
    unsigned long long s = 42;
    for (int i = 0; i < CONSULTAS; i++) {
        if (i % 2 == 0) { const Amizade *a = &amz[aleatorio(&s) % (unsigned)m]; qa[i] = a->a; qb[i] = a->b; }
        else { qa[i] = (int)(aleatorio(&s) % (unsigned)n); qb[i] = (int)(aleatorio(&s) % (unsigned)n); }
    }
    for (int i = 0; i < PARES_COMUNS; i++) { pa[i] = (int)(aleatorio(&s) % (unsigned)n); pb[i] = (int)(aleatorio(&s) % (unsigned)n); }

    GrafoMatriz gm; GrafoLista gl;
    for (int k = 0; k < r; k++) {
        clock_t t;
        /* construir: alocar + inserir; liberar fica fora do tempo */
        t = clock();
        if (!gm_criar(&gm, n)) { fprintf(stderr, "matriz: sem memoria\n"); return 0; }
        for (int i = 0; i < m; i++) gm_inserir(&gm, amz[i].a, amz[i].b);
        md[0].t_m[k] = ms_desde(t); md[0].chk_m = gm.arestas;

        t = clock();
        if (!gl_criar(&gl, n)) { fprintf(stderr, "lista: sem memoria\n"); return 0; }
        for (int i = 0; i < m; i++) gl_inserir(&gl, amz[i].a, amz[i].b);
        md[0].t_l[k] = ms_desde(t); md[0].chk_l = gl.arestas;

        t = clock(); md[1].chk_m = graus_m(&gm); md[1].t_m[k] = ms_desde(t);
        t = clock(); md[1].chk_l = graus_l(&gl); md[1].t_l[k] = ms_desde(t);
        t = clock(); md[2].chk_m = consultas_m(&gm, qa, qb); md[2].t_m[k] = ms_desde(t);
        t = clock(); md[2].chk_l = consultas_l(&gl, qa, qb); md[2].t_l[k] = ms_desde(t);
        t = clock(); md[3].chk_m = componentes_m(&gm, dist, fila, visto); md[3].t_m[k] = ms_desde(t);
        t = clock(); md[3].chk_l = componentes_l(&gl, dist, fila, visto); md[3].t_l[k] = ms_desde(t);
        t = clock(); md[4].chk_m = comuns_m(&gm, pa, pb); md[4].t_m[k] = ms_desde(t);
        t = clock(); md[4].chk_l = comuns_l(&gl, pa, pb, marca); md[4].t_l[k] = ms_desde(t);

        if (k < r - 1) { gm_liberar(&gm); gl_liberar(&gl); }
    }

    int iguais = 1;
    FILE *f = fopen(csv, "w");
    if (f) fprintf(f, "operacao,complexidade_matriz,complexidade_lista,matriz_ms_media,matriz_ms_dp,lista_ms_media,lista_ms_dp,razao_matriz_sobre_lista\n");
    printf("\n%-40s %16s %16s %10s\n", "Operacao (media de 10 execucoes)", "Matriz (ms)", "Lista (ms)", "Matriz/Lista");
    for (int i = 0; i < 5; i++) {
        double mm, dm, ml, dl;
        media_dp(md[i].t_m, r, &mm, &dm);
        media_dp(md[i].t_l, r, &ml, &dl);
        double razao = ml > 0 ? mm / ml : 0;
        printf("%-40s %9.2f +-%5.2f %9.2f +-%5.2f %9.1fx%s\n", md[i].nome, mm, dm, ml, dl, razao, md[i].chk_m == md[i].chk_l ? "" : "  << RESULTADOS DIFERENTES");
        if (md[i].chk_m != md[i].chk_l) iguais = 0;
        if (f) fprintf(f, "\"%s\",%s,%s,%.3f,%.3f,%.3f,%.3f,%.2f\n", md[i].nome, md[i].complexidade_m, md[i].complexidade_l, mm, dm, ml, dl, razao);
    }
    printf("\nMemoria: matriz = %.1f MB | lista = %.2f MB (%.0fx menos)\n",
           gm_bytes(&gm) / 1048576.0, gl_bytes(&gl) / 1048576.0, (double)gm_bytes(&gm) / (double)gl_bytes(&gl));
    if (f) { fprintf(f, "\"Memoria (MB)\",n^2,n+m,%.2f,0,%.3f,0,%.1f\n", gm_bytes(&gm) / 1048576.0, gl_bytes(&gl) / 1048576.0, (double)gm_bytes(&gm) / (double)gl_bytes(&gl)); fclose(f); }
    printf("Conferencia: as duas estruturas deram %s resultados em todas as operacoes.\n", iguais ? "os MESMOS" : "resultados DIFERENTES (erro!)");

    gm_liberar(&gm); gl_liberar(&gl);
    free(qa); free(qb); free(pa); free(pb); free(dist); free(fila); free(visto); free(marca);
    return iguais;
}
