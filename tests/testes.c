/* Testes: grafo pequeno conhecido + conferência matriz x lista nos dados reais. */
#include "../src/analise.h"
#include "../src/dados.h"
#include "../src/grafo_lista.h"
#include "../src/grafo_matriz.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int falhas = 0, verificacoes = 0;
#define CHECK(c) do { verificacoes++; if (!(c)) { falhas++; printf("FALHOU %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)

static void grafo_pequeno(void) {
    /* 0-1, 1-2, 1-3, 4-5 (como o exemplo dos slides: A-B, B-C, B-D, E-F) + 1 repetida + vértice 6 isolado */
    int arestas[][2] = { {0, 1}, {1, 2}, {1, 3}, {4, 5}, {1, 0} };
    GrafoMatriz gm; GrafoLista gl;
    CHECK(gm_criar(&gm, 7)); CHECK(gl_criar(&gl, 7));
    int novas_m = 0, novas_l = 0;
    for (int i = 0; i < 5; i++) { novas_m += gm_inserir(&gm, arestas[i][0], arestas[i][1]); novas_l += gl_inserir(&gl, arestas[i][0], arestas[i][1]); }
    CHECK(novas_m == 4 && novas_l == 4);              /* a repetida (1,0) foi ignorada */
    CHECK(gm.arestas == 4 && gl.arestas == 4);
    CHECK(gm_existe(&gm, 2, 1) && gl_existe(&gl, 2, 1)); /* simétrico */
    CHECK(!gm_existe(&gm, 3, 5) && !gl_existe(&gl, 3, 5));
    int graus[] = { 1, 3, 1, 1, 1, 1, 0 };
    for (int v = 0; v < 7; v++) { CHECK(gm_grau(&gm, v) == graus[v]); CHECK(gl_grau(&gl, v) == graus[v]); CHECK(gl_grau_contando(&gl, v) == graus[v]); }
    int dm[7], dl[7], fm[7], fl[7];
    CHECK(gm_bfs(&gm, 0, dm, fm) == 4 && gl_bfs(&gl, 0, dl, fl) == 4); /* componente {0,1,2,3} */
    CHECK(dm[3] == 2 && dl[3] == 2);                                    /* 0 -> 1 -> 3 */
    CHECK(dm[4] == -1 && dl[4] == -1);                                  /* outro componente */
    double pr[7], soma = 0;
    pagerank(&gl, pr, 0.85, 1e-12, 500);
    for (int v = 0; v < 7; v++) soma += pr[v];
    CHECK(fabs(soma - 1.0) < 1e-9);                                     /* PageRank soma 1 */
    CHECK(pr[1] > pr[0] && pr[1] > pr[6]);                              /* o "B" central é o mais influente */
    gm_liberar(&gm); gl_liberar(&gl);
}

static void topicos(void) {
    CHECK(popcount16(0x0B) == 3);
    CHECK(topicos_em_comum(0x03, 0x06) == 1);
    CHECK(fabs(jaccard(0x03, 0x06) - 1.0 / 3.0) < 1e-12);
    CHECK(jaccard(0, 0) == 0.0);
}

static void dados_reais(const char *pasta) {
    char cu[512], ca[512];
    snprintf(cu, sizeof cu, "%s/usuários rede social.txt", pasta);
    snprintf(ca, sizeof ca, "%s/amizades rede social.txt", pasta);
    int n = 0, m = 0, ign = 0;
    Usuario *us = carregar_usuarios(cu, &n);
    Amizade *amz = us ? carregar_amizades(ca, n, &m, &ign) : NULL;
    CHECK(us && amz);
    if (!us || !amz) { free(us); return; }
    CHECK(n == 10000 && m == 19998 && ign == 0);
    CHECK(topicos_total() == 10);
    CHECK(us[1].id == 2 && popcount16(us[1].topicos) == 3); /* "Esportes, Saúde, Tecnologia" entre aspas */
    GrafoMatriz gm = { 0 }; GrafoLista gl = { 0 };
    CHECK(gm_criar(&gm, n));
    CHECK(gl_criar(&gl, n));
    if (!gm.m || !gl.adj) { gm_liberar(&gm); gl_liberar(&gl); free(us); free(amz); return; }
    for (int i = 0; i < m; i++) { gm_inserir(&gm, amz[i].a, amz[i].b); gl_inserir(&gl, amz[i].a, amz[i].b); }
    CHECK(gm.arestas == 19996 && gl.arestas == 19996);       /* 2 pares repetidos no arquivo */
    int iguais = 1;
    for (int v = 0; v < n; v++) if (gm_grau(&gm, v) != gl_grau(&gl, v)) iguais = 0;
    CHECK(iguais);
    int *dm = malloc((size_t)n * sizeof *dm), *dl = malloc((size_t)n * sizeof *dl), *f = malloc((size_t)n * sizeof *f);
    if (dm && dl && f) {
        int km = gm_bfs(&gm, 0, dm, f), kl = gl_bfs(&gl, 0, dl, f);
        CHECK(km == kl);
        int mesmas = 1;
        for (int v = 0; v < n; v++) if (dm[v] != dl[v]) mesmas = 0;
        CHECK(mesmas);                                       /* mesmas distâncias nas duas estruturas */
    }
    free(dm); free(dl); free(f);
    gm_liberar(&gm); gl_liberar(&gl); free(us); free(amz);
}

int main(int argc, char **argv) {
    grafo_pequeno();
    topicos();
    dados_reais(argc > 1 ? argv[1] : "../../atividade de grafos");
    printf("%d verificacoes, %d falhas\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
