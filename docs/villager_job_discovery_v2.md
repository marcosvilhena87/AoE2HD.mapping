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
