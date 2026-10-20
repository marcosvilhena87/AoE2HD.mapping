# Evidência experimental: estado de atividade dos aldeões — v5 (2026-10-20)

## Proveniência
Capturas locais fornecidas pelo operador, sessão `teste01`, Age of Empires II HD 5.8.INT, monitor somente leitura `AoE2HD.control --recognize-villager-activity-v5`:
`activity_idle.csv`, `activity_wood.csv` e `activity_moving.csv`.

## Campo em investigação
`Object+0x78 -> Object78Target+0x94` (`uint32`), já existente no catálogo de estruturas como **hipótese**, sem validação de semântica de ordem.

## Observações controladas
Os três aldeões identificados por Object IDs `2172`, `2173`, `2174` mantiveram os mesmos ponteiros nas três fases e classe `4`.
Cada fase continha 30 amostras por aldeão (90 leituras bem-sucedidas):

| Fase rotulada pelo operador | 2172 | 2173 | 2174 | Genie Type ID (2172/2173/2174) |
| --- | --- | --- | --- | --- |
| idle | 0 (30/30) | 0 (30/30) | 0 (30/30) | 293/293/83 |
| wood | 1 (30/30) | 1 (30/30) | 1 (30/30) | 218/218/123 |
| moving | 2 (30/30) | 2 (30/30) | 2 (30/30) | 293/293/83 |

Movimento entre intervalos comparáveis (29 por aldeão, por fase): idle `0/29,0/29,0/29`; wood `7/29,12/29,5/29`; moving `29/29,29/29,29/29`.

Em cada fase, os objetos `2245–2248` apresentaram falha de leitura do ponteiro `Object+0x78` (120 registros de erro por fase). Não são classificados como aldeões pela evidência disponível.

## Interpretação e limitações
Associação **experimental de sessão única**: `0` com repouso, `1` com coleta de madeira, `2` com deslocamento simples. O valor 1 não foi demonstrado para outros trabalhos, nem o valor 2 para todas as ordens de deslocamento. Fases são rótulos do operador; resultados não provam causalidade ou semântica da ordem. O campo não deve ser promovido a validado por esses dados.

Próximos controles: fases `gold`, `stone`, `berries`, `build`, interrupção/cancelamento de ação, coleta versus transporte/deposição, e repetição após reiniciar o jogo. Investigar ponteiro de ordem e alvo somente após coleta de evidências.

## Captura gold — complemento (2026-10-20)
Arquivo fornecido pelo operador: `activity_gold.csv`, sessão `teste01`, 30 amostras por aldeão, 90 leituras válidas. Object IDs `2172`, `2173`, `2174`, classe `4`, mesmos ponteiros individuais durante a coleta; tipos Genie `581,581,579` (garimpeiros). O campo `Object78Target+0x94` ficou em `3` nas amostras 1–16 e passou a `4` nas amostras 17–30, **simultaneamente para todos os três IDs**: 48 leituras com 3 e 42 com 4. Movimento entre 29 intervalos: `9,17,11` por aldeão. Outros objetos `2245–2248`: 120 erros de leitura de `Object+0x78`.

**Ressalva crítica:** a sequência conjunta `0` (idle), `1` (wood), `2` (moving), `3/4` (gold) e especialmente a transição sincronizada `3→4` indicam que o campo pode ser contador global/compartilhado (ou da definição), não ordem ou atividade específica de cada objeto. As fases foram executadas sequencialmente e não isolam tempo global versus atividade. Não mapear valores 0–4 para profissões nem promover o campo. Controle prioritário: observar simultaneamente aldeões diferentes em tarefas distintas e comparar os valores/pointers `Object+0x78`, a estrutura alvo e `+0x94` por objeto, além de controles sem novas ordens e após reinício.

## Teste simultâneo de atividades distintas — activity_other.csv

Captura da sessão `teste01` com 30 amostras simultâneas e 3 aldeões locais identificados (90 leituras válidas). Os valores `Object78Target+0x94` permaneceram constantes em cada indivíduo nas 30 amostras:

| object_id | genie_type_id | rótulo de variante | +0x94 | diferenças de posição entre 29 intervalos |
| --- | --- | --- | --- | --- |
| 2172 | 218 | Lenhador | 7 | 9 |
| 2173 | 581 | Garimpeiro | 6 | 2 |
| 2174 | 83 | Aldeão | 5 | 0 |

Todos classe 4; ponteiros de objetos estáveis. Outros Object IDs `2245–2248` produziram 120 erros de ponteiro, não classificados como aldeões.

**Revisão da interpretação:** a presença simultânea de valores 7, 6 e 5 contradiz a hipótese simplista de um *único valor global idêntico* para todos os objetos em cada instante. Entretanto, a sequência observada nas capturas anteriores (0,1,2,3→4) ainda sugere uma possível variável de contador, geração, estrutura de definição compartilhada ou outro mecanismo sem relação causal direta com ordem ativa. Os rótulos de trabalho derivam do Genie Type ID, e não de ordem/target comprovados; o valor não deve ser mapeado para atividade, sequer provisoriamente como enumeração de ordens, sem novos controles.

**Próximas provas:** guardar o ponteiro completo `Object+0x78` por aldeão, comparar igualdade/divergência de definição, monitorar valor bruto durante transições controladas de UM aldeão enquanto outros ficam imóveis, repetir comandos idênticos e inverter a ordem das fases. Repetir em nova sessão após reiniciar o jogo. Estado segue `hipotese`.

## Teste controlado v5.1 — activity_v5_1_wood.csv

Captura fornecida pelo operador: sessão `teste01`, 60 amostras (180/180 leituras `ok`). Na própria captura os papéis registrados são **2174 foco**, 2173 e 2172 controles — diferente do planejamento anterior de focar 2172. Os três objetos têm ponteiros de definição distintos e estáveis *dentro desta captura*:

| Object ID | Papel | Object+0x78 alvo (sessão) | +0x94 | Observação |
|---|---|---|---|---|
| 2174 | focus | 0x143FBD98 | 6 (amostras 1–8) → 7 (9–60) | uma mudança individual |
| 2173 | control | 0x143FBF50 | 7 constante | controle sem mudança |
| 2172 | control | 0x143FBA28 | 8 constante | controle sem mudança |

Os valores `delta_94` vieram vazios mesmo após a primeira amostra; é necessário revisar a comparação temporal da instrumentação antes de usar indicadores automáticos. `shared_definition=0` para todos.

**Interpretação limitada:** a transição individual é compatível com mudança de estado associada à unidade foco, mas também pode ser um contador individual/evento; não comprova que 7 signifique cortar madeira. Os endereços de definição informados na captura idle anterior eram outros, de modo que não se deve assumir estabilidade *entre arquivos* sem conferir a identidade e a sessão de memória. Confirmar que a unidade 2174 recebeu a ordem pretendida; reproduzir a transição com outra unidade e ordem invertida após reinício. O catálogo permanece com status `hipotese`.
