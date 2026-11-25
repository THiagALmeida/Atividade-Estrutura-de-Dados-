#ifndef GRAFO_LISTA_H
#define GRAFO_LISTA_H

#include <stddef.h>

/* Grafo não direcionado em LISTA DE ADJACÊNCIA: cada vértice tem uma lista encadeada
   com os amigos. Memória: n ponteiros + 2 nós por aresta (proporcional a n + m). */
typedef struct No {
    int v;
    struct No *prox;
} No;

typedef struct {
    int n;
    No **adj;
    int *grau;      /* grau guardado na inserção: consulta O(1) */
    int arestas;
} GrafoLista;

int  gl_criar(GrafoLista *g, int n);
int  gl_inserir(GrafoLista *g, int a, int b);        /* O(grau(a)) por causa da checagem de repetida; 1 = nova, 0 = já existia, -1 = sem memória */
int  gl_existe(const GrafoLista *g, int a, int b);   /* O(grau(a)) */
int  gl_grau(const GrafoLista *g, int v);            /* O(1) */
int  gl_grau_contando(const GrafoLista *g, int v);   /* O(grau): percorre a lista (para comparar com a matriz) */
int  gl_vizinhos(const GrafoLista *g, int v, int *saida); /* O(grau) */
int  gl_bfs(const GrafoLista *g, int origem, int *dist, int *fila); /* O(n + m) */
size_t gl_bytes(const GrafoLista *g);
void gl_liberar(GrafoLista *g);

#endif
