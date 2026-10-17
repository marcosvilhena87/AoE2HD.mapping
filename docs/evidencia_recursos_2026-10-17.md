# Evidência dinâmica de recursos — 2026-10-17

## Ambiente e proveniência
- Jogo declarado pelo monitor: Age of Empires II HD 5.8.INT, `AoK HD.exe` (x86).
- Origem: transcrição de PowerShell enviada pelo usuário em 2026-10-17, executando `python -m aoe2hdcontrol --live --monitor --interval 0.5 --output recursos_validacao.jsonl`.
- Diretório observado: `C:\Minha_Pasta\Jogos\AoE_II_HD\AoE2HDControl` (repositório local legado, **não** `AoE2HD.control`).
- Intervalo amostrado: 20:15:54.376 UTC em diante; 149 snapshots JSON válidos analisados. A transcrição terminou com `KeyboardInterrupt`; não houve registro de `monitor_summary` nessa execução.
- Resultado: 149 leituras `status=ok`, zero `read_error`, 8 eventos `resource_changed` e um `initial_snapshot`.
- `World` na sessão: `0x142CDD08`. Jogador local índice 1, três entradas (0, 1, 2). **Endereços absolutos apenas desta sessão, não estáveis**.

## Evidência de mudanças
| Amostra | Índice | Recurso | Antes | Depois | Delta |
|---:|---:|---|---:|---:|---:|
| 87 | 2 | food | 150 | 100 | -50 |
| 93 | 2 | food | 100 | 40 | -60 |
| 93 | 2 | gold | 100 | 25 | -75 |
| 95 | 1 | wood | 200 | 220 | +20 |
| 116 | 2 | food | 40 | 60 | +20 |
| 119 | 1 | gold | 100 | 120 | +20 |
| 129 | 1 | food | 200 | 220 | +20 |
| 137 | 2 | food | 60 | 80 | +20 |

A tabela de recursos é alcançada pela cadeia histórica `RVA Owner global -> Owner +0xC8 (World) -> World +0x184/0x188 (Players) -> PlayerEntry Object -> WorldPlayer +0x3C (ResourceTable)`; os campos relativos são `ResourceTable +0x00/+0x04/+0x08/+0x0C` interpretados como float32 (`food/wood/stone/gold`).

## Limitações e próxima validação
- Evidência **dinâmica observada**, não comprovação independente da semântica. Não há comparação visual das quantidades na interface nem registro de qual ação foi realizada, e não foi testado reinício do processo neste experimento.
- Conservar os atuais status `historico_validado` e as cautelas de documentação; **não promover** por estes resultados a `validado_na_sessao` nem inferir nomes de jogadores ou causa dos gastos.
- Próxima prova: registrar valores na interface antes/depois de ações controladas (coleta de madeira, gasto de alimento e ouro), com controles negativos e nova execução após reinício do jogo.

## Evidência adicional — monitor v0.2.0 no repositório novo

- Fonte: transcrição de PowerShell fornecida pelo usuário, execução `--live --monitor --interval 1 --count 90 --output monitor.jsonl`, iniciada às 20:23:27 UTC em 2026-10-17, interrompida por Ctrl+C às 20:24:04 UTC.
- 37 amostras bem-sucedidas; 0 erros de leitura; registro final `monitor_summary` de 11 eventos incluindo `initial_snapshot` e duração 36,281 s.
- World nesta sessão: `0x102AA290`; jogador local índice 1; três entradas. Endereços absolutos são específicos da sessão.
- Mudanças observadas entre amostras: jogador 2 madeira 55→15 (-40), 15→35 (+20), alimento aproximadamente 221,6103→261,6103 (+40), 261,6103→281,6103 (+20), 281,6103→301,6103 (+20); ouro 45→65 (+20). Jogador 1 madeira 300,0089→320,0089 (+20) e 320,0089→340,0089 (+20); ouro 180→200 (+20); alimento 260→280 (+20).
- A precisão decimal não inteira é compatível com leitura float32, mas não demonstra correspondência com valores de interface. Pedra permaneceu constante (200) para ambos os jogadores.
- Não foram adicionados offsets: esta seção documenta apenas evidência dinâmica adicional para os offsets históricos existentes. Sem promoção de status. A prova visual e testes controlados após reinício continuam pendentes.
