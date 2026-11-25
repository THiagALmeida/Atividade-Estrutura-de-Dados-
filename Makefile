# gcc/clang (Linux, macOS, MinGW). No Windows com Visual Studio, use build.bat.
CC ?= cc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror
SRC = src/dados.c src/grafo_matriz.c src/grafo_lista.c src/bench.c src/analise.c

all: bin/rede bin/testes

bin/rede: $(SRC) src/main.c
	mkdir -p bin && $(CC) $(CFLAGS) $^ -o $@ -lm

bin/testes: $(SRC) tests/testes.c
	mkdir -p bin && $(CC) $(CFLAGS) $^ -o $@ -lm

asan: CFLAGS = -std=c11 -O1 -g -Wall -Wextra -fsanitize=address,undefined
asan: clean all

clean:
	rm -rf bin

.PHONY: all asan clean
