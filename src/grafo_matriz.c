#include "grafo_matriz.h"

#include <stdlib.h>

int gm_criar(GrafoMatriz *g, int n) {
    g->n = n;
    g->arestas = 0;
    g->m = calloc((size_t)n * (size_t)n, 1);
    return g->m != NULL;
}

int gm_inserir(GrafoMatriz *g, int a, int b) {
    size_t n = (size_t)g->n;
    if (g->m[(size_t)a * n + (size_t)b]) return 0; /* amizade repetida no arquivo */
    g->m[(size_t)a * n + (size_t)b] = 1;
    g->m[(size_t)b * n + (size_t)a] = 1;           /* não direcionado: simétrica */
    g->arestas++;
    return 1;
}

int gm_existe(const GrafoMatriz *g, int a, int b) {
    return g->m[(size_t)a * (size_t)g->n + (size_t)b];
}

int gm_grau(const GrafoMatriz *g, int v) {
    const unsigned char *linha = g->m + (size_t)v * (size_t)g->n;
    int grau = 0;
    for (int j = 0; j < g->n; j++) grau += linha[j];
    return grau;
}

int gm_vizinhos(const GrafoMatriz *g, int v, int *saida) {
    const unsigned char *linha = g->m + (size_t)v * (size_t)g->n;
    int k = 0;
    for (int j = 0; j < g->n; j++)
        if (linha[j]) saida[k++] = j;
    return k;
}

int gm_bfs(const GrafoMatriz *g, int origem, int *dist, int *fila) {
    for (int i = 0; i < g->n; i++) dist[i] = -1;
    int ini = 0, fim = 0;
    dist[origem] = 0;
    fila[fim++] = origem;
    while (ini < fim) {
        int u = fila[ini++];
        const unsigned char *linha = g->m + (size_t)u * (size_t)g->n;
        for (int v = 0; v < g->n; v++) {          /* precisa olhar as n colunas */
            if (linha[v] && dist[v] < 0) {
                dist[v] = dist[u] + 1;
                fila[fim++] = v;
            }
        }
    }
    return fim;
}

size_t gm_bytes(const GrafoMatriz *g) { return (size_t)g->n * (size_t)g->n; }

void gm_liberar(GrafoMatriz *g) {
    free(g->m);
    g->m = NULL;
    g->n = g->arestas = 0;
}
