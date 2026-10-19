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

## World Object Monitoring v9.7 — primeira serie temporal real

Fonte: arquivo enviado `world_object_monitor_v9_7.csv`. Vinte amostras, 80 linhas no CSV: 61 eventos `baseline` na primeira amostra e 19 eventos `changed` nas amostras 2 a 20; **zero `scan_error`**, zero aparecimentos ou desaparecimentos entre as amostras. Composicao da linha de base: 9 animais tipo 594 (8 com HP candidato e 1 classificado `carcass_candidate`), 51 cardumes tipo 458 sem HP aplicavel e 1 armadilha tipo 199 com HP candidato.

Objeto ID individual 2086, tipo 594, object_pointer `0x14763138`, owner_pointer `0x1489CA1C`: foi o **unico objeto modificado nas 19 comparacoes**, mantendo ID, tipo e ponteiros estaveis. `Object+0x48` float32 (recurso/alimento restante candidato) passou de 98.00434112548828 para 44.365501403808594 ao longo de 39.380952 s (UTC 2026-10-18 21:13:27 a 21:14:07), delta total -53.63883972167969, aproximadamente -1.362 unidades/s. A cada uma das 19 amostras seguintes o delta foi negativo, entre aproximadamente -2.75 e -2.89 por amostra. Compatibilidade muito forte com alimento sendo coletado continuamente em carcaca do animal tipo 594. Classificacao de carcaca vem da regra de HP bruto igual a zero e ainda e **candidata**, nao verificacao visual. A serie temporal reforca a interpretacao de `Object+0x48` como recurso restante para esse objeto, mas nao valida universalmente o mesmo offset em cardumes/armadilhas nem prova a taxa de coleta em todas as situacoes. Nenhum novo offset foi promovido a `validado` nesta etapa.

## Wood & Berry Resource Discovery v9.8 — protocolo

Controlador agora inclui `--map-wood-berries-v9-8`: inventario automatico e temporal dos objetos, sem digitar IDs, usando tipo em `[Object+0x0C]+0x10` e candidato a quantidade restante `Object+0x48` float32. O tipo 351 foi observado como arvore em controle guiado v9.2 e agora e classificado `tree_candidate`, nao como identificador universal de todas as especies de arvore. O ID de tipo dos arbustos de frutas NAO foi identificado e permanece `unclassified`. Novas coletas controladas em madeira e frutas servirao para relacionar tipo/ID individual a fonte visual; variacao de +0x48 sozinha nao valida semantica. O modo nao atribui HP a arvores/arbustos nem cria novo offset. Primeira captura v9.8 ainda pendente.

## Wood & Berry v9.8 — primeira serie temporal, 20 amostras

Fonte: `wood_berries_v9_8.csv` fornecido pelo operador. 2.188 linhas baseline, 150 `resource_changed`, 1 `appeared` (ID 2295 tipo 592, amostra 13), 1 `missing_from_scan` (ID 2294 tipo 595, amostra 19); 0 erros reportados. Tipo 351: 1.969 objetos mantidos nas 20 amostras. Outros objetos analisados: 219/220.

Alteracoes por tipo: 351 ID 2120 teve 15 deltas totalizando -16.939416 no float32 `Object+0x48`, compativel com arvore sendo cortada. Tipo 59 ID 2014 teve tres quedas (-0.984184, -0.979591, -0.255142), total -2.218917, de valores 117.219109 a 115.000191. Outros 11 objetos tipo 59 iniciaram com `Object+0x48=125` e permaneceram sem alteracoes. **Tipo 59 e candidato forte a arbusto de frutas**, pendente confirmacao visual e replicacao.

Correlacao potencial fonte -> coletor: nas amostras 2, 3 e 4, ID 2014 (tipo 59) diminuiu respectivamente ~0.984184, 0.979591, 0.255142 e ID 2002 (tipo 120) aumentou ~0.984243, 0.979604, 0.255149 (diferencas residuais pequenas). Igualmente, nas amostras 2–7, queda de ID 2120 tipo 351 foi espelhada por aumento aproximado de ID 2001 tipo 123, por exemplo -1.238266 vs +1.238250 e -1.232422 vs +1.232401. Isto sugere *transferencia de recurso entre objeto fonte e coletor*, mas **nao valida** nomes dos tipos 120/123 sem controle visual.

Outras variacoes: tipo 595 ID 2294 perdeu ~54.419091 (17 eventos), possivelmente outra fonte de recursos; tipo 590 IDs 2004/2005/2293 e tipo 592 IDs 2006/2292 mostraram valores de pequena magnitude com aumentos, possiveis estados internos de unidades, sem rotulos. Nao concluir categoria pelo valor de +0x48 isoladamente, nem afirmar que objeto ausente foi destruido. Os offsets permanecem candidatos ate experimentos controlados adicionais.

## Resource Transfer Validation v9.9 — protocolo de correspondência temporal

Foi implementado `--validate-resource-transfers-v9-9` no AoE2HD.control. A rotina compara os deltas de `Object+0x48` float32 em amostras consecutivas, ligando reduções (fonte candidata) a aumentos de magnitude aproximadamente igual (coletor candidato), para IDs e tipos estáveis. Tolera diferença absoluta até 0,02 ou relativa de 1%, com delta mínimo 0,1. Quando um nó possui mais de um parceiro possível, marca `ambiguous_candidate`; ligação biunívoca na amostra recebe `unique_candidate`, o que **não prova causalidade**. Testes com madeira tipo 351 e possíveis frutos tipo 59 são os controles prioritários; tipos de coletores 123/120 continuam candidatos. Arquivo de saída `resource_transfers_v9_9.csv`. Nenhum offset é promovido a validado antes de captura real reproduzida.

## Resource Transfer Validation v9.9 — primeira execução real

Fonte: `resource_transfers_v9_9.csv` fornecido pelo usuário. 20 amostras de ~2 s, 179 pares candidatos ao todo, classificados como 34 `unique_candidate` (19,0%) e 145 `ambiguous_candidate` (81,0%). A primeira amostra só estabelece baseline. Repetições de um mesmo par são eventos em tempos distintos, não objetos novos. Não foram identificadas falhas na saída do terminal, que mostra 2191 objetos na linha de base, chegando a 2189/2190 em outras amostras; o CSV de pares não permite explicar ausências.

Pares únicos recorrentes (fonte ID/tipo -> coletor ID/tipo; numero de amostras; soma das perdas apenas nessas amostras, em unidades do float32 +0x48; erro absoluto médio):
- 2039/66 -> 2004/581: 12 amostras, -14.449095, erro médio ~0.000105; tipo 66 já associado a ouro em mapeamentos anteriores.
- 2014/59 -> 2002/120: 9 amostras, -8.359714, erro médio ~0.000008; reforça tipo 59 como candidato a arbusto e tipo 120 como possível coletor de alimento.
- 2123/351 -> 2293/218: 5 amostras, -3.892212, erro médio ~0.000005.
- 2117/351 -> 2292/123: 4 amostras, -3.349343, erro médio ~0.000003.
- 2022/59 -> 2298/120: 2 amostras, -1.869126, erro médio ~0.000010.
- 2116/351 -> 2001/123: 2 amostras, -1.349399, erro médio ~0.000001.

O conjunto reforça que o float32 `Object+0x48` contém quantidade que diminui em fontes e aumenta em coletores candidatos. Pareamento é coincidência temporal e de magnitude com tolerância, **não prova causalidade, direção física ou classificação visual**. Tipos 59, 120, 123, 218 e 581 mantidos como hipóteses de papel funcional. Tipos 351 (árvore) e 66 (ouro) baseados em evidências anteriores. A próxima validação deve separar sessões controladas por recurso e relacionar os objetos com unidades observadas na tela, sem promover offsets a validados prematuramente.

## Wood & Berry Existence v9.10 — existencia somente

Implementado `--map-wood-berry-existence-v9-10` no controlador: uma varredura de leitura da tabela `World+0xE8`, ID individual verificado em `Object+0x08` e tipo uint16 de `[Object+0x0C]+0x10`. Rotulos candidatos: 351 `tree_candidate`; 59 `berry_bush_candidate`, este ainda sem verificacao visual definitiva. CSV `wood_berry_existence_v9_10.csv` com objeto presente/ID/ponteiros/tipo e erros; sem atributos de recurso ou HP. A ausencia em uma janela, especialmente com erros, nao valida inexistencia global. Aguarda primeira varredura real v9.10; nenhum endereco novo foi adicionado.

## Wood & Berry Existence v9.10 — primeira captura real (18/10/2026)

Entrada: CSV `wood_berry_existence_v9_10.csv` enviado pelo operador. Uma varredura sem erros: **1.981 objetos `present` com IDs individuais unicos**, sendo **1.969 entradas tipo 351 `tree_candidate`** e **12 entradas tipo 59 `berry_bush_candidate`**. Arbustos candidatos de IDs individuais consecutivos `2014–2025`, todos com ponteiro de tipo da sessao `0x14FA47D0`. O arquivo confirma ponteiros e IDs coerentes na tabela; nao mede HP, madeira, frutas, visibilidade, coletabilidade nem recursos restantes. A classificacao funcional do tipo 59 permanece candidata, embora corroborada por capturas anteriores de coleta. Ponteiros absolutos sao temporarios e nao devem ser registrados como offsets universais. Ausencia de erros na janela examinada nao estabelece que todos os tipos de arvore do jogo tenham ID 351.

## Resource Counters v9.11 — madeira, frutas, ouro e pedra

Implementado modo de captura `--map-resource-counters-v9-11` no controlador. Hipotese de contador de recurso restante `Object+0x48` float32, rastreado por ID individual e tipo obtido em `[Object+0x0C]+0x10`: 351 arvore/madeira, 59 arbusto/frutas (identidade botanica candidata), 66 ouro, 102 pedra. O controlador registra valores brutos, alteracoes de valor e identidade em CSV; **nao valida o offset genericamente em todos os quatro tipos** nem assume que todo registro e coletavel. Necessarios controles de coleta isolada de cada familia, especialmente ouro e pedra; promover somente evidencias observadas. Nenhum novo endereco de memoria foi descoberto nesta etapa.

## Resource Counters v9.11 — primeira coleta simultânea dos quatro recursos (18/10/2026)

Fonte: arquivo `resource_counters_v9_11.csv` enviado pelo operador, 20 amostras. Foram identificados 2.038 IDs únicos na linha de base: 1.969 árvore tipo 351, 12 arbusto candidato tipo 59, 36 ouro tipo 66, 21 pedra tipo 102. Total de 44 eventos `changed`, sem `scan_error`, `appeared`, `missing_from_scan` ou `identity_changed` no CSV.

Resultados do float32 candidato `Object+0x48`, por ID individual estável:
- Árvore/madeira: objeto 2115 tipo 351; valor inicial 100.0, final 86.05509948730469, 13 reduções, delta acumulado -13.944902 (arredondamento por evento).
- Arbusto/frutas: objeto 2017 tipo 59; inicial 125.0, final 115.00042724609376, 11 reduções, delta -9.999572.
- Ouro: objeto 2029 tipo 66; inicial 800.0, final 788.8148803710938, 10 reduções, delta -11.185120.
- Pedra: objeto 2059 tipo 102; inicial 350.0, final 339.998291015625, 10 reduções, delta -10.001709.

As 44 alterações ocorreram exclusivamente nesses quatro objetos; nenhum aumento de recurso foi registrado para eles. A observação em ouro e pedra amplia significativamente o suporte funcional ao endereço relativo `Object+0x48` como contador da quantidade restante das fontes observadas. Entretanto, a captura não registra confirmação visual sincronizada de cada fonte nem demonstra cobertura de todas as variantes; manter `candidate` até controles de repetição com coleta isolada. Os valores absolutos de ponteiros são específicos da sessão, e os IDs individuais também.

## Resource Counters v9.11 — experimento sequencial wood, food, gold, stone (80 amostras)

Protocolo informado pelo operador: ordem de coleta intencional madeira, alimento, ouro e pedra. CSV `resource_counters_v9_11(1).csv`, 80 amostras a intervalo configurado 2 segundos, 2038 objetos na linha de base (1969 tipo 351, 12 tipo 59, 36 tipo 66, 21 tipo 102), 97 eventos `changed` em seis IDs, sem eventos de erro, aparecimento, ausencia ou identidade alterada.

Conjunto principal consistente com a ordem relatada: ID 2121 tipo 351 madeira amostras 3–11, nove reducoes somadas -30.000023; ID 2014 tipo 59 alimento amostras 20–32, 13 reducoes somadas -29.998917; ID 2031 tipo 66 ouro amostras 40–49, dez reducoes somadas -30.003907; ID 2059 tipo 102 pedra amostras 63–75, treze reducoes somadas -30.003844. Quedas de aproximadamente 30 unidades para cada uma das quatro familias, em intervalos sucessivos, sustentam fortemente a interpretacao de `Object+0x48` float32 como quantidade restante nesses tipos observados. Os tipos sao obtidos por `[Object+0x0C]+0x10` uint16 e os IDs individuais por `Object+0x08` / `World+0xE8`.

Foram registradas alteracoes adicionais sobrepostas: arvore ID 2118 tipo 351, 35 eventos nas amostras 46–80, reducao -76.893917; ouro ID 2039 tipo 66, 17 eventos nas amostras 64–80, reducao -18.664610. Portanto o experimento foi sequencial na ordem de inicio da coleta principal, mas **nao totalmente isolado durante a fase final**. Quantidades decorrem do campo bruto e arredondamentos dos deltas CSV, nao de leitura direta de UI do jogo. Elevada confianca funcional para os quatro tipos concretamente capturados; nao generalizar para outras variantes/tipos ou inferir deposito completo/esgotamento.

## Coordenadas de aldeoes e recursos — descoberta v9.12

Foi implementado no controlador `--discover-world-positions-v9-12`, leitura experimental de float32 entre `Object+0x18` e `Object+0x70` (passo 4), com snapshots e deltas pelo mesmo ID individual. Tipos considerados: aldeao 83, arvore 351, arbusto candidato 59, ouro 66, pedra 102. **Ainda nao existe X ou Y validado**; sao apenas offsets candidatos que precisam de teste guiado com aldeao em deslocamento e recursos estacionarios. A captura nao pressupoe que esses offsets contenham posicao e nao promove nenhum deles a endereco validado. A identificacao de outros tipos de aldeao/recursos ainda deve ser ampliada a partir de dados reais.

## Villager Coordinate Discovery v9.12 — candidatos a X/Y (sem offset confirmado)

Foi criado `--discover-villager-coordinates-v9-12` no controlador: encontra automaticamente objetos de aldeoes tipo 83, verifica identidade `World+0xE8` / `Object+0x08` e compara em intervalos os campos brutos alinhados de `Object+0x14` ate `Object+0xBC` (janela default 192 bytes). Registro diferencial CSV inclui ID, ponteiro, offset e leituras `float32` e `int32` candidatas. Nenhum offset de posicao foi identificado/validado nesta etapa. Proximos controles: aldeao parado, movimento em um eixo e movimento no outro eixo, de preferencia cada um em captura separada; reconhecer movimento nao equivale automaticamente a coordenada.

## Villager Coordinate Discovery v9.12 — primeira captura real (18/10/2026)

Fonte: arquivo enviado `villager_coordinate_candidates_v9_12.csv` (20 amostras, 249 linhas de mudanças; 3 aldeões tipo 83 identificados por amostra, somente ID individual **2001** alterou bytes). Amostras 1–3 e 5 sem alterações; 4 com uma; 6 com 21; 7–19 com 16 por amostra; 20 com 19. Não houve `scan_error` no CSV.

**Candidatos fortes a componentes de posição `float32`**:
- `Object+0x3C`: na amostra 6 ~15.37470, cresce até ~23.44337 na 13, depois quase constante e levemente decrescente até ~23.218 na 20.
- `Object+0x40`: na amostra 6 ~41.47043, decresce levemente até ~41.015 na 13, depois decresce continuamente até ~32.052 na 20.

As duas sequências formam um percurso compatível com duas pernas aproximadamente perpendiculares (uma componente domina 6–12, outra domina 14–20), forte evidencia temporal para campos de posição. **Ainda não foi confirmado experimentalmente qual é X/Y do mapa, nem a escala/unidade ou correspondencia com tiles**. Offsets `+0x9C/+0xA0/+0xA4` e `+0xB4/+0xB8/+0xBC` tambem variaram de modo correlacionado, possivelmente representacoes transformadas da posicao, sem interpretacao validada. Proximo controle: comparar posicoes de jogo conhecidas em eixos independentes, e testar parada e deslocamento de um segundo aldeao. Nenhum offset elevado a `validado` nesta etapa.

## Villager Coordinates v9.13 — monitor por ID

No AoE2HD.control, `--monitor-villager-coordinates-v9-13` acompanha todos os aldeões tipo 83 por ID individual usando a tabela `World+0xE8`. Candidatos `Object+0x3C` float32 e `Object+0x40` float32 são exportados em toda amostra, com deltas por objeto e distância candidata. Detecta estabilidade, movimentação, aparecimento, ausência na varredura e troca de identidade; não mistura trajetórias de IDs distintos. Os rótulos espaciais X e Y permanecem **candidatos**, pois a v9.12 mostrou uma trajetória em L mas ainda não confirmou correspondência com eixos/tiles do mapa. Primeira validação da v9.13 pendente.

### Resultado da captura v9.13

A captura real dos três aldeões e a evidência dos offsets candidatos `Object+0x3C` e `Object+0x40` estão documentadas em [Coordenadas dos aldeões v9.13](coordenadas_aldeoes_v9_13_2026-10-18.md). Os offsets ainda não foram validados contra posições conhecidas no mapa.

## Town Center Coordinates v9.14 — candidatos do Centro da Cidade

Novo comando do controlador `--map-town-center-coordinates-v9-14`. Descoberta por `World+0xE8`, tipo `[Object+0x0C]+0x10` igual a 109 (`town_center_dark`) e proprietário `Object+0x10` igual ao jogador local. Captura `Object+0x3C` e `Object+0x40`, `float32`, como coordenadas candidatas; ainda não foram validadas especificamente para edifícios. A coordenada pode indicar origem/âncora e não o centro da área ocupada. Tipos de TC alternativos podem existir, portanto ausência de tipo 109 não demonstra ausência de Centro da Cidade. Primeira captura real pendente.

## Town Center Coordinates v9.14 — primeira captura real

Captura do controlador com `--map-town-center-coordinates-v9-14`, arquivo `town_center_coordinates_v9_14.csv`: exatamente um Centro da Cidade candidato pertencente ao jogador local, sem erros. ID individual **1991**, Genie type **109** (`town_center_dark`), `Object+0x3C` float32 = **20.0** e `Object+0x40` float32 = **40.0**. O proprietário foi confirmado pelo filtro de jogador local. Ponteiros observados nesta sessão: objeto `0x1060E3B8`, proprietário `0x18FF2C1C`; não reutilizar como endereços fixos. A leitura é evidência inicial de posição de construção, mas ainda não estabelece se (20,40) designa centro geométrico, âncora ou canto da fundação. Estado: **candidato**, aguardando comparação com localização visual ou nova construção.

## Town Center Coordinates v9.14 — segunda partida

Nova partida, CSV `town_center_coordinates_v9_14(1).csv`: um Centro da Cidade do jogador local, tipo Genie 109, ID individual 2186, ponteiro de objeto `0x153D78E8`, proprietário `0x106D7014`, `Object+0x3C` (`float32`) = **39.0**, `Object+0x40` (`float32`) = **42.0**, sem erros. Comparação com a primeira partida: ID 1991, coordenadas (20.0, 40.0). Diferença entre partidas: +19.0 em +0x3C e +2.0 em +0x40; **não é deslocamento de um mesmo edifício**, mas valores de objetos distintos em partidas distintas. Essa repetição reforça que os offsets guardam posições candidatas de Centros da Cidade tipo 109; para validar os eixos/âncora ainda é necessária comparação com o mapa ou dois edifícios na mesma partida. Ponteiros e IDs são específicos da sessão.

## Town Center v9.14 — terceiro TC acima do primeiro

Captura em mesma partida, tres Centros da Cidade locais, tipo 109, sem erros: original ID 2186 = (39,42), segundo ID 2500 = (43,46), terceiro ID 2510 = (43,38), nos campos float32 `Object+0x3C` e `Object+0x40`. Operador informou segundo TC à direita e terceiro acima do primeiro na tela. Deslocamentos internos relativos ao original: direita (+4,+4), acima (+4,-4). Dois vetores independentes, compatíveis com a projeção isométrica. Evidência espacial reforça que os offsets armazenam componentes de posição dos edifícios tipo 109. Não determina centro geométrico da construção ou escala de tiles. IDs e ponteiros são específicos da partida.

## Town Center Spatial Validation v9.15 — projeção isométrica candidata

Implementado `--validate-town-center-spatial-v9-15` no controlador. Para Centros da Cidade locais tipo 109, calcula as coordenadas projetadas sem unidade de pixel `u = X + Y` e `v = Y - X`, a partir dos offsets `Object+0x3C` e `Object+0x40` (float32). Captura de três TC: ID 2186 (39,42) → (81,3); ID 2500 (43,46) → (89,3), deslocamento em U (+8,0) condizente com TC colocado à direita; ID 2510 (43,38) → (81,-5), deslocamento em V (0,-8) condizente com TC colocado acima. Dois vetores independentes sustentam o modelo linear normalizado; **não representa coordenadas de pixel, orientação absoluta, escala de tiles ou centro geométrico da fundação**. O próximo controle consiste em confrontar a transformação com posicionamentos visuais medidos, zoom e câmera fixa. Arquivo gerado: `town_center_spatial_v9_15.csv`.

## Town Center Spatial Validation v9.15 — execução real

Arquivo `town_center_spatial_v9_15.csv`: três Centros da Cidade do proprietário local, tipo 109, referência ID 2186 e zero erros. TC 2186: XY=(39,42), UV=(81,3), delta UV=(0,0). TC 2500: XY=(43,46), UV=(89,3), delta UV=(+8,0), direção visual informada à direita. TC 2510: XY=(43,38), UV=(81,-5), delta UV=(0,-8), direção visual informada acima. Para ambos os TCs secundários, distância euclidiana entre coordenadas XY e a referência = 5.656854249492381 unidades candidatas. A captura confirma a consistência matemática da transformação experimental `u=X+Y` e `v=Y-X` para as duas direções visuais relatadas. Não confirma escala em pixels, zoom, orientação absoluta nem centro geométrico das construções. Os endereços `Object+0x3C` e `Object+0x40` permanecem validados funcionalmente para componentes de posição dos TCs tipo 109 observados.

## Town Center Screen Projection Calibration v9.16 — ajuste pendente de medidas reais

Implementado no controlador `--calibrate-town-center-screen-v9-16`. Entradas: pares de posição XY candidatos (`Object+0x3C`, `Object+0x40`, float32) dos TCs locais tipo 109 e posições dos mesmos edifícios medidas em pixels em CSV `object_id,screen_x,screen_y`. Ajuste linear `screen_x=a*(X+Y)+b`, `screen_y=c*(Y-X)+d` com resíduos e RMSE. Coeficientes ainda **não calibrados empiricamente**; a v9.15 estabeleceu apenas coordenadas UV normalizadas. Posições de tela exigem zoom/câmera/janela fixos e uma definição constante de ponto do edifício. Mínimo de três referências, variância não nula nos dois eixos, idealmente validação independente em quarto TC. Não confundir valores ilustrativos de testes com medidas reais.

## Screen Projection Calibration v9.16 — primeira calibração real

Medições fornecidas para os três TCs na mesma configuração de tela/câmera: ID 2186 XY mundo (39,42), UV (81,3), pixel (51,417); ID 2500 XY (43,46), UV (89,3), pixel (435,417); ID 2510 XY (43,38), UV (81,-5), pixel (51,224). Execução real de `--calibrate-town-center-screen-v9-16` retornou `pontos=3`, `RMSE_px=0.000`, `screen_x=48.000000*u-3837.000000` e `screen_y=24.125000*v+344.625000`. Assim, `screen_x=48*(X+Y)-3837` e `screen_y=24.125*(Y-X)+344.625` para a configuração observada. O RMSE zero é erro **no conjunto de ajuste**, não validação independente; o modelo e a definição do ponto de ancoragem precisam ser testados em pelo menos um quarto TC sem recalibração. Não generalizar a outros zooms, câmeras, resoluções ou tipos de objetos.

## Screen Projection v9.16 — teste independente com quarto TC

Mesma partida e enquadramento atual: TC 2186 XY=(39,42), pixel (438,428); TC 2500 XY=(43,46), pixel (822,428); TC 2510 XY=(43,38), pixel (438,236); TC 2539 XY=(35,38), pixel (55,428). Todos locais e tipo 109, zero erros na leitura. Ajuste usando apenas os tres primeiros: screen_x=48*(X+Y)-3450; screen_y=24*(Y-X)+356, com RMSE de treino 0 px. O quarto nao integrou o ajuste: previsao (54,428), observacao (55,428), erro euclidiano de 1 pixel. Os interceptos diferem dos da camera anterior, o que limita a validade ao enquadramento/zoom/resolucao e ancora medidos. O ensaio mostra capacidade de generalizacao a um quarto TC nessa configuracao; nao valida automaticamente classes diferentes, pontos fora do campo observado ou transformacao sob mudanca de camera.
