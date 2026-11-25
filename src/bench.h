#ifndef BENCH_H
#define BENCH_H

#include "dados.h"

/* Mede, com clock() de <time.h>, as mesmas operações na matriz e na lista:
   construir, grau de todos, consultas de aresta, BFS em todos os componentes e
   amigos em comum. Imprime a tabela e grava resultados/benchmark.csv.
   Devolve 1 se as duas estruturas deram os mesmos resultados (checksums iguais). */
int executar_benchmark(int n, const Amizade *amz, int m, int repeticoes, const char *csv);

/* Gerador pseudoaleatório com semente fixa (rand() do MSVC só vai até 32767). */
unsigned long long aleatorio(unsigned long long *estado);

#endif
