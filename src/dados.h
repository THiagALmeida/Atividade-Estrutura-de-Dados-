#ifndef DADOS_H
#define DADOS_H

#include <stdint.h>
#include <stddef.h>

/* Usuário da rede social (linha do arquivo "usuários rede social"). */
typedef struct {
    int id;              /* UsuarioID (1..n); no grafo o vértice é id - 1 */
    char nome[64];
    char usuario[40];
    int postes;
    double anos;         /* TempoNaRedeSocial (Anos) */
    uint16_t topicos;    /* bit i ligado = gosta do tópico i (ver topico_nome) */
} Usuario;

typedef struct { int a, b; } Amizade; /* índices de vértice (0..n-1) */

#define MAX_TOPICOS 16

/* Carrega os usuários. Devolve o vetor (o chamador libera) e o tamanho em *n; NULL em erro. */
Usuario *carregar_usuarios(const char *caminho, int *n);

/* Carrega as amizades como pares de vértices. Ignora linhas inválidas e IDs fora de 1..n
   (contados em *ignoradas). Devolve o vetor (o chamador libera) e o tamanho em *m. */
Amizade *carregar_amizades(const char *caminho, int n, int *m, int *ignoradas);

int topicos_total(void);
const char *topico_nome(int i);
int topicos_em_comum(uint16_t a, uint16_t b);  /* quantos tópicos os dois têm */
double jaccard(uint16_t a, uint16_t b);        /* |A ∩ B| / |A ∪ B| */
int popcount16(uint16_t x);

#endif
