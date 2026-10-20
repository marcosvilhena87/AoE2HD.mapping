# Villager Carry Capacity Discovery v1 — proposta de validação

**Campo investigado:** `Object+0x48` `float32`. **Status: hipótese.**
Evidência anterior: `docs/identificacao_objetos_mundo_v9_2_2026-10-18.md`, protocolos v9.8 e v9.9, nos quais fontes de recursos diminuíram enquanto objetos coletores candidatos aumentaram em quantidades quase iguais. A mesma posição relativa nos objetos não prova que seja capacidade máxima.

No controlador `AoE2HD.control`, o modo `--discover-villager-carry-capacity-v1` lê um aldeão local por ID estável, verifica classe 4, captura o valor bruto e suas diferenças temporais, o maior valor observado em uma sequência válida, quedas candidatas a depósito e falhas de leitura. O rótulo de fase e recurso é informado pelo operador; nenhum limite máximo é presumido.

## Critérios de validação futura
1. Registrar ciclo completo vazio → coleta → transporte → depósito, verificando simultaneamente o estoque do jogador e a fonte.
2. Mostrar que um platô precede o retorno ao depósito, sem ordens manuais adicionais, em múltiplos ciclos.
3. Repetir com madeira, alimentos, ouro e pedra, e testar tecnologias que alterem transporte, se aplicáveis.
4. Repetir após reiniciar o jogo, mantendo identificação individual e controles negativos.
5. Diferenciar **maior valor observado na captura** de **capacidade máxima efetiva**: os dois não são equivalentes.

Registro adicionado sem promover o endereço a validado.
## Captura real `carry_wood.csv` — sessão `carga01` (2026-10-20)

Monitor v1 do `AoE2HD.control`, fase manual `other`, recurso rotulado `wood`. **180/180 leituras válidas**, sem erros. Objeto `2174`, Genie Type ID `123`, classe `4`, ponteiro do objeto `0x14074FC0` estável na captura.

O candidato `Object+0x48` `float32` atingiu **10.0** nas amostras **43–44, 90–95, 133–138 e 177–180**. Ocorreram três reduções abruptas de **10.0 para 0.0** nas amostras **45, 96 e 139**, todas sinalizadas por `peak_candidate=1` e `reset_candidate=1`. A quarta sequência terminou em 10.0 antes do fim da observação. No início da captura, o valor já era aproximadamente 0.39897, portanto o primeiro ciclo começou parcialmente em andamento.

**Conclusão limitada:** há evidência temporal forte de acúmulo de carga até limite observado de 10.0 e descarregamentos candidatos repetidos para esse aldeão ao coletar madeira. A capacidade máxima universal ou formal do jogo NÃO está validada; uma queda não confirma depósito sem verificação simultânea do estoque do jogador. `Object+0x48` permanece **hipotese** até controles com madeira do jogador, fonte, vários aldeões, reinício e tecnologias. Não interpretar `max_observed_in_run` isoladamente como parâmetro de capacidade.

## Protocolo de validação v2 — carga individual versus estoque do jogador

Disponibilizado no `AoE2HD.control`: `--validate-villager-carry-capacity-v2`. O experimento sincroniza leituras do candidato `Object+0x48` (float32) de um aldeão local de classe 4 com `ResourceTable+0x00` alimento, `+0x04` madeira, `+0x08` pedra ou `+0x0C` ouro. Uma leitura de estoque antes e depois da carga detecta alterações durante a amostra.

Um evento é marcado `matched_deposit_candidate` apenas quando a queda da carga e o aumento do estoque, no mesmo intervalo, são numericamente compatíveis dentro da tolerância. Sem isolamento do aldeão e sem controles de outras fontes/consumos, a correspondência **não comprova causalidade**. O máximo observado não é parâmetro de capacidade universal; status de `Object+0x48` continua `hipotese`, não `validado`. Capturas reais v2 ainda pendentes.

## Validação experimental v2 — `carry_wood_v2.csv` (captura real)

Fonte: arquivo fornecido pelo operador, sessão `deposito01`, recurso `wood`; 180 amostras, 180 leituras `ok`, zero erros, aldeão ID individual `2174`, Genie Type ID `123`, ponteiro `0x14074FC0` constante. `capacity_validated=no` no relatório.

Monitorados em conjunto `Object+0x48` (float32, carga candidata) e `ResourceTable+0x04` (estoque de madeira do jogador local). Resultados:

| Amostra | Delta Object+0x48 | Delta ResourceTable+0x04 | Classificação v2 |
|---:|---:|---:|---|
| 51 | -10.0 | +10.0 | `matched_single_sample` |
| 94 | -10.0 | +10.0 | `matched_single_sample` |
| 138 | -10.0 | +10.0 | `matched_single_sample` |

Estoque inicial `314.994720`, final `344.994720`: aumento acumulado **+30.0**, correspondente às três reduções individuais de carga. Em cada um dos três casos, carga caiu de 10.0 para 0.0. Quatro sequências de carga chegaram a 10.0 (amostras 45–50, 88–93, 131–137 e 175–180); a última terminou sem novo depósito capturado. O relatório contém exatamente três `matched_deposit_candidate=1` e zero outras quedas de carga. Foram 180/180 leituras estáveis.

**Interpretação:** evidência funcional e temporal forte de `Object+0x48` como quantidade carregada por esse aldeão durante madeira, com limite observado repetido de 10.0; eventos são *depósitos candidatos numericamente correlacionados*, não comprovação de causa exclusiva. Sem controle integral de outros trabalhadores e intervenções no estoque, não declarar causalidade provada. Não confundir quantidade observada com capacidade máxima universal, bônus e melhorias ainda não testados. Manter o offset `Object+0x48` com status `hipotese` no catálogo enquanto forem pendentes controles negativos, replicação em nova partida e outros recursos.

## Validação experimental v2 — `carry_food_v2.csv` (alimentos)

Captura fornecida pelo operador: `session=deposito01`, `resource=food`, 180/180 leituras `ok`, nenhum erro. Aldeão `object_id=2174`, `genie_type_id=120` (coletor), ponteiro estável `0x14074FC0`. A quantidade candidata `Object+0x48` atingiu **10.0** e três transições tiveram `delta_carried=-10.0`, `delta_stock=+10.0`, `matched_deposit_candidate=1`, `match_reason=matched_single_sample`:

| Amostra | Carga anterior → atual | Estoque antes → depois | Correspondência |
|---:|---|---|---|
| 57 | 10.0 → 0.0 | 200.0 → 210.0 | exata |
| 117 | 10.0 → 0.0 | 210.0 → 220.0 | exata |
| 178 | 10.0 → 0.0 | 220.0 → 230.0 | exata |

Estoque total de alimentos cresceu de 200 para 230 (+30), em três correspondências. Igual ao experimento v2 com madeira, o maior valor de carga observado foi 10.0, sem outras quedas de carga. Indício forte de `Object+0x48` como carga corrente do aldeão também para alimento. Não prova capacidade máxima universal nem causalidade exclusiva do aldeão. Permanecem pendentes ouro, pedra, replicação em nova partida e controles de tecnologias e outros trabalhadores; campo continua com status `hipotese`.

## Validação experimental v2 — `carry_gold_v2.csv` (ouro)

Arquivo fornecido pelo operador: `session=deposito01`, `resource=gold`, 180 amostras, 180 `read_status=ok`, sem erros. Objeto individual `2174`, Genie Type ID `579` (garimpeiro), ponteiro `0x14074FC0` estável. O arquivo v2 não inclui campo `class_id`.

Foram encontrados **dois eventos correlacionados** (`matched_deposit_candidate=1`, `match_reason=matched_single_sample`):

| Amostra | Carga Object+0x48 | Estoque ResourceTable+0x0C | Delta de carga/estoque |
|---:|---|---|---|
| 68 | 10 → 0 | 100 → 110 | -10 / +10 |
| 139 | 10 → 0 | 110 → 120 | -10 / +10 |

Picos de 10.0 nas amostras 49–67 e 120–138. Ao final (amostra 180), terceiro ciclo com carga ~6.72943; estoque inicial 100.0 e final 120.0 (+20), sem outras quedas detectadas. A medição fortalece `Object+0x48` como candidato a carga transportada e **10.0 como limite observado de ouro nesta configuração**. Não confundir com capacidade máxima geral ou depósito causalmente atribuído ao único aldeão sem controles de outros produtores/consumidores.

Quadro acumulado de evidências v2: madeira 3 eventos de -10/+10; alimento 3; ouro 2; pedra ainda pendente. Preservar status `hipotese` para `Object+0x48`; `capacity_validated=no`.

## Validação experimental v2 — `carry_stone_v2.csv` (pedra)

Arquivo do operador: `session=deposito01`, `resource=stone`, 180/180 amostras `read_status=ok`, nenhuma falha. Object ID `2174`, ponteiro `0x14074FC0`, variante Genie `124` (mineiro de pedra); `capacity_validated=no`.

Uma correspondência de queda da carga e aumento do estoque do jogador ocorreu na **amostra 93**, com `Object+0x48` **10.0→0.0** (`delta_carried=-10.0`) e `ResourceTable+0x08` **200.0→210.0** (`delta_stock=+10.0`), `matched_deposit_candidate=1`, `match_reason=matched_single_sample`. A carga chegou a 10.0 nas amostras **65–92** e **154–180**. A segunda sequência terminou sem queda observada; início com carga 0, primeira carga positiva na amostra 32. Estoque só mudou uma vez (+10), nenhuma outra queda de carga.

### Consolidação v2, quatro recursos (capturas do operador)

| Recurso | Limite observado `Object+0x48` | Depósitos candidatos de -10/+10 | Soma candidata |
|---|---:|---:|---:|
| Madeira | 10.0 | 3 | 30.0 |
| Alimento | 10.0 | 3 | 30.0 |
| Ouro | 10.0 | 2 | 20.0 |
| Pedra | 10.0 | 1 | 10.0 |
| **Total** | — | **9** | **90.0** |

A repetição em quatro recursos sustenta fortemente `Object+0x48` como campo de carga atual em aldeões dessa configuração de partida e **10.0** como limite observado. Não demonstra capacidade máxima universal ou invariância por tecnologias/unidades, nem isola causalmente cada depósito de outros atores. Próximos controles: nova partida, múltiplas unidades, tecnologias e fontes/estoques simultâneos; manter `hipotese` no catálogo até esses controles.

## Validation v3 — protocolo tecnológico (implementado, aguardando captura)

O `AoE2HD.control` possui `--validate-villager-carry-capacity-v3`, que reaproveita a captura sincronizada v2 e anota rótulo **manual** de fase tecnológica (`none`, `wheelbarrow`, `hand_cart`, `heavy_plow` ou `other`), civilização, recurso, sessão e repetição. `--compare-villager-carry-v3` consolida CSVs e compara máximos observados do **mesmo recurso e civilização**, com diferenças nominais; não comprova tecnologia na memória e não confere capacidade máxima por um pico isolado.

Verificações futuras: linha de base e após Wheelbarrow/Hand Cart em ciclos completos e consecutivos, efeito de Heavy Plow exclusivamente sob condição pertinente, controle de civilizações e outras fontes de recursos, replicação em sessões novas. Não há novos endereços de memória identificados pelo v3; `Object+0x48` permanece `hipotese` até evidência independente. Resultados reais v3 ainda não apresentados.

## V3 — linha de base declarada sem tecnologia: `carry_wood_none_v3.csv`

Captura fornecida pelo operador; `session=pesquisa01`, `trial_id=antes`, `technology_label=none`, `civilization_label=bretoes`, `resource=wood`. Total 180/180 leituras `ok`, zero falhas. Object ID `2174`, ponteiro estável `0x15D1C0A0`. Variantes Genie `120` (7 amostras) e `123` (173 amostras). Máximo de `carried_48` = **10.0**.

| Amostra | Δ carga Object+0x48 | Δ estoque ResourceTable+0x04 | Estoque após | Classificação |
|---:|---:|---:|---:|---|
| 48 | -10.0 | +10.0 | 9910.0 | matched_single_sample |
| 84 | -10.0 | +10.0 | 9920.0 | matched_single_sample |
| 120 | -10.0 | +10.0 | 9930.0 | matched_single_sample |
| 156 | -10.0 | +10.0 | 9940.0 | matched_single_sample |

Estoque de madeira inicial **9900.0**, final **9940.0**, variação total +40.0. A linha de base é consistente com o limite observado 10 para madeira sem melhorias *declaradas*. O nome da tecnologia é metadado manual, não confirmação de pesquisas pela memória. Este resultado não valida por si só capacidade máxima como atributo estático nem os efeitos de Wheelbarrow / Hand Cart; tais capturas ainda são necessárias. `Object+0x48` permanece hipótese.

## V3 — após Wheelbarrow: `carry_wood_wheelbarrow_v3.csv`

Captura real do operador `session=pesquisa01`, `trial_id=apos_wheelbarrow`, `technology_label=wheelbarrow` (declaração manual), `civilization_label=bretoes`, `resource=wood`. Total de **180/180** leituras `ok`, zero erros. Unidade individual `2174`, variante Genie `123`, ponteiro `0x15D1C0A0` estável. O candidato `Object+0x48` chegou a **13.0**, nas amostras 53–55, 99–100, 143–145, antes de três quedas completas:

| Amostra | Delta carga Object+0x48 | Delta estoque ResourceTable+0x04 | Estoque depois |
|---:|---:|---:|---:|
| 56 | -13.0 | +13.0 | 19913.458984 |
| 101 | -13.0 | +13.0 | 19926.458984 |
| 146 | -13.0 | +13.0 | 19939.458984 |

Todas sinalizadas `matched_deposit_candidate=1` / `matched_single_sample`; estoque de 19900.458984 até 19939.458984, delta total **+39.0**. Amostra 180 encerrou em 10.395076 unidades de carga, quarto ciclo sem depósito capturado.

**Comparação com v3 `none` da mesma civilização e mesmo recurso:** limite observado 10.0 → 13.0, diferença **+3.0 (+30%)**, com 4 eventos anteriores de -10/+10 e 3 eventos posteriores de -13/+13. Interpretação: forte evidência de efeito de Wheelbarrow sobre a carga atual e o limite observado nesta configuração, embora o rótulo de tecnologia seja manual e o endereço do estado de pesquisa não tenha sido identificado. `capacity_validated=no` no relatório; capacidade interna universal permanece não mapeada, e o campo `Object+0x48` mantém seu status cadastral de hipótese até controles independentes.
