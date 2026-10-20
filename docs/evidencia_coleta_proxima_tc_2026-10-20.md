# Evidência experimental — coleta de madeira próxima do Centro da Cidade (20/10/2026)

Binário investigado: AoK HD.exe 5.8.INT, x86. Origem: execução local do usuário e CSVs de medição de âncora, Environment Anchor Validation v2 e ranking nearest_tc_resource_v1.

## Objetos e IDs da sessão (não são constantes globais)

- TC local: Object ID 2007; tipo Genie 109; coordenadas de mundo candidatas (75.0, 35.0).
- Aldeão local: Object ID 2017, tipo Genie 83; coordenadas candidatas (78.197914, 37.364582).
- Pinheiros com neve tipo Genie 413, mapeados como candidatos a madeira nesta partida:
  - Object 2130: (71.5, 35.5); distância euclidiana candidata ao TC 3.54.
  - Object 2131: (71.5, 38.5); distância 4.95.
  - Object 2134: (81.5, 34.5); distância 6.52.
  - Object 2136: (68.5, 35.5); distância 6.52.
  - Object 2135: (67.5, 40.5); distância 9.30.
- A classificação 413 como Pinheiro com Neve está sustentada por observação anterior de HUD e queda do contador de madeira na sessão, mas não é garantida em todos os mapas.

## Campos e cadeias usados no controlador

- World+0x0E8: tabela de ponteiros indexada por Object ID, entrada ID * 4 (registro histórico no catálogo).
- Object+0x08: Object ID (candidato; checagem contra ID da entrada).
- Object+0x0C -> ObjectTypeTarget+0x10: Genie Type ID, a depender da estrutura de definição lida.
- Object+0x10: proprietário candidato.
- Object+0x3C e Object+0x40: coordenadas X/Y candidatas em float32; coerência espacial na sessão, mas não universalmente validadas para todos os objetos.
- Object+0x48: estoque restante de recurso candidato em float32, com indícios de diminuição durante coleta de madeira; semântica não estabelecida para todos os tipos e estados de objeto.

A seleção de recurso é por distância euclidiana entre candidatos no mundo, **não** por caminho real, acessibilidade ou tempo de coleta.

## Âncoras e limites

A captura 1313 x 533 registrou sete pinheiros distintos (quatro treino e três holdout) sob `capture_id=neve_1313x533_01`. RMSE nas três árvores holdout: projeção original 7.418 px, deslocamento constante 16.747 px, correção espacial 17.119 px (limite 8 px). Isso não habilita cliques: screenshots e coordenadas de desktop não são equivalentes, e a identidade visual foi inserida manualmente.

A varredura de aldeões reportou quatro erros em objetos irrelevantes, com primeiro erro Object ID 2098: `Object+0x78 nao e ponteiro plausivel`. O ranking foi gerado a partir de inventário parcial; não há prova de que a árvore 2130 seja a mais próxima entre **todos** os recursos acessíveis.

## Comportamento de esgotamento

O operador relata que o jogo normalmente procura nova fonte do mesmo recurso após o esgotamento; esse comportamento é responsabilidade do jogo, não há endereço de memória validado para a lógica interna de pathfinding ou mudança de alvo. A automação atual envia, no máximo, a ordem inicial supervisionada, sem confirmação causal posterior de coleta.

## Critério para evolução

Não elevar `Object+0x3C/+0x40` ou `Object+0x48` para status `validado` somente com esses dados. Repetir em partidas/câmeras e registrar observação independente do início da coleta e do consumo.
