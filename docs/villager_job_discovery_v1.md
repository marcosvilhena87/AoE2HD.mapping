# Villager Job Discovery v1 — protocolo experimental

Implementado em `AoE2HD.control` com `--discover-villager-jobs-v1` (somente leitura). Não há **offset de trabalho validado** até o momento.

## Evidência reutilizada
- `World+0xE8`: índice de objetos por ID individual, entrada `ID*4`.
- `Object+0x08`: conferir identidade individual.
- `Object+0x10`: jogador proprietário (igual ao jogador local).
- `[Object+0x0C]+0x10` (uint16): tipo Genie, **não** profissão.
- `[Object+0x78]+0x0C` (uint32): classe 4 para aldeão.
- `[Object+0x78]+0x94` (uint32): campo candidato que variou 5→6 após mudança de atividade. **Sem semântica comprovada**.

## Execução no Windows

Reutilizar o mesmo ID enquanto o aldeão continua vivo e o mesmo processo do jogo está aberto. Informar um rótulo de sessão novo a cada reinício/partida. Antes de mandar executar outra tarefa:

```powershell
python -m aoe2hdcontrol --live --discover-villager-jobs-v1 --villager-jobs-v1-id 2001 --villager-jobs-v1-phase idle --villager-jobs-v1-session teste01
```

Depois, quando o **mesmo** aldeão estiver efetivamente cortando madeira:

```powershell
python -m aoe2hdcontrol --live --discover-villager-jobs-v1 --villager-jobs-v1-id 2001 --villager-jobs-v1-phase woodcutting --villager-jobs-v1-session teste01
```

Outras fases: walking, carrying, depositing, building, farming, mining, repairing, other. Um ID como 2001 é apenas **exemplo**, não fixo. Usar `--villager-jobs-v1-log caminho.csv` se necessário; janela padrão 256 bytes por região, até 512 via `--villager-jobs-v1-window`.

## Saída
`villager_jobs_v1.csv` armazena uma linha por DWORD de `Object` e da estrutura `Object+0x78`, incluindo hex bruto, interpretação uint32 candidata, fase, ID, tipo, sessão, offset e `changed` relativo à fase `idle` da mesma sessão/ID/tipo/região. Sem baseline, `changed` fica vazio. Snapshot é descartado se os ponteiros ou raízes mudarem; leitura parcial causa erro, não promoção de campos. O registro de fase é declarado manualmente pelo operador e NÃO é prova automática de tarefa.

## Próximo critério de validação
Comparar várias transições do mesmo aldeão, controles parados, outros aldeões, reabertura da partida; distinguir ação/ordem/alvo de indicadores de movimento e relógios. Somente adicionar novos endereços ao catálogo após inferência fundamentada, sem substituir candidatos ou promover `+0x94` automaticamente.
