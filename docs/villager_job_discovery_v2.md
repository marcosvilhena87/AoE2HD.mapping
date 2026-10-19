# Villager Job Discovery v2 — monitoramento automático (19/10/2026)

Implementado no repositório `AoE2HD.control` em `aoe2hdcontrol/villager_job_discovery_v2.py`, CLI `--discover-villager-jobs-v2`.

## Como funciona

Varredura periódica, somente leitura, por `World+0xE8` e `Object_ID`, filtrando proprietário local e classe interna 4. A rotina reaproveita a verificação de identidade da v9.18 e registra eventos `appeared`, `type_changed`, `identity_changed`, `missing_from_scan`, `read_error` e `scan_error`. Capturas com erro não são falsamente marcadas como destruição. Em cada amostra verifica estabilidade das raízes e da tabela.

O tipo Genie é atributo da variante da unidade, **não prova de atividade instantânea**. O monitor não executa ordens, não altera memória, não assume que Parar reverta o tipo. Os rótulos das profissões são candidatos; tipos desconhecidos permanecem `unknown`.

## Mapa experimental de tipos

| Genie Type ID | Rótulo | Evidência |
|---|---|---|
| 83 | male_base | Observado |
| 293 | female_base | Observado |
| 218 | female_woodcutting | Replicado em teste01 e teste02 |
| 581 | female_gold_mining | Replicado em teste01 e teste02 |
| 220 | female_stone_mining | Uma partida, teste02 |

Não extrapolar esses números para todas as variantes, sexos ou versões de Age of Empires II.

## Execução

No Windows, com o jogo aberto e código atualizado:

```powershell
python -m aoe2hdcontrol --live `
  --discover-villager-jobs-v2 `
  --villager-jobs-v2-session teste03 `
  --villager-jobs-v2-samples 60 `
  --villager-jobs-v2-interval 1 `
  --villager-jobs-v2-log villager_jobs_v2.csv
```

A saída é sobrescrita quando o monitor é iniciado novamente: salvar cópias para preservar experimentos anteriores. Duração aproximada de coleta: 60 amostras com intervalos de um segundo, mais o tempo de varredura. Um novo rótulo de sessão deve ser utilizado após reiniciar a partida.

## Próximas validações

Testar mudanças de função e ordens de Parar para aldeões de ambos os sexos; repetir pedra em outra partida; buscar variantes agricultura, construção, reparo e alimentação. Para inferir trabalho *ativo*, é necessário mapear ordem/estado independentemente do tipo Genie. Nenhum novo offset de memória foi validado nesta versão.

## Captura real teste03 (19/10/2026)

Arquivo fornecido `villager_jobs_v2.csv`: 55 registros de eventos ao longo de 60 amostras, sendo 3 `appeared`, 4 `type_changed`, 48 `read_error`; nenhum `scan_error` ou `missing_from_scan`.

Aldeões locais inicialmente encontrados: ID 2104 tipo 293, ID 2105 tipo 293 e ID 2106 tipo 83, classe 4. Somente o ID individual 2106 (objeto 0x143BE7F0 nos eventos) mudou tipos: amostra 7, `83 → 123`; amostra 20, `123 → 579`; amostra 33, `579 → 124`; amostra 57, `124 → 120`. Os tipos 123, 579, 124 e 120 **não possuem função semântica validada ou declarada neste CSV**. Não inferir profissões sem correlacionar com ações/ordens do jogador.

Falhas: IDs 2177, 2178, 2179 e 2180 geraram cada um `Object+0x78 nao e ponteiro plausivel` nas amostras 49..60 (4 x 12 = 48). Isto pode representar incompatibilidade de layout estrutural, não prova de corrupção. Melhoria recomendada: registrar `unsupported_layout` separadamente e agregar falhas repetidas por ID/causa, sem mascarar demais erros.

O monitor associa tipos Genie a variantes da unidade, não a prova da tarefa que está ativa neste instante. Nenhum novo offset foi validado.

## Identificação das quatro tarefas masculinas — teste03 (19/10/2026)

O operador informou explicitamente a sequência de ordens do aldeão masculino de tipo inicial 83, Object ID 2106, já registrada no CSV: madeira → ouro → pedra → alimento em arbusto. Assim, as transições observadas em uma partida passam a ter os seguintes rótulos experimentais:

| Tipo Genie | Atividade informada | Estado da evidência |
|---|---|---|
| 83 | aldeão masculino inicial | observado |
| 123 | corte de madeira (masculino) | identificado pelo operador, uma partida |
| 579 | mineração de ouro (masculino) | identificado pelo operador, uma partida |
| 124 | mineração de pedra (masculino) | identificado pelo operador, uma partida |
| 120 | coleta de comida em arbusto (masculino) | identificado pelo operador, uma partida |

Sequência de tipos `83 → 123 → 579 → 124 → 120` nos eventos 7, 20, 33 e 57 da captura teste03. Esses rótulos são evidência contextual relatada pelo operador, não demonstram exclusividade desses tipos nem atividade instantânea, e requerem replicação em nova partida. Os offsets existentes permanecem com seus estados anteriores.
