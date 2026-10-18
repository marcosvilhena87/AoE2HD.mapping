# Identidade individual de aldeoes - Discovery v1

Instrumentacao no AoE2HD.control com modo --discover-villager-identity. Usa cadeia historica ate WorldPlayer local, selecao LocalPlayer+0x15C/+0x160, candidatos Object+0x08 ID, +0x0C tipo, +0x10 proprietario e +0x34 HP. Etapas: A, B, C, reselecao A, movimento/reselecao A, novo D. Confere estabilidade de selecao, identidade diferente entre objetos e igualdade ao reselecionar A. Resultados ainda nao obtidos em jogo real. Todos os candidatos continuam com seus status originais, sem promocao. Proximo passo: investigacao de colecao global de objetos para enumeracao sem selecao.

## Resultado real Villager Identity Discovery v1 (seis capturas)

CSV `villager_identity_v1.csv`: `WorldPlayer local=0x18315744` nesta sessao. A: objeto `0x13EA6720`, ID candidato 2108; B: `0x13EA85A0`, ID 2109; C: `0x13EA81D0`, ID 2110; reselecao de A e movimento/reselecao de A mantiveram ponteiro `0x13EA6720` e ID 2108; D: `0x13EAE6D8`, ID 2417. Todas as seis fases registraram leitura bem sucedida e ancoras LocalPlayer+0x15C/+0x160 coincidentes. Identificadores distintos nao sao necessariamente sequenciais.

`Object+0x10` apontou para `0x18315744` em todos os objetos (igual ao jogador local; evidencia da hipotese de proprietario). `Object+0x34` foi 40.0 em todos (HP candidato, ainda sem controle por dano). `Object+0x0C` foi 408937992 para A/B/C, mas 334993232 para D: refuta a interpretacao simplista de um unico codigo de tipo identico para todos os aldeoes observados; investigar papel estrutural e variantes. Os enderecos absolutos sao apenas desta sessao. `Object+0x08` e identidade individual **comprovada comportamentalmente nesta sessao**, mas status do catalogo permanece hipotese ate controles em partida independente/apos reinicio e teste de vida/morte/reutilizacao de IDs. Nenhuma enumeracao automatica de todos os aldeoes foi comprovada.

## Discovery v2 — procura de colecao de objetos

O comando `--discover-villager-collection` no AoE2HD.control inspeciona somente leitura referencias a aldeoes selecionados, usando os caminhos historicos para World, WorldPlayer e ResourceTable. Busca enderecos que apontam para cada objeto selecionado em estruturas de raiz e em ponteiros filhos limitados (janela ate 0x2000 bytes; padrao 0x800; ate 32 filhos por raiz). Controle: A, B, C, D e ausencia de selecao. Registra endereco e origem dos hits; referencias em LocalPlayer+0x15C/+0x160 sao tratadas explicitamente como ancoras, nao colecao. Nenhuma colecao ou offset novo foi confirmado; os candidatos de World+0x88 e CandidateBase+0x178/+0x28C permanecem com seu status historico. Nao ha uso de ponteiros absolutos de aldeoes de sessao anterior. Captura real de v2 pendente.

## Resultado da coleta v2

O experimento gerou 9 registros: oito referencias localizadas em LocalPlayer+0x15C e +0x160 (duas por aldeao A/B/C/D) e um controle negativo sem selecao aprovado. IDs observados: 2108, 2109, 2110 e 2417. Nenhuma referencia de colecao independente da selecao foi identificada dentro da cobertura limitada. Os offsets candidatos existentes continuam como hipoteses; nao foram promovidos. O resultado nao exclui colecoes em outras regioes de memoria.

## Collection Discovery v3 — implementacao pendente de execucao real

Novo comando no AoE2HD.control: `--discover-villager-collection-v3`. Coleta A/B/C/D da sessao atual via selecao; remove selecao no controle final; pesquisa blocos diretos e filhos de World, WorldPlayer e ResourceTable, priorizando World+0x88. Procura dois ou mais IDs ou ponteiros de aldeoes distintos na mesma regiao, sob strides 4, 8, 12 ou 16 bytes. As ancoras de selecao conhecidas nao contam como evidencia de colecao. Nao reutiliza enderecos absolutos da sessao passada; resultados serao registrados como `collection_candidate`, sem validacao automatica ou mudanca dos statuses do catalogo. A implementacao e os testes sinteticos foram publicados, sem captura real ate o momento.

## Resultado real Collection Discovery v3

Arquivo villager_collection_v3.csv: cinco registros. Referencias de identidade A=2108, B=2109, C=2110, D=2417 registradas; controle no_selection com status negative_selection_pass; 50 regioes examinadas e **zero** linhas collection_candidate. A ferramenta nao identificou arranjo compartilhado de dois ou mais desses objetos nos strides 4/8/12/16 na cobertura pesquisada. Resultado negativo e valido, mas nao exclui colecao global baseada em indices, referencias indiretas ou blocos de dimensao maior. Nenhum novo offset deve ser promovido ou inferido. Os campos e enderecos candidatos anteriores permanecem preservados.

## Villager Collection Discovery v4 — candidato, sem execucao real

Comando no AoE2HD.control: `--discover-villager-collection-v4`. A investigacao reutiliza os IDs e ponteiros atuais selecionados de A/B/C/D, exige controle sem selecao e examina descritores begin/end/capacity proximos das raizes ja conhecidas (prioriza World+0x88), vetores de ate 8192 bytes com strides 4/8/12/16/20/24/32/40/48/64/80/96/128, bem como sondagens de tabelas diretas indexadas por Object_ID (offsets ID*4, ID*8, ID*16). Saida inclui regioes cobertas, falhas de leitura e candidatos. Baseline historica: UnitIdentityDiscovery.h e entity_graph_discovery_v5.cpp, revisao 62c3a7e1. Nenhum endereco novo confirmado; World+0x88 permanece hipotese. Captura real v4 pendente.

## Resultado observado da v4

A captura de jogo registrou uma tabela candidata em World+0xE8. O ponteiro de tabela observado na sessao foi 0x13D60018. As entradas em tabela + ID * 4 apontaram para quatro objetos distintos: 2108 para 0x13EA6720, 2109 para 0x13EA85A0, 2110 para 0x13EA81D0 e 2417 para 0x13EAE6D8. Foram 84 sondagens e um candidato, com selecao vazia. Este resultado e uma hipotese forte, nao uma validacao entre reinicios. Necessita teste de novas unidades, mortes, IDs ausentes e reinicio do jogo.

## Villager Collection Validation v1 — protocolo publicado

AoE2HD.control implementou `--verify-villager-collection`, sondagem somente leitura de `World+0xE8` e entrada `table+Object_ID*4` em oito etapas guiadas: A, B, C, desmarcados, D novo, desmarcados com D, A morto, ID potencialmente inexistente. Registra endereco efetivo da tabela por sessao, ID, ponteiro esperado e observado, rechecagem de raizes e selecao. Recomenda-se reiniciar o jogo antes de executar para teste independente. Campos de morte e ID invalido nao tem comportamento presumido: uma entrada pode permanecer historicamente preenchida ou estar inacessivel. Nenhuma captura real desta validacao foi fornecida ainda; manter World+0xE8 como `hipotese` ate o experimento.

## Validation v1 — resultado em nova sessao

Captura real `villager_collection_validation_v1.csv` com 13 linhas: World 0x13EB7980, tabela World+0xE8 = 0x04E47008. A (ID 2202, objeto 0x13D0C8F8), B (2203, 0x13D0CAE0) e C (2201, 0x13D0C710) corresponderam na selecao e sem selecao. Novo D (2505, 0x180E5F28) correspondeu selecionado e desmarcado. Total de 11 leituras de consulta com ponteiro esperado, inclusive 7 sem selecao. Depois da perda de A, entrada ID 2202 foi zero. Controle de ID inexistente ficou inconclusivo: selecao ativa 0x180DEAE0 impediu a tentativa. Nova sessao tem base diferente do experimento v4; funcionalidade da tabela por ID reproduzida. Promover World+0xE8 a validado funcionalmente **somente para consulta de objetos conhecidos por ID**, sem afirmacao de tamanho, enumeracao integral ou politica de reuso de IDs. Enderecos absolutos sao exemplos de sessao, nao estaveis.

## Villager Enumeration v1 — protocolo instrumental

Publicada no AoE2HD.control a enumeracao exploratoria `--enumerate-villagers`. Percorre faixa numerica limitada de indices de `World+0xE8` (padrao IDs 1..4096), ignora entradas nulas e confere Object+0x08 == ID indexado. Documenta Object+0x0C (tipo candidato), +0x10 (proprietario candidato) e +0x34 (HP candidato), com contagem independente ResourceTable+0x94. Exige selecao vazia e rechecagem de raizes. Nao foi identificado campo comprovado para distinguir aldeoes de outras unidades; resultado classifica **objetos**, nao automaticamente aldeoes. Nao confirma completude nem tamanho real da tabela. Coleta real de enumeracao v1 pendente. Nenhum novo offset adicionado.

## Enumeracao v1 — resultado experimental

CSV `villager_enumeration_v1.csv`: faixa de IDs [1,4097); 2470 objetos `object_candidate` e nenhum erro registrado; 8 pertencentes ao jogador local segundo `Object+0x10`, com contador validado ResourceTable+0x94 = 3.0 aldeoes vivos. IDs locais e HP candidatos: 2191,2192,2193,2194 com HP=2400; 2201,2202,2203 com HP=40; 2207 com HP=45. Os tres IDs 2201/2202/2203 sao **aldeoes provaveis**, pois coincidem em quantidade com o contador, mas nao estao identificados semanticamente apenas por HP. Campo `Object+0x0C` foi diferente entre os tres (404567384 no 2201; 406010448 nos 2202/2203), portanto nao fornece codigo de tipo de aldeao simples. A enumeracao por World+0xE8 funciona neste recorte, mas sua completude e limite maximo sao desconhecidos. Nenhum offset novo nem promocao de tipo resultou desta captura.

## Unit Definition Pointer Discovery v1 — experimento publicado

`AoE2HD.control --discover-unit-definition` implementa descoberta limitada e somente leitura de referencias de objeto para suposta definicao de unidade. Requer capturas selecionadas de tres aldeoes, milicia, lanceiro e construcao, conferindo cada identidade na tabela validada `World+0xE8`. Inspeciona ponteiros DWORD no objeto e uma janela limitada dos alvos; registra ponteiros compartilhados e numeros candidatos 4 (classe interna Civilian hipotetica), 904 (classe exposta pela API DE) e tipos 83/74/93. Nao supoe que os enums do DE estejam literalmente na memoria do HD. Nenhuma captura real deste protocolo foi executada ou apresentou offsets novos: catalogo permanece inalterado, inclusive Object+0x0C ainda sendo hipotese.

## Unit Definition Pointer Discovery v1 — resultado real

Seis capturas completas (aldeoes IDs 2203/2201/2202, milicia 2508, lanceiro 2511, construcao 2191). CSV com 78 linhas: seis captured, 56 pointer_candidate e 16 field_candidate. Nos tres aldeoes, Object+0x78 aponta a bases distintas; em cada base, +0x0C e +0x94 continham uint32=4. Compatibilidade com hipotese Civilian/aldeao, **nao prova** semantica de classe. Milicia e lanceiro tiveram ponteiros em Object+0x0C com palavras em +0x10 e +0x14 cujo uint16 inferior era respectivamente 74 e 93; as palavras completas nao eram 74 ou 93. Essa coincidencia pode ser codificacao ou artefato; investigar com controles. Object+0x00 foi identico em aldeoes e militares e diferente em construcao, sugestivo de vtable comum sem identificacao por classe. Object+0x0C dos aldeoes A/C coincidiu, B diferiu, portanto nao supor definicao unica para todos os aldeoes. Candidatos devem permanecer como hipoteses ate controles e repeticao apos reinicio.

## Referencia externa de IDs do AoE2Control (DE)

Ver [ids_aoe2control_hipoteses.md](ids_aoe2control_hipoteses.md). Enumeracoes da API DE: UnitClass.VILLAGER=904, BUILDING=903, INFANTRY=906, SPEARMAN=950 e UnitObjectType.VILLAGER_MALE=83. A investigacao HD considera 4/3/6/50 como possiveis representacoes internas **a validar**, e 74/93 como controles empiricos do HD. Nao ha transferencia demonstrada de offsets entre edicoes; valores sao apenas hipoteses numericas e nunca prova automatica de classe. Nenhum endereco novo identificado nesta revisao.

## Referencia externa: identificadores do AoE2Control (DE)

Documentadas em `docs/hipoteses_ids_aoe2control_2026-10-17.md` as hipoteses de classes 904/4 aldeao, 903/3 edificio, 906/6 infantaria, 950/50 lanceiro, e tipos 83/74/93. Esses numeros sao sugestoes para sondagem numerica com controles, e **nao** evidenciam offsets de memoria identicos entre DE e HD. O controle de classe continua pendente; nenhum endereco foi validado por correspondencia numerica isolada.

## Unit Class Validation v1 — implementacao pendente de captura real

Em AoE2HD.control foi implementado `--validate-unit-class` para verificar `Object+0x78 -> +0x0C/+0x94` em uint32 e low16. Etapas: tres aldeoes diferentes, milicia, lanceiro, construcao, mesmo aldeao A apos mudar de atividade e A consultado sem selecao. Compara candidato de classe interna 4 (aoe2control UnitClass.VILLAGER=904 e transformacao -900 como hipotese), mas registra dados brutos de cada controle e falhas. Verifica identidade no indice `World+0xE8` e rejeita mudanca de raizes/selecao no meio da captura. Nao altera o status dos campos: Object+0x78, Object78Target+0x0C e Object78Target+0x94 continuam `hipotese`. Nenhuma captura real da v1 ainda foi recebida.

## Unit Class Validation v1 — captura real 2026-10-18

CSV `unit_class_validation_v1.csv`: oito etapas `captured`, zero erros. Cadeia `Object+0x78 -> target+0x0C` leu 4 para aldeoes A/B/C (IDs 1969, 1971, 1970), 6 para milicia 2275 e lanceiro 2277, 3 para construcao 1959. O mesmo aldeao A permaneceu com classe candidata 4 depois de mudar de atividade e quando consultado sem selecao. Evidencia funcional forte para classificacao aldeao/infantaria/construcao nos controles da sessao; ainda testar outra sessao reiniciada e outras classes antes de afirmar cobertura integral. Campo independente `target+0x94` leu 5 nos aldeoes e mudou para 6 no mesmo A apos mudanca de atividade, descartando-o como identificador estavel de classe nessa observacao. A consulta da tabela por ID permanece independente da selecao. Os IDs 4/6/3 sao compativeis com a hipotese de classes internas Genie/AoE2Control DE, mas a interpretacao nao depende de equivalencia de offsets entre as versoes.

## Unit Class Validation v2 — protocolo apos reinicio

No repositorio AoE2HD.control foi publicado `--validate-unit-class-v2` como repeticao independente do experimento. Exige oito controles validos: tres aldeoes diferentes (classe candidata 4); milicia/lanceiro (6); construcao (3); A apos atividade diferente (4, mesma identidade); A consultado sem selecao (4). Relatorio registra objetos, bases, ponteiro Object+0x78, classe em target+0x0C e campo comparativo target+0x94, com avaliacao explicita `consistent_with_hypothesis` apenas para bateria completa. Sem captura nova, manter os tres offsets de definicao em hipotese; aprovacao funcional depende de dados reais apos reinicio.

## Unit Class Validation v2 — nova sessao e validacao funcional

CSV `unit_class_validation_v2.csv` (2026-10-18): oito capturas bem-sucedidas, zero erros, veredito `consistent_with_hypothesis`. World=0x14857050 e tabela=0x14F35928 nesta sessao. Aldeoes ID 2150/2149/2151: `Object+0x78 -> target+0x0C` = uint32 4; milicia 2457 e lanceiro 2464: 6; construcao 2139: 3. O mesmo aldeao A (ID 2150, objeto 0x14E95C80) permaneceu classe 4 apos mudar atividade e desmarcado. Em conjunto com a v1 (IDs diferentes e os mesmos controles), valida funcionalmente Object+0x78 e Object78Target+0x0C para categorias aldeao/infantaria/construcao. Escopo: nao atesta todas as classes do jogo, nem layout do DE. `target+0x94` leu 8 nos aldeoes e 0 nos controles, mas diferiu da v1, nao e campo estavel de classe. Enderecos absolutos sao especificos da sessao.

## Villager Enumeration v2 — modulo publicado, captura real pendente

`AoE2HD.control --enumerate-villagers-v2` implementa consulta de entradas `World+0xE8 + ID*4` e verificacao de `Object+0x08`, propriedade local candidata `Object+0x10` e classe validada `Object+0x78 -> target+0x0C`. Classes observadas: aldeao 4, infantaria 6, construcao 3; outras classes ficam desconhecidas. Compara a quantidade de aldeoes locais enumerados ao contador de vivos `ResourceTable+0x94` antes e depois da captura, com rejeicao de mudancas nas raizes/tabela/selecao. A cobertura depende da faixa numerica de IDs; `count_matches_in_window` nao prova enumeracao completa. Sem novos enderecos identificados e sem captura real desta v2, catalogo inalterado.

## Villager Enumeration v2 — captura real com classificacao funcional

CSV `villager_enumeration_v2.csv`: faixa [1,4097), 2422 entradas nao nulas processadas; 2414 `other_owner`, oito `classified`, zero `read_error`. Objetos do jogador local: construcoes de classe 3 IDs 2139, 2140, 2141, 2142; aldeoes de classe 4 IDs 2149, 2150, 2151; objeto ID 2155 de classe 47, ainda sem interpretacao comprovada. Nao havia infantaria local de classe 6 nessa captura. Contador `ResourceTable+0x94` = 3.0, identico aos tres aldeoes encontrados: `count_matches_in_window`. Os tres IDs dos aldeoes coincidem com os IDs da validacao de classe v2 anterior. Conclusao: enumeracao automatica de aldeoes locais **funcionou neste intervalo e estado**, com concordancia independente do contador. Nao extrapolar a cobertura para IDs superiores a 4096, para todas as classes, nem classificar a classe 47 sem controles.

## Building Identity & Ownership Validation v1 — protocolo publicado

AoE2HD.control disponibiliza `--validate-building-identity` com CSV para cinco etapas: selecionar TC local; selecionar TC adversario se possivel (pular quando indisponivel); enumerar edificios de classe 3 locais sem selecao; selecionar batedor para conferir ID/classe 47; construir edificio novo e comparar a lista de IDs de classe 3. Todos os dados sao obtidos por leitura de `World+0xE8`, `Object+0x08`, `Object+0x10`, `Object+0x78 -> target+0x0C` e `Object+0x34`. O protocolo investiga se objetos de classe 3 adicionais correspondem a componentes de um mesmo edificio, sem presumir isso. A propriedade do adversario deve ser verificada diretamente; se a selecao adversaria nao for possivel, o passo pode ser pulado. Os resultados de execucao real ainda estao pendentes; nenhum novo offset ou classe foi validado pela publicacao.

## Building Components Discovery v1 — protocolo de ambos os proprietarios

Publicado no AoE2HD.control o modo `--discover-building-components`, que percorre a tabela `World+0xE8` em faixa numerica limitada (padrao 1..4096), confirma identidade `Object+0x08` e captura objetos com classe 3 pela cadeia validada `Object+0x78 -> alvo+0x0C` **sem restringir ao jogador local**. Agrupa resultados pelo ponteiro proprietario `Object+0x10`, registra vida candidata e mudancas de conjunto de IDs entre baseline, apos casa do adversario e apos construcao local. Se a casa adversaria ja existia na captura inicial, nao se pode identifica-la por delta temporal. Nao ha prova de que os IDs 2140/2141/2142 sejam componentes do TC 2139; sao objetos classe 3 nao identificados individualmente. Nenhum offset novo nem promocao do catalogo: captura real deste modulo pendente.

## Building Components Discovery v1 — captura real

CSV `building_components_v1.csv` (tres fases): baseline objetos classe 3 por proprietario: local `0x106E4014` IDs 2139,2140,2141,2142 (HP candidato 2400), adversario `0x18B84854` IDs 2144,2145,2146,2147 (HP candidato 2400) e 2451 (HP candidato ~128,89). A etapa apos casa adversaria nao alterou os IDs porque a casa ja existia na primeira captura. Apos nova construcao local surgiu apenas ID 2453; total local 4->5, adversario manteve 5. Simetria de quatro objetos classe 3 com HP 2400 em cada lado reforca, mas nao comprova, que cada Centro da Cidade seja representado por quatro entidades internas. ID 2451 e candidato a casa adversaria, ainda sem selecao direta. Captura marcou 2377 falhas por fase, primeira `Object+0x78 nao e ponteiro plausivel`: scanner aplicado a entidades com layout nao compativel. O controlador passou a ignorar esse caso como `unsupported layout`, sem afirmar corrupcao de memoria; erros distintos continuam reportados. Nenhum novo offset confirmado.

## Building destruction v2 — desaparecimento coordenado

CSV building_destruction_v2.csv: nas duas primeiras fases existiam quatro objetos classe 3 locais IDs 2139, 2140, 2141, 2142 (HP candidato 2400), e cinco objetos adversarios. Na ultima fase, os quatro IDs locais desapareceram simultaneamente da enumeracao, os cinco adversarios permaneceram, e surgiu novo objeto local classe 3 ID 2454 (HP 750). Observacao fortemente compativel com quatro entidades associadas ao ciclo de vida do Centro da Cidade local (ID 2139 selecionado como TC em experimento anterior). Ainda nao determina os campos estruturais de vinculo nem a semantica exata dos IDs auxiliares. O scanner reportou ~2377-2378 incompatibilidades por fase, sobretudo Object+0x78 nao plausivel; cobertura parcial, sem confundir falhas de classificacao com ausencia individual comprovada. Estado: evidência experimental, sem novo endereco de memoria validado.

## Building Type Memory Discovery v1 — novo protocolo

Publicado no AoE2HD.control o modo `--discover-building-type`. O experimento compara janelas de memoria de definicoes obtidas por `Object+0x78` para casas selecionadas (tipo esperado 70) e Centros da Cidade na Idade das Trevas (tipo esperado 109). Examina candidatos uint16 e uint32 em offsets alinhados a 2 bytes, registra ponteiro de definicao, ID individual, proprietario e classe. Exige controles de pelo menos duas casas e dois TCs para reportar offset `consistent_candidate`. IDs 70/109 sao referencias de tipos do motor do jogo, mas **nenhum offset de tipo no HD esta validado**. Validacao independente ainda pendente; os campos nao foram acrescentados ao catalogo.

## Building Type Memory Discovery v1 — captura negativa em cinco controles

Arquivo `building_type_v1.csv`: casa local A ID 2500 def 0x10825D18; TC local ID 2188 def 0x14AFE1E8; TC adversario ID 2193 def 0x14AFF160; casa local B ID 2504 def 0x108263F8; casa adversaria ID 2499 def 0x10825638. Classe 3 nos cinco casos, sem erro. Nenhum candidato numerico 70/109 detectado em uint16/uint32 na janela de 256 bytes da estrutura alvo de Object+0x78. Ponteiros distintos entre as tres casas: Object+0x78 nao e endereco de definicao de tipo compartilhado literalmente entre essas instancias nesta sessao. Nao exclui um tipo em outro offset, outra codificacao ou ponteiro indireto. Proxima etapa: capturar dumps de bytes e investigar subponteiros/assinaturas comuns com controles entre proprietarios. Sem promocao de offsets.

## Building Type Pointer Discovery v2 — protocolo

O controlador passa a oferecer `--discover-building-type-v2`, com captura bruta de 512 bytes a partir da estrutura apontada por `Object+0x78` para controles de casas e Centros da Cidade. Exporta arquivos `building_type_pointer_v2.csv` (comparacao de offsets de uint8, uint16, uint32 e candidatos de ponteiros) e `building_type_pointer_v2_raw.csv` (dumps hexadecimais). Identifica constantes por tipo com >=2 casas e >=2 Centros da Cidade; explora somente um nivel de subponteiros, com leituras limitadas a 256 bytes. O fato de os ponteiros de definicao serem distintos na v1 motiva a busca de um segundo nivel de referencia, sem a presumir. Resultados reais pendentes e **nenhum offset de tipo validado**.

## Building Type Pointer Discovery v2 — resultado em cinco construcoes

Captura real: 3 casas (IDs 2500,2504,2499) e 2 Centros da Cidade (2188,2193), janelas 512 bytes, 35 candidatos no CSV e dumps completos no `_raw.csv`. Campo **[Object+0x78]+0x16C** lido como uint32 = **39** em todas as casas e **40** em ambos os TCs (uint16 igualmente). Campo +0x140 uint32 = **3000** para casas, **1000** para TCs; +0x164 interpretado como float32 = **2.0** nas casas e **8.0** nos TCs. Campo +0x000 contem ponteiro 0x00E9F06C para casas e 0x00E9EE8C para TCs; alvos lidos possuem bytes identicos dentro de cada grupo e diferentes entre grupos, compativel tambem com metadados/vtable. Valores em +0x16C nao sao os IDs Genie 70/109: pode ser outro discriminador, exige controles com mais tipos e sessoes. Outros candidatos tem semantica incerta. **Nao promover como identificador de tipo validado** antes de controles adicionais.

## Building Type Validation v3 — protocolo para sessao independente

O controlador oferece `--validate-building-type-v3`: seis controles manuais em jogo reiniciado/nova partida (duas casas locais, casa adversaria, TC local, TC adversario, outra construcao). Registra `[Object+0x78]+0x16C` uint16/uint32, +0x140 uint32, +0x164 float32 e +0x000 ponteiro. Hipoteses da v2: 39 para casas e 40 para TCs no +0x16C. O criterio computacional requer duas casas e dois TCs concordantes, e um terceiro tipo com valor diferente; identifica somente `consistent_candidate`, nunca promocao automatica a validado. Pendentes captura real v3, controles de mais tipos e confirmacao independente de reinicio. CSV `building_type_validation_v3.csv`.

## Building Type Validation v3 — quartel contradiz +0x16C como tipo exclusivo

CSV `building_type_validation_v3.csv`, seis capturas sem erros: casas IDs 2503,2508,2502: [Object+0x78]+0x16C uint32=39, +0x140 uint32=3000, +0x164 float32=2.0; TC local ID 2186 e adversario ID 2191: +0x16C=40, +0x140=1000, +0x164=8.0; **quartel** selecionado ID 2504: +0x16C=39, +0x140=3000 e **+0x164=6.0**. Campo +0x000: casas/quartel compartilham 0x00C6F06C, TCs 0x00C6EE8C nesta sessao (enderecos absolutos nao permanentes). Resultado do programa `not_unique_or_inconsistent` e correto: +0x16C **nao distingue casa de quartel**. O offset +0x164 (float32) distingue os tres tipos observados, mas so existe um controle de quartel; manter como **candidato**, nao promover a tipo validado. Novos controles: mais quarteis, moinho e outros edificios em sessao nova, com foco +0x164.
