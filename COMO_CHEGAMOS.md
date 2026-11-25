# Como chegamos às conclusões

A regra que seguimos foi simples: **primeiro a conta, depois o programa, no fim a comparação**. Se a teoria previa um
número e o programa media o mesmo, a conclusão ficava. Se não batia, a gente ia atrás do porquê.

Para conferir tudo:

```bat
bin\rede.exe                       :: o que o programa em C mediu
python tools\verificar_teoria.py   :: as mesmas contas refeitas em Python, do zero
```

---

## 1. A rede é esparsa

Cada usuário é um vértice e cada amizade é uma aresta: $n = 10.000$ e $m = 19.996$ (o arquivo tinha 2 amizades repetidas).

Toda amizade é contada duas vezes quando somamos os graus, uma em cada ponta. É o **lema do aperto de mãos**:

$$\sum_{v \in V} \text{grau}(v) = 2m = 39.992 \quad\Rightarrow\quad \bar{k} = \frac{2m}{n} = \frac{39.992}{10.000} \approx 4$$

Cada pessoa tem uns 4 amigos. E quantas amizades caberiam na rede?

$$\text{densidade} = \frac{m}{\binom{n}{2}} = \frac{2m}{n(n-1)} = \frac{39.992}{99.990.000} \approx 0{,}0004 = 0{,}04\%$$

De cada 10 mil amizades possíveis, só 4 existem. **A rede é esparsa**, e isso decide todo o resto.

> No código: `gl_inserir` e `gm_inserir` descartam as repetidas; o benchmark confere que a soma dos graus dá 39.992 nas duas estruturas.

---

## 2. Lista ganha da matriz (menos em uma coisa)

Fizemos a conta antes de rodar. A matriz guarda uma célula para **cada par** de usuários, existindo amizade ou não. A lista
guarda **só o que existe**:

$$\text{Matriz: } n^2 = 10^8 \text{ bytes} \approx 95{,}4\ \text{MB} \qquad \text{Lista: } 12n + 16 \cdot 2m \approx 0{,}72\ \text{MB}$$

Para percorrer o grafo inteiro (BFS), a matriz olha todas as células e a lista olha só os vizinhos de verdade:

$$\frac{O(n^2)}{O(n + 2m)} = \frac{10^8}{10.000 + 39.992} \approx 2.000\times$$

A única operação em que a matriz deveria ganhar é perguntar "A e B são amigos?": nela é $O(1)$, um acesso direto; na lista
é $O(\text{grau})$, percorrer até 14 amigos.

**O que medimos** (média de 10 execuções):

| Operação | Matriz | Lista | |
|---|---|---|---|
| Construir | 25,7 ms | 3,8 ms | lista 6,8× |
| Grau de todos | 17,6 ms | 0,8 ms | lista 22× |
| 1.000.000 de "são amigos?" | **15,9 ms** | 25,1 ms | **matriz 1,6×** |
| BFS em toda a rede | 68,6 ms | 1,5 ms | lista 46× |
| Amigos em comum | 48,2 ms | 1,9 ms | lista 25× |
| Memória | 95,4 MB | 0,72 MB | lista 132× |

A memória bateu na vírgula, e a teoria acertou quem ganha em cada linha. Na BFS deu 46× em vez de 2.000× porque a
notação $O$ ignora constantes: a matriz lê memória contínua, que o processador puxa em blocos, enquanto a lista pula de
ponteiro em ponteiro e perde o cache. **A teoria dá a tendência; o hardware dá o tamanho.**

### Como medimos o tempo

Cada operação rodou 10 vezes. Usamos a média e o desvio-padrão:

$$\bar{x} = \frac{1}{r}\sum_{i=1}^{r} x_i \qquad s = \sqrt{\frac{1}{r-1}\sum_{i=1}^{r}(x_i - \bar{x})^2}$$

As duas estruturas receberam as mesmas entradas (semente fixa), a leitura dos arquivos ficou fora do tempo e o programa
confere que matriz e lista deram o mesmo resultado. O `clock()` do Windows tem resolução de ~1 ms, então tempos abaixo de
2 ms valem pela ordem de grandeza.

---

## 3. Quase todo mundo está conectado, a uns 7 passos

A BFS visita a rede em camadas: amigos, amigos de amigos e assim por diante. A camada em que alguém aparece é a sua
distância até quem começou:

$$d(u, v) = \text{menor número de amizades entre } u \text{ e } v$$

Rodando BFS a partir de cada usuário ainda não visitado, encontramos **182 componentes**. O maior tem 9.815 pessoas
(**98,2%**); quase todos os outros são os 177 usuários sem amigos.

Para a distância média, sorteamos 200 pessoas do componente grande e fizemos BFS a partir de cada uma:

$$\bar{d} = \frac{1}{|P|}\sum_{(u,v) \in P} d(u,v) \approx 6{,}72$$

É o famoso **mundo pequeno**: duas pessoas quaisquer estão, em média, a menos de 7 amizades de distância.

---

## 4. martin574 é o mais influente

Olhamos de dois jeitos.

**Pelo grau:** quem tem mais amigos. martin574 e kaitlyn261 empatam com 14.

**Pelo PageRank:** imagine alguém navegando pela rede, pulando de amigo em amigo e, de vez em quando (15% das vezes),
caindo num perfil qualquer. O PageRank é a fração do tempo que essa pessoa passa em cada usuário:

$$PR(v) = \frac{1 - d}{n} + d \sum_{u \in N(v)} \frac{PR(u)}{\text{grau}(u)}, \qquad d = 0{,}85$$

Isso é uma **cadeia de Markov**, e o PageRank é a sua distribuição estacionária. Calculamos pelo método da potência:
começamos com $PR = 1/n$ para todos e repetimos a fórmula até estabilizar:

$$\sum_{v} \left| PR^{(t+1)}(v) - PR^{(t)}(v) \right| < 10^{-10} \quad \Rightarrow \quad \text{parou em } t = 67$$

| # | Usuário | Grau | PageRank (× média) |
|---|---|---|---|
| 1 | martin574 | 14 | 3,11 |
| 2 | kaitlyn261 | 14 | 2,98 |
| 3 | marc167 | 13 | 2,74 |

Os dois critérios concordam porque, numa rede de amizades (sem direção), o passeio aleatório passa em cada pessoa na
proporção do seu grau:

$$\pi(v) = \frac{\text{grau}(v)}{2m}$$

Por isso a correlação entre grau e PageRank deu **0,99**. O PageRank serviu de desempate: os 14 amigos de martin574 são
mais bem conectados que os de kaitlyn261.

---

## 5. Postar mais não dá mais amigos

Usamos a **correlação de Pearson**, que vai de −1 a +1:

$$r = \frac{\sum_i (x_i - \bar{x})(y_i - \bar{y})}{\sqrt{\sum_i (x_i - \bar{x})^2}\ \sqrt{\sum_i (y_i - \bar{y})^2}}$$

Grau × postes deu $r = 0{,}001$; grau × tempo na rede deu $r = -0{,}001$. Praticamente zero: **nenhuma relação**.

---

## 6. Amigos não têm mais interesses em comum que desconhecidos

Os tópicos de cada pessoa viraram um **conjunto**. Para medir o quanto dois conjuntos se parecem, usamos o **índice de Jaccard**:

$$J(A, B) = \frac{|A \cap B|}{|A \cup B|}$$

Exemplo: $\{\text{Música}, \text{Moda}\}$ e $\{\text{Moda}, \text{Jogos}\}$ dão $J = \frac{1}{3} \approx 0{,}33$.

Ver que 37,1% das amizades têm um tópico em comum não diz nada sozinho. Precisávamos saber **quanto daria por acaso**.
Então sorteamos 200.000 pares de desconhecidos e medimos a mesma coisa:

| | Amigos | Ao acaso |
|---|---|---|
| Ao menos 1 tópico em comum | 37,1% | 36,9% |
| Jaccard médio | 0,132 | 0,130 |

A diferença é de 0,2 ponto percentual. É muito ou pouco? O **erro-padrão** diz quanto essa porcentagem varia só por sorte:

$$EP = \sqrt{\frac{p(1-p)}{m}} = \sqrt{\frac{0{,}371 \times 0{,}629}{19.996}} \approx 0{,}0034 = 0{,}34 \text{ p.p.}$$

$$\frac{0{,}2}{0{,}34} \approx 0{,}6 < 1$$

A diferença é menor que um erro-padrão: está dentro do acaso. **Não há homofilia** nesta rede.

---

## 7. A base foi gerada ao acaso

Essa foi a descoberta mais interessante. No modelo de **Erdős–Rényi**, cada par de pessoas vira amigo com a mesma
probabilidade $p$, como num sorteio. Com os nossos números:

$$p \approx 0{,}0004 \qquad \lambda = n \cdot p \approx 4$$

A teoria desse modelo prevê vários números. Comparamos:

| O que | Fórmula | Previsto | Medido |
|---|---|---|---|
| Sem amigos | $n \cdot e^{-\lambda}$ | 183 | **177** |
| Maior componente | $S = 1 - e^{-\lambda S}$ | 98,0% | **98,2%** |
| Distância média | $\dfrac{\ln n}{\ln \lambda} = \dfrac{\ln 10.000}{\ln 4}$ | 6,64 | **6,72** |
| Triângulos | $\binom{n}{3} p^3$ | 10,7 | **7** |

Os 7 triângulos, contra 10,7 previstos, estão dentro do normal: a chance de sair 7 ou menos é de cerca de 17%.

No sorteio puro, o número de amigos de cada pessoa segue uma **distribuição de Poisson**:

$$P(\text{grau} = k) = \frac{e^{-\lambda}\lambda^k}{k!}$$

| Grau | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Medido | 177 | 710 | 1532 | 1997 | 1874 | 1506 | 1076 | 611 | 308 | 131 | 50 | 16 | 9 | 1 | 2 |
| Poisson | 183 | 733 | 1466 | 1954 | 1954 | 1563 | 1042 | 595 | 298 | 132 | 53 | 19 | 6 | 2 | 0,6 |

Para não ficar só no olho, aplicamos o **teste qui-quadrado** (graus 0 a 10):

$$\chi^2 = \sum_{k} \frac{(O_k - E_k)^2}{E_k} = 12{,}3 \;<\; \chi^2_{9;\,0{,}05} = 16{,}9$$

São 11 classes, menos 1, menos o $\lambda$ que estimamos dos dados: 9 graus de liberdade. Como 12,3 ficou abaixo de 16,9,
os graus seguem a Poisson.

Redes sociais de verdade não são assim: têm muitos triângulos (amigos dos meus amigos viram meus amigos), têm "famosos"
com milhares de conexões e têm homofilia. Esta não tem nada disso. **Conclusão: os dados são sintéticos.** O método está
certo e serve para qualquer rede, mas aqui os resultados não mostram comportamento social real.

---

## 8. Sugestões de amizade

Usamos o **fechamento triádico**: se A é amigo de B e B é amigo de C, A e C tendem a virar amigos. Para cada pessoa,
olhamos os amigos dos amigos e damos uma nota a cada candidato:

$$\text{nota}(u, w) = \big(\ |N(u) \cap N(w)|\ ,\ J(\text{tópicos}_u, \text{tópicos}_w)\ \big)$$

Primeiro conta quantos amigos em comum eles têm; no empate, quem tem interesses mais parecidos. Como quase não há
triângulos, o empate é a regra, e o Jaccard é que decide. Exemplo: kaitlyn261 → lisa457 (1 amigo em comum, $J = 0{,}67$).
Ao todo, 9.815 pessoas receberam sugestões.

---

## Resumo

| Conclusão | De onde veio |
|---|---|
| Rede esparsa | Lema do aperto de mãos e densidade |
| Lista > matriz | Notação $O$: $O(n^2)$ contra $O(n+m)$, confirmado no benchmark |
| Matriz ganha em "são amigos?" | $O(1)$ contra $O(\text{grau})$ |
| Mundo pequeno | BFS e média de distâncias |
| martin574 lidera | Grau + PageRank (cadeia de Markov) |
| Postar não dá amigos | Pearson ≈ 0 |
| Sem homofilia | Jaccard + sorteio de 200 mil pares + erro-padrão |
| Dados sintéticos | Erdős–Rényi, Poisson e qui-quadrado |
| Sugestões | Fechamento triádico + Jaccard |
