#include "grafo_lista.h"

#include <stdlib.h>

int gl_criar(GrafoLista *g, int n) {
    g->n = n;
    g->arestas = 0;
    g->adj = calloc((size_t)n, sizeof *g->adj);
    g->grau = calloc((size_t)n, sizeof *g->grau);
    if (!g->adj || !g->grau) { free(g->adj); free(g->grau); g->adj = NULL; g->grau = NULL; return 0; }
    return 1;
}

int gl_existe(const GrafoLista *g, int a, int b) {
    for (const No *p = g->adj[a]; p; p = p->prox)
        if (p->v == b) return 1;
    return 0;
}

static int empilhar(GrafoLista *g, int a, int b) {
    No *no = malloc(sizeof *no);
    if (!no) return 0;
    no->v = b;
    no->prox = g->adj[a];   /* insere no início: O(1) */
    g->adj[a] = no;
    g->grau[a]++;
    return 1;
}

int gl_inserir(GrafoLista *g, int a, int b) {
    if (gl_existe(g, a, b)) return 0;            /* amizade repetida no arquivo */
    if (!empilhar(g, a, b)) return -1;
    if (!empilhar(g, b, a)) {                     /* desfaz a metade já feita */
        No *no = g->adj[a]; g->adj[a] = no->prox; g->grau[a]--; free(no);
        return -1;
    }
    g->arestas++;
    return 1;
}

int gl_grau(const GrafoLista *g, int v) { return g->grau[v]; }

int gl_grau_contando(const GrafoLista *g, int v) {
    int k = 0;
    for (const No *p = g->adj[v]; p; p = p->prox) k++;
    return k;
}

int gl_vizinhos(const GrafoLista *g, int v, int *saida) {
    int k = 0;
    for (const No *p = g->adj[v]; p; p = p->prox) saida[k++] = p->v;
    return k;
}

int gl_bfs(const GrafoLista *g, int origem, int *dist, int *fila) {
    for (int i = 0; i < g->n; i++) dist[i] = -1;
    int ini = 0, fim = 0;
    dist[origem] = 0;
    fila[fim++] = origem;
    while (ini < fim) {
        int u = fila[ini++];
        for (const No *p = g->adj[u]; p; p = p->prox) { /* só os vizinhos de verdade */
            if (dist[p->v] < 0) {
                dist[p->v] = dist[u] + 1;
                fila[fim++] = p->v;
            }
        }
    }
    return fim;
}

size_t gl_bytes(const GrafoLista *g) {
    return (size_t)g->n * (sizeof(No *) + sizeof(int)) + (size_t)g->arestas * 2 * sizeof(No);
}

void gl_liberar(GrafoLista *g) {
    if (g->adj) {
        for (int i = 0; i < g->n; i++) {
            No *p = g->adj[i];
            while (p) { No *q = p->prox; free(p); p = q; }
        }
    }
    free(g->adj); free(g->grau);
    g->adj = NULL; g->grau = NULL;
    g->n = g->arestas = 0;
}
