/* TDE 02 — Grafo da rede social em matriz e lista de adjacência.
   Uso: rede.exe [pasta_com_os_arquivos] [--repeticoes N] [--sem-bench]
   A pasta padrão é a da atividade: ..\..\atividade de grafos (relativa à pasta do TDE). */
#include "analise.h"
#include "bench.h"
#include "dados.h"
#include "grafo_lista.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define criar_pasta(p) _mkdir(p)
#else
#include <sys/stat.h>
#define criar_pasta(p) mkdir(p, 0755)
#endif

int main(int argc, char **argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8); /* nomes e tópicos em UTF-8 (Saúde, Educação) */
#endif
    const char *pasta = "../../atividade de grafos";
    int repeticoes = 10, bench = 1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--repeticoes") == 0 && i + 1 < argc) repeticoes = atoi(argv[++i]);
        else if (strcmp(argv[i], "--sem-bench") == 0) bench = 0;
        else pasta = argv[i];
    }
    if (repeticoes < 1) repeticoes = 1;

    char cu[512], ca[512];
    snprintf(cu, sizeof cu, "%s/usuários rede social.txt", pasta);
    snprintf(ca, sizeof ca, "%s/amizades rede social.txt", pasta);

    clock_t t = clock();
    int n = 0, m = 0, ignoradas = 0;
    Usuario *us = carregar_usuarios(cu, &n);
    Amizade *amz = us ? carregar_amizades(ca, n, &m, &ignoradas) : NULL;
    if (!us || !amz) {
        fprintf(stderr, "Nao consegui ler os arquivos em \"%s\". Passe a pasta como argumento.\n", pasta);
        free(us); free(amz);
        return 1;
    }
    double t_leitura = 1000.0 * (double)(clock() - t) / CLOCKS_PER_SEC;
    printf("Leitura dos arquivos: %d usuarios, %d linhas de amizade (%d invalidas), %d topicos, %.1f ms (fora do benchmark)\n",
           n, m, ignoradas, topicos_total(), t_leitura);

    criar_pasta("resultados");
    int ok = 1;
    if (bench) {
        printf("\n=== 0. Matriz x lista de adjacencia (%d repeticoes, clock() de <time.h>) ===", repeticoes);
        ok = executar_benchmark(n, amz, m, repeticoes, "resultados/benchmark.csv");
    }

    GrafoLista g;
    if (!gl_criar(&g, n)) { fprintf(stderr, "sem memoria\n"); return 1; }
    int duplicadas = 0;
    for (int i = 0; i < m; i++) {
        int r = gl_inserir(&g, amz[i].a, amz[i].b);
        if (r == 0) duplicadas++;
        else if (r < 0) { fprintf(stderr, "sem memoria\n"); return 1; }
    }
    analisar_rede(&g, us, duplicadas, "resultados");
    printf("\nArquivos gerados em resultados/: benchmark.csv, ranking.csv, recomendacoes.csv, distribuicao_graus.csv\n");

    gl_liberar(&g);
    free(us); free(amz);
    return ok ? 0 : 2;
}
