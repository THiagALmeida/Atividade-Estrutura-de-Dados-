#include "dados.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* fopen com caminho em UTF-8 ("usuários rede social.txt"): no Windows o fopen usa a página
   de código ANSI e não acha o arquivo; convertemos para UTF-16 e usamos _wfopen. */
static FILE *abrir_utf8(const char *caminho, const char *modo) {
#ifdef _WIN32
    wchar_t wc[1024], wm[8];
    if (!MultiByteToWideChar(CP_UTF8, 0, caminho, -1, wc, 1024)) return NULL;
    if (!MultiByteToWideChar(CP_UTF8, 0, modo, -1, wm, 8)) return NULL;
    return _wfopen(wc, wm);
#else
    return fopen(caminho, modo);
#endif
}

/* Tabela de tópicos descoberta na leitura (os arquivos têm 10). */
static char g_topicos[MAX_TOPICOS][32];
static int g_ntopicos = 0;

int topicos_total(void) { return g_ntopicos; }
const char *topico_nome(int i) { return (i >= 0 && i < g_ntopicos) ? g_topicos[i] : "?"; }

int popcount16(uint16_t x) {
    int c = 0;
    while (x) { x &= (uint16_t)(x - 1); c++; }
    return c;
}

int topicos_em_comum(uint16_t a, uint16_t b) { return popcount16((uint16_t)(a & b)); }

double jaccard(uint16_t a, uint16_t b) {
    int uniao = popcount16((uint16_t)(a | b));
    return uniao ? (double)popcount16((uint16_t)(a & b)) / uniao : 0.0;
}

/* Remove espaços nas pontas, no lugar. */
static char *aparar(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r' || s[n - 1] == '\n')) s[--n] = '\0';
    return s;
}

static int indice_topico(const char *nome) {
    for (int i = 0; i < g_ntopicos; i++)
        if (strcmp(g_topicos[i], nome) == 0) return i;
    if (g_ntopicos == MAX_TOPICOS) return -1;
    strncpy(g_topicos[g_ntopicos], nome, sizeof g_topicos[0] - 1);
    g_topicos[g_ntopicos][sizeof g_topicos[0] - 1] = '\0';
    return g_ntopicos++;
}

/* Divide uma linha CSV em até max campos, respeitando aspas ("Esportes, Saúde").
   Modifica a linha; devolve quantos campos achou. */
static int dividir_csv(char *linha, char **campos, int max) {
    int n = 0;
    char *p = linha;
    while (n < max) {
        if (*p == '"') {
            p++;
            campos[n++] = p;
            char *fim = strchr(p, '"');
            if (!fim) break;
            *fim = '\0';
            p = fim + 1;
            if (*p == ',') p++; else break;
        } else {
            campos[n++] = p;
            char *virg = strchr(p, ',');
            if (!virg) break;
            *virg = '\0';
            p = virg + 1;
        }
    }
    return n;
}

Usuario *carregar_usuarios(const char *caminho, int *n) {
    *n = 0;
    FILE *f = abrir_utf8(caminho, "r");
    if (!f) return NULL;
    int cap = 1024;
    Usuario *v = malloc((size_t)cap * sizeof *v);
    if (!v) { fclose(f); return NULL; }
    char linha[1024];
    int primeira = 1;
    while (fgets(linha, sizeof linha, f)) {
        if (primeira) { primeira = 0; continue; } /* cabeçalho */
        char *c[6];
        if (dividir_csv(linha, c, 6) < 6) continue;
        if (*n == cap) {
            cap *= 2;
            Usuario *nv = realloc(v, (size_t)cap * sizeof *v);
            if (!nv) { free(v); fclose(f); return NULL; }
            v = nv;
        }
        Usuario *u = &v[*n];
        u->id = (int)strtol(c[0], NULL, 10);
        strncpy(u->nome, aparar(c[1]), sizeof u->nome - 1); u->nome[sizeof u->nome - 1] = '\0';
        strncpy(u->usuario, aparar(c[2]), sizeof u->usuario - 1); u->usuario[sizeof u->usuario - 1] = '\0';
        u->postes = (int)strtol(c[3], NULL, 10);
        u->anos = strtod(c[4], NULL);
        u->topicos = 0;
        char *ctx = aparar(c[5]);
        for (char *tok = strtok(ctx, ","); tok; tok = strtok(NULL, ",")) {
            int t = indice_topico(aparar(tok));
            if (t >= 0) u->topicos |= (uint16_t)(1u << t);
        }
        (*n)++;
    }
    fclose(f);
    return v;
}

Amizade *carregar_amizades(const char *caminho, int n, int *m, int *ignoradas) {
    *m = 0; *ignoradas = 0;
    FILE *f = abrir_utf8(caminho, "r");
    if (!f) return NULL;
    int cap = 4096;
    Amizade *v = malloc((size_t)cap * sizeof *v);
    if (!v) { fclose(f); return NULL; }
    char linha[128];
    while (fgets(linha, sizeof linha, f)) {
        char *fim;
        long a = strtol(linha, &fim, 10);
        if (fim == linha || *fim != ',') continue; /* cabeçalho ou linha vazia */
        long b = strtol(fim + 1, NULL, 10);
        if (a < 1 || a > n || b < 1 || b > n || a == b) { (*ignoradas)++; continue; }
        if (*m == cap) {
            cap *= 2;
            Amizade *nv = realloc(v, (size_t)cap * sizeof *v);
            if (!nv) { free(v); fclose(f); return NULL; }
            v = nv;
        }
        v[*m].a = (int)a - 1;
        v[*m].b = (int)b - 1;
        (*m)++;
    }
    fclose(f);
    return v;
}
