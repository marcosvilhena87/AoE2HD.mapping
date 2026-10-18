# Unit Type Validation v8 — replicação entre sessões (18/10/2026)

## Escopo
Teste somente leitura no AoE2 HD investigado, cadeia `Object+0x0C -> ponteiro -> +0x10` como uint16. Resultado informado pelo executor: `consistent_new_session_candidate`, etiqueta `2026-10-18-restart-2`, confirmação manual `REINICIEI`. O reinício não é verificável automaticamente pelo programa.

## Capturas
| Etapa | ID individual | Classe | Tipo em +0x10 | Situação |
|---|---:|---:|---:|---|
| Casa | 2294 | 3 | 70 | Esperado 70 |
| Centro da Cidade | 1991 | 3 | 109 | Esperado 109 |
| Aldeão A | 2001 | 4 | 83 | Esperado 83 |
| Aldeão B | 2003 | 4 | 83 | Esperado 83 |
| Milícia | 2301 | 6 | 74 | Esperado 74 |
| Lanceiro | 2302 | 6 | 93 | Esperado 93 |
| Batedor | 2007 | 47 | 448 | Esperado 448 |
| Quartel | 2292 | 3 | 12 | Esperado 12 |
| Moinho | 2297 | 3 | 68 | Esperado 68 |
| Armazém de madeira | 2298 | 3 | 562 | Esperado 562 |
| Arqueiro (opcional A) | 2327 | 0 | 4 | Descoberta observada |
| Monge (opcional B) | 2317 | 18 | 125 | Descoberta observada |

Os dez controles esperados coincidiram; as duas unidades opcionais não foram verificadas contra valores previamente configurados. Todos os 12 IDs individuais são diferentes.

## Separação dos campos vizinhos
Os valores reportados no CSV da v8 mostram que `Target+0x12` **não é cópia invariável** do ID de tipo:
| Construção | +0x10 | +0x12 | +0x14 |
|---|---:|---:|---:|
| Casa | 70 | 464 | 70 |
| Centro da Cidade | 109 | 141 | 109 |
| Quartel | 12 | 132 | 12 |
| Moinho | 68 | 130 | 68 |
| Armazém de madeira | 562 | 564 | 562 |

`+0x14` coincidiu com `+0x10` nos doze controles, porém a equivalência semântica permanece não demonstrada. `+0x12` deve ser estudado separadamente; não utilizá-lo como ID do tipo.

## Interpretação e limites
As experiências v6/v7/v8 oferecem forte validação funcional, em classes diferentes e numa segunda sessão declarada, para **`[Object+0x0C]+0x10` uint16 como ID específico de tipo**. Os ponteiros absolutos e IDs individuais variam por partida; não persistir tais endereços como estáticos. A validação é limitada à versão HD investigada e aos controles testados; não extrapolar para Definitive Edition. O campo `Object+0x0C` é o ponteiro usado no caminho; `Object+0x78 -> +0x0C` permanece caminho distinto da classe.

Fontes: saída de console e CSV `unit_type_validation_v8.csv` fornecidos pelo executor; v6 e v7 documentadas em `evidencia_identidade_aldeoes_2026-10-17.md`.

## Integração no controlador — Unit Type Integration v9

O repositorio AoE2HD.control implementa `--integrate-unit-types-v9`, inventario automatico somente leitura pela tabela `World+0xE8`, sem selecao manual. Le `Object+0x08` (ID individual), `Object+0x10` (dono candidato), `Object+0x0C -> +0x10` (ID tipo uint16, validado funcionalmente nos experimentos anteriores) e `Object+0x78 -> +0x0C` (classe uint32). Registra em CSV tipo/classe/owner; tipos desconhecidos permanecem no relatorio. Por padrao considera apenas objetos de dono local, com janela limitada de IDs; escopo opcional `all` inclui outros donos. Falhas de leitura sao separadas em `read_error` e nao contam como tipos descobertos. O modulo v9 ainda aguarda CSV experimental; nao cria novos offsets.

## Unit Type Integration v9 — resultados reais

CSV local (`unit_types_v9.csv`) analisado: 19 objetos identificados e quatro `read_error`. CSV todos os proprietarios (`unit_types_v9_all.csv`): 68 objetos identificados e 2197 `read_error`. Todas as falhas dos dois arquivos dizem `Object+0x78 nao e ponteiro plausivel`; tal falha indica incompatibilidade com o layout de definicao presumido pelo scanner, nao necessariamente unidade com memoria corrompida. No escopo local, seis tipos ainda sem rotulo: objetos 1992/1993/1994 classe 3 tipos 618/619/620; 2308 classe 3 tipo 87; 2309 classe 3 tipo 101; 2315 classe 3 tipo 104. No escopo all, proprietarios classificados por ponteiro: 0x14A92AE4 (local, 19), 0x1877EFB4 (22), 0x10643344 (27). Este ultimo grupo inclui classes 9/10/42 e tipos 65/48/126/285, possivelmente objetos ambientais/Gaia; semantica pendente. Proximo aprimoramento: distinguir `unsupported_layout` de `read_error`, registrar analise de tipos desconhecidos separadamente e validar a associacao de owners. Nao introduzir nomes especulativos no catalogo.
