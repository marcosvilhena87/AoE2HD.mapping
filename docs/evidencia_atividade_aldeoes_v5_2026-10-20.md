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
