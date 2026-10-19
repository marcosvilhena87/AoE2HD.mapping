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

## Evidência da sessão teste01 — ID 2197
O usuário registrou aldeã local com `object_id=2197`, classe 4, tipo Genie 293 quando parada (`idle`), e tipo Genie 218 quando cortando madeira (`woodcutting`). O mesmo ID foi informado nas duas capturas, cada uma com 128 campos. A versão inicial do comparador usava `type_id` na chave e, por isso, mostrou `sem_baseline=128` na segunda captura: **ausência de comparação, não ausência de mudanças**.

O controlador foi corrigido para comparar por `session, object_id, region, offset`, mantendo `type_id` como atributo observado. Não se afirma que 218 seja um código universal de lenhadora: é **tipo experimental associado à atividade nesse teste**. Repetir idle → woodcutting → idle, com a mesma unidade, e replicar com outra unidade e sessão antes de promover a validação.

## Captura real teste01 — resultados analisados (19/10/2026)

CSV `villager_jobs_v1.csv` fornecido pelo operador com 384 linhas: uma captura idle (128 campos, tipo 293), uma captura woodcutting pré-correção (128 campos, sem baseline por chave incluindo tipo) e uma captura woodcutting após correção (128 campos, tipo 218, 38 alterações e 90 campos inalterados). Mesmo `Object_ID=2197`, proprietário local e classe 4 aceita pelo capturador. Não interpretar a captura anterior à correção como 128 campos iguais.

Alterações relevantes da última captura comparadas ao idle:
- `class_definition+0x094` DWORD: `0x00000000 -> 0x00000001` (candidato a estado, não profissão validada).
- `object+0x07C` DWORD: `0xFFFFFFFF -> 0x00000002` (significado desconhecido).
- `class_definition+0x028`: `0xFFFFFFFF -> 713`.
- `class_definition+0x030`: `0xFFFFFFFF -> 613`.
- `class_definition+0x038`: `0xFFFFFFFF -> 15`.
- `class_definition+0x050`: `0xFFFFFFFF -> 710`.
- `class_definition+0x054`: `0xFFFFFFFF -> 610`.

Estas são **observações de valores em offsets já rastreados pelo capturador**, não validações semânticas de novos endereços. O campo de tipo variou de 293 para 218 na mesma identidade durante a tarefa de madeira. A referência idle e a captura posterior ocorreram em instantes diferentes, de modo que deslocamento, relógios e outras variáveis podem explicar parte das 38 diferenças.

Próximos controles: obter captura de volta à inatividade do mesmo ID, repetir ida/volta para lenha; testar outras tarefas e outro aldeão. Preservar logs anteriores ao rodar novos experimentos.

## Retorno ao estado idle: captura real de 19/10/2026

Arquivo fornecido: `villager_jobs_v1(1).csv`, 512 linhas, quatro capturas de 128 campos (`idle` tipo 293; `woodcutting` tipo 218; `woodcutting` tipo 218; `idle` tipo 218), mesmo ID 2197 e sessão `teste01`. A quarta captura foi rotulada manualmente como parada. Em comparação ao primeiro idle, persistiram 34 diferenças (94 campos iguais); em relação ao woodcutting imediatamente anterior, 24 campos mudaram.

Ponto crítico: **o tipo Genie não retornou a 293** quando o operador rotulou a unidade como idle; permaneceu 218. O `class_definition+0x094` também **permaneceu 1** (primeiro idle: 0; woodcutting: 1; segundo idle: 1). Logo, nem tipo 218 nem `+0x094=1` são indicadores confiáveis de *trabalho ativo neste momento* a partir desta amostra; podem representar variante persistente, modo/estado anterior ou outro significado ainda não identificado.

Reversões parciais: `class_definition+0x028` passou de 713 durante woodcutting para `0xFFFFFFFF` no segundo idle; `+0x030` passou de 613 para `0xFFFFFFFF`. Entretanto, `+0x050` mudou de 710 para 713 e `+0x054` de 610 para 613 no segundo idle, em vez de retornar à referência inicial `0xFFFFFFFF`. `class_definition+0x038` permaneceu 15 (no idle inicial era `0xFFFFFFFF`). Interpretar como transição estrutural candidata, jamais código de profissão validado.

Controle seguinte: confirmar visualmente que a unidade realmente está sem ordem/tarefa e registrar vários snapshots após Parar, antes e depois de nova ordem de corte, mantendo ID e sessão. Não presumir mudança imediata da definição de tipo ao parar.

## Segunda rodada em teste01 — sexta captura (19/10/2026)

CSV fornecido: `villager_jobs_v1(2).csv` com 768 linhas de dados, seis capturas de 128 campos cada, mesmo ID individual 2197. Ordem temporal UTC: 03:04:39 idle tipo 293; 03:06:08 woodcutting tipo 218 sem baseline na versão antiga; 03:09:22 woodcutting tipo 218 com 38 alterações versus idle original; 03:11:35 idle tipo 218 com 34 alterações versus idle original; 03:13:40 idle tipo 218 com **1 alteração** versus idle de 03:11:35; 03:14:03 woodcutting tipo 218 com **13 alterações** versus idle de 03:13:40.

Importante: o algoritmo compara cada captura à **última fase idle no CSV** para a mesma sessão e mesmo object_id. Logo, 38, 34, 1 e 13 não compartilham sempre a mesma referência. A associação de phase a atividade é declaração do operador, não confirmação automática.

Na transição mais recente idle → woodcutting (tipo 218 → 218):
- `class_definition+0x028`: `0xFFFFFFFF → 713`; `+0x030`: `0xFFFFFFFF → 613`; `+0x034`: `0xFFFFFFFF → 2309`.
- `class_definition+0x050`: `713 → 0xFFFFFFFF`; `+0x054`: `613 → 0xFFFFFFFF`; `+0x05C`: `0xFFFFFFFF → 15`.
- `class_definition+0x094`: `1 → 2` (também 0 → 1 entre idle inicial e primeira atividade). Isto pode ser um contador/estado progressivo, **não comprova** código de trabalho.
- `object+0x038`: `0x00000300 → 0x00010300`; `object+0x014`, `+0x048` e `class_definition+0x08C` também mudaram; `class_definition+0x03C/+0x040` variaram (candidatos a dados dinâmicos, não interpretar automaticamente como profissão).

A sequência confirma que **existem mudanças correlacionadas com a ordem de coleta sem alteração de tipo Genie no segundo ciclo**, mas não valida semanticamente nenhum campo. Priorizar comparações reversíveis e repetidas de `+0x028/+0x030/+0x050/+0x054`, junto com controles parado, andando sem coletar, e coleta de outro recurso. Não confundir atualização de baseline com reversão de valores de memória.
