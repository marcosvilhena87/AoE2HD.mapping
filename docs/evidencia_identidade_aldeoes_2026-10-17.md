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
