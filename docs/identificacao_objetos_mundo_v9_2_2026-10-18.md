# World Object Identification v9.2 — resultados reais

Experimento no AoE2 HD somente leitura, arquivos `world_object_identification_v9_2.csv` e `world_object_identification_v9_2_groups.csv`, faixa IDs [1,4097).

## Panorama

- 2.195 objetos `unsupported_layout`, agrupados em 24 tuplas distintas (owner, ponteiro de tipo, ponteiro de definição).
- 12 amostras automáticas representativas, mais cinco controles guiados, todos `captured` sem erros no CSV.
- O grupo predominante possui **1.962 entradas** (89,4% dos incompatíveis): owner `0x10643344`, `Object+0x0C=0x103860F0`, `Object+0x78=0`. Exame do alvo `+0x10` como uint16: 351.
- Os ponteiros são endereços temporários desta sessão, não offsets estáticos.

## Controles guiados (rótulos visuais declarados pelo operador)

| Rótulo | Object ID | Owner | Object+0x0C | Object+0x78 | Target+0x10 uint16 |
|---|---:|---|---|---|---:|
| tree | 2115 | 0x10643344 | 0x103860F0 | 0 | 351 |
| animal | 2075 | 0x14A92AE4 | 0x1912EEE8 | 0 | 594 |
| gold | 2030 | 0x10643344 | 0x10724420 | 0 | 66 |
| stone | 2059 | 0x10643344 | 0x10724510 | 0 | 102 |
| relic | 2013 | 0x10643344 | 0x1459C318 | 0x14A0BE58 | 285 |

Os cinco nomes são identificações manuais do que foi selecionado; a ferramenta não valida a aparência. Tipo +0x12 também coincidiu com +0x10 nestes cinco controles, mas NÃO representa garantia geral: na v8 +0x12 divergiu em construções.

## Amostragem automática

Exemplos de alvos da leitura alternativa:
- Grupo de 1.962: ID 70, tipo candidato 351;
- Grupo de 51: ID 2213, tipo candidato 458;
- Grupo de 36: ID 2026, tipo candidato 66;
- Grupo de 26: ID 68, tipo candidato 709;
- Grupo de 25: ID 2264, tipo candidato 69;
- Grupo de 23: ID 2, tipo candidato 264;
- Grupo de 21: ID 2056, tipo candidato 102;
- Grupo de 12: ID 2201, tipo candidato 455.

O controle tree (351) coincide com o grupo de 1.962; gold (66) com o grupo de 36; stone (102) com o grupo de 21. Ainda não há nomenclatura validada dos outros grupos.

## Interpretação

A presença de `Object+0x78=0` não impede que `Object+0x0C -> Target+0x10` seja legível em objetos do cenário. O status `unsupported_layout` da v9.1 significa layout incompatível com a leitura conjunta do tipo e classe, não ausência de ID específico.

O ponteiro `0x10643344` concentra muitos objetos rotulados árvore, ouro, pedra e relíquia: candidato a Gaia/owner ambiental, **não validado**. O animal teve outro owner igual ao observado como local na v9.1, compatível com controle por jogador. Não assumir que todas as árvores são tipo 351 nem que todos os animais são 594 antes de novos controles e sessões.

Próximo aprimoramento: separar detecção de **tipo** e **classe** para que `Object+0x78=0` não seja classificado como falha total; manter campo class_id como ausente, identificar tipo independentemente e rotular os owners por controles. Preservar todos os valores desconhecidos sem criar equivalências sem validação adicional.

## World Object Identification v9.3 — protocolo de atributos

Novo experimento no AoE2HD.control: `--identify-world-attributes-v9-3`. Coleta snapshots somente leitura, limitados a 128–512 bytes, para animal vivo, abatido, em coleta, quadrado/armadilha de peixes e cardume antes/durante coleta. A captura do objeto selecionado nao exige HP, pois ha fontes de alimento sem HP. Captura ID individual, owner, type_id (`Object+0x0C -> +0x10`), classe quando aplicavel e dumps de objeto/definicao de tipo. Campos HP atual/maximo e alimento restante/inicial sao gravados vazios com `not_mapped` ate demonstracao experimental. Ausencia de valor NAO e HP=0; ausencia de estrutura de classe nao e ausencia de tipo. Esta etapa nao acrescenta offsets validados nem atribui quantidade de alimento automaticamente. Capturas experimentais e comparacao temporal pendentes.

## World Object Identification v9.3 — primeiras capturas reais

CSV `world_object_attributes_v9_3.csv` analisado. Sete fases: seis `captured`, uma `read_error` (animal/gathering: `Selecao unica ausente; objeto ambiental pode nao ser selecionavel`). Animal vivo e abatido mantiveram ID individual 2075, tipo 594: `Object+0x34` float32 passou de 7.0 para 0.0 (candidato FORTE a HP atual); `Object+0x48` float32 passou de 100.0 para 97.07219696 (candidato experimental a alimento restante, SEM validacao). Owner do mesmo objeto mudou de 0x14A92AE4 (vivo) para 0x10643344 (abatido); semantica precisa de replicacao. `Object+0x78=0` no animal, logo classe nao disponivel pela cadeia convencional.

Peixes: fish_trap/before capturou ID 2382 tipo 199; fish_trap/gathering capturou ID 2377 tipo 13. fish_school/before capturou ID 2234 tipo 458; fish_school/gathering capturou novamente ID 2377 tipo 13. As comparacoes antes/durante NAO referem o mesmo objeto e nao demonstram consumo de alimento; tipo 13 pode corresponder ao barco selecionado, ainda nao identificado por controle independente. O teste futuro deve permitir leitura por ID indexado na tabela World+0xE8, sem depender de selecao, e repetir o MESMO ID durante a coleta. Nao promover +0x34 ou +0x48 a validado com uma unica transicao.

## World Object Attributes v9.4 — protocolo publicado

Novo modo do controlador: `--identify-world-attributes-v9-4`, captura somente leitura por ID individual na tabela indexada `World+0xE8`, sem exigir selecao ativa de animal/carcaca/armadilha/cardume. Reaproveita o MESMO ID entre estados da mesma categoria e rejeita tentativa de trocar identificador em comparacao; compara valores candidatos float32 `Object+0x34` e `Object+0x48` e conserva dump de ate 512 bytes. ID da entrada e objeto devem coincidir. Campos sao **candidatos**, nao HP/recurso automaticamente validado, sobretudo para fontes sem HP; resultados reais e controles adicionais pendentes. IDs 2075 (animal), 2382 (fish_trap) e 2234 (fish_school) sao apenas exemplos da sessao v9.3, nao IDs fixos.

## World Object Attributes v9.4 — cardume comparado por ID (captura real)

No CSV `world_object_attributes_v9_4(2).csv`, foram puladas as cinco etapas de animal/armadilha e capturado o MESMO cardume nas duas fases: objeto ID 2234, type_id=458, Object=0x147F7A58, owner=0x158E9014, type_pointer=0x14CE4F98, definition_pointer=0. Status `captured` nas duas, `same_id_candidate`, **zero erros**. Os ponteiros e tipo permaneceram iguais.

Valores float32: `Object+0x48` antes=205.8340301513672, durante coleta=202.19786071777344, delta=-3.63616943359375 (CSV arredonda -3.636169). Campo `Object+0x34` permaneceu 0.0 nas duas. Isso reforca fortemente a HIPOTESE de `Object+0x48` como recurso/alimento restante para cardume tipo 458, mas NAO valida automaticamente a interpretacao para todos os objetos. `Object+0x34=0.0` nao significa HP presente nem prova de ausencia. Prosseguir com replicacao temporal e outros cardumes antes da promocao de status. Enderecos absolutos sao da sessao.

## World Object Attributes v9.5 — separacao HP/recurso (implementacao)

No controlador o modo `--identify-world-attributes-v9-5` preserva a captura pelo mesmo ID da v9.4 e a leitura read-only. Os campos `hp_34_raw_candidate` e `resource_48_raw_candidate` mantem os floats observados, sem interpretacao obrigatoria. Segundo o modelo informado pelo operador, cardumes (`fish_school`) e animais mortos/em coleta (`animal/dead` ou `animal/gathering`) tem `hp_status=not_applicable` e `hp_current` vazio; animal vivo tem HP candidato; armadilha de pesca tem HP desconhecido. Recurso alimenticio restante em +0x48 permanece candidato. Deltas de floats brutos sao registrados mesmo quando HP nao e aplicavel. A categoria/fase e declaracao do operador, nao comprovacao visual automatica. Nenhum novo offset promovido a validado, tampouco HP=0 confundido com nao aplicavel. Captura e testes locais no Windows pendentes.

## Correcao World Object Attributes v9.5.1 — HP em Armadilhas de Pesca

Informacao funcional fornecida pelo operador: Armadilhas de Pesca possuem **HP aplicavel** e quantidade de alimento disponivel, como atributos distintos. O controlador foi corrigido para informar `hp_status=candidate` em `fish_trap/before` e `fish_trap/gathering` e mostrar `hp_current` candidato quando `Object+0x34` for legivel. `Object+0x48` permanece candidato a alimento restante. A existencia do atributo HP na armadilha nao valida o offset +0x34; confirmar por variacao controlada do HP. Cardumes e carcacas permanecem `not_applicable` para HP. Nenhum novo endereco de memoria foi validado nesta correcao.

## Descoberta de objetos pelo tipo — protocolo de automacao

Implementado no AoE2HD.control `--discover-world-objects-v9-6` (varredura automaticamente pela tabela `World+0xE8`) e opcao `--world-attributes-v9-5-auto` no capturador de atributos, sem digitacao dos IDs individuais. Tipos com rotulos candidatos: 594 animal (controle v9.2), 199 armadilha (v9.3), 458 cardume (v9.3). Nao generalizar para todas as especies/variantes. Em cada categoria, selecao automatica somente quando existe exatamente um candidato; ambiguidades nao sao resolvidas por suposicao. Nenhum novo offset foi criado/validado. Experimento real de descoberta automatica pendente.

## World Object Discovery v9.6 — primeira execucao real

Comando `--discover-world-objects-v9-6` no AoE2HD.control, janela `[1,4097)`: 0 erros; 16 `animal_candidate` tipo 594, IDs individuais consecutivos 2074–2089; 51 `fish_school_candidate` tipo 458, IDs consecutivos 2213–2263; zero `fish_trap_candidate` tipo 199 nesta janela/partida. Estes valores replicam os tipos anteriormente observados para uma variante de animal e cardume, mas nao identificam todas as especies e variantes. IDs individuais sao da sessao, nao IDs Genie de tipo. A politica de escolha automatica exige exatamente um candidato por categoria e por isso marcou as tres categorias `ambiguous_or_missing` (16/0/51); nao e falha de leitura. Proximo passo: monitoramento de varios IDs simultaneamente, preservando identidade, HP aplicavel por categoria e alimento candidato +0x48; nao escolher arbitrariamente o primeiro objeto para uma comparacao antes/durante.

## World Object Discovery v9.6 — segunda captura (18/10/2026)

CSV fornecido com 64 candidatos e zero erros: tipo 594 (12 animais, IDs 2074–2077 e 2082–2089), tipo 458 (51 cardumes, IDs 2213–2263) e tipo 199 (uma armadilha de pesca, ID individual 2299). Armadilha: Object=0x188022D0, owner=0x18F2421C, tipo pointer=0x1587C420; candidato float32 Object+0x34=50.0 e Object+0x48=15.0. Estes campos ainda requerem controle de dano/coleta para confirmacao semantica.

Entre animais tipo 594: dez objetos +0x34=7.0 e +0x48=100.0; dois objetos ID 2088/2089 com +0x34=0.0 e +0x48=97.675117/82.802887, compativeis com carcacas, MAS estado vivo/morto nao confirmado por selecao visual nesta captura. IDs 2078–2081 nao aparecem mais entre candidatos tipo 594; nao e possivel concluir se foram removidos ou mudaram de tipo sem varredura geral e acompanhamento temporal. Os 51 cardumes mantiveram seus IDs da primeira varredura. Os grupos tem multiplas entradas, portanto a politica `unique` nao resolve automaticamente a categoria animal/cardume e nao deve selecionar um objeto arbitrariamente. Enderecos absolutos sao especificos da sessao.

## World Object Monitoring v9.7 — protocolo publicado

Foi implementado no controlador `--monitor-world-objects-v9-7`: varreduras read-only repetidas do `World+0xE8` para acompanhar simultaneamente todos os IDs candidatos de animal tipo 594, cardume tipo 458 e armadilha tipo 199. Compara alimento candidato `Object+0x48` e HP candidato `Object+0x34`, sem elevar offsets a validados. Cardumes nao possuem HP aplicavel, armadilhas possuem HP aplicavel e animais com HP bruto zero sao somente candidatos a carcaca. Registra eventos baseline, alteracao, aparecimento e `missing_from_scan` (ausencia da categoria amostrada, nao destruicao confirmada); erros de leitura impedem falsos eventos de desaparecimento. Primeira captura v9.7 pendente; nenhum novo offset de memoria adicionado.
