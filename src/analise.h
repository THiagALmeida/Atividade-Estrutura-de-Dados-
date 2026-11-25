#ifndef ANALISE_H
#define ANALISE_H

#include "dados.h"
#include "grafo_lista.h"

/* Análise da rede (feita na lista de adjacência, que é a estrutura certa para grafo esparso).
   Imprime o relatório e grava, na pasta `saida`, ranking.csv, recomendacoes.csv e distribuicao_graus.csv. */
void analisar_rede(const GrafoLista *g, const Usuario *u, int duplicadas, const char *saida);

/* PageRank (d = 0,85; vértices sem amigos distribuem a nota igualmente). Devolve iterações usadas. */
int pagerank(const GrafoLista *g, double *pr, double d, double tolerancia, int max_iter);

#endif
