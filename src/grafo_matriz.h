#ifndef GRAFO_MATRIZ_H
#define GRAFO_MATRIZ_H

#include <stddef.h>

/* Grafo não direcionado em MATRIZ DE ADJACÊNCIA: m[i*n + j] = 1 se i e j são amigos.
   Memória: n*n bytes, mesmo que existam poucas arestas (n = 10.000 → 100.000.000 bytes ≈ 95 MB). */
typedef struct {
    int n;
    unsigned char *m;
    int arestas;
} GrafoMatriz;

int  gm_criar(GrafoMatriz *g, int n);                 /* 1 = ok, 0 = sem memória */
int  gm_inserir(GrafoMatriz *g, int a, int b);        /* O(1); 1 = nova, 0 = já existia */
int  gm_existe(const GrafoMatriz *g, int a, int b);   /* O(1) */
int  gm_grau(const GrafoMatriz *g, int v);            /* O(n): percorre a linha inteira */
int  gm_vizinhos(const GrafoMatriz *g, int v, int *saida); /* O(n); devolve quantos */
int  gm_bfs(const GrafoMatriz *g, int origem, int *dist, int *fila); /* O(n²); devolve alcançados */
size_t gm_bytes(const GrafoMatriz *g);
void gm_liberar(GrafoMatriz *g);

#endif
