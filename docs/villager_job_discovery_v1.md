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

## Terceira transição observada — mineração (19/10/2026)

Na mesma sessão `teste01`, aldeã individual ID 2197, classe 4 previamente confirmada, após atribuição de mineração de ouro (`phase=mining`), o capturador reportou **tipo Genie 581**, 128 DWORDs, 34 diferentes do último baseline idle, zero campos sem referência, `tipos_idle=[218,293]`, `tipo_alterado=1`. Histórico observado: idle 293 → woodcutting 218 → stop/idle 218 → woodcutting 218 → mineração de ouro 581.

**Hipótese reforçada, não validada como regra geral:** tipo Genie pode refletir a função/variante atribuída e persistir quando se emite Parar. O valor 581 é candidato a variante na mineração de ouro, e não ainda prova de código exclusivo dessa tarefa. A comparação de 34 campos se refere ao último baseline idle do tipo 218; não é comparação direta com a última amostra woodcutting. Recomenda-se mandar Parar e testar permanência de 581, depois mineração de pedra, caminhada e retorno à madeira, capturando mesmo ID e sessão. Nenhum endereço novo validado.

## Replicação entre partidas — teste02 (19/10/2026)

Nova partida, aldeã local `object_id=2188` classe 4. Captura `idle` com tipo Genie 293 (128 campos, baseline inaugural); após ordem de cortar madeira, captura `woodcutting` apresentou tipo Genie 218, **35/128 campos diferentes** da referência idle, nenhum sem baseline, `tipos_idle=[293]` e `tipo_alterado=1`.

Comparação com `teste01`: na partida anterior ID 2197 mudou 293 → 218 com 38/128 campos diferentes na primeira comparação válida. Portanto a **transição de tipo 293 → 218 associada à atribuição de corte de madeira foi reproduzida em duas partidas**, com IDs individuais diferentes. Ainda não comprova que 218 seja código universal de profissão, nem identifica por si só atividade instantânea. Captura bruta detalhada de teste02 não foi fornecida; não inferir offsets específicos entre os 35 alterados.

Próximo controle: pressionar Parar e observar se o tipo 218 persiste, depois comparar ordem de caminhada e nova profissão (ouro/pedra), mantendo sessão teste02 e ID 2188.

## teste02 — persistência após ordem Parar (19/10/2026)

Aldeã local ID 2188: fase idle inicial tipo 293 (128 campos, sem baseline); fase woodcutting tipo 218 (35 diferenças contra idle original); após botão Parar, nova captura rotulada idle permaneceu tipo 218 (33 diferenças contra idle original, 0 sem baseline, tipos_idle=[293], tipo_alterado=1). A sequência 293 → 218 → 218 replica a persistência observada na partida teste01 ID 2197. Evidência de que a ordem Parar **não restaurou imediatamente** o tipo 293 nas duas partidas. Não prova a regra universal de mudança somente sob atribuição nova. Valores detalhados de offsets entre as 33 diferenças não foram fornecidos nesta rodada. Próximo controle: atribuir mineração de ouro e verificar repetição 218 → 581; depois Parar e verificar persistência.

## Replicação de mineração de ouro — teste02 (19/10/2026)

Na nova partida, a aldeã local de ID 2188 apresentou a sequência: idle tipo Genie 293 (128 campos sem baseline), woodcutting tipo 218 (35 diferenças em relação ao idle inicial), idle após Parar tipo 218 (33 diferenças em relação ao idle anterior), mineração de ouro tipo 581 (37 diferenças em relação ao último idle, nenhum campo sem baseline, `tipos_idle=[218,293]`, `tipo_alterado=1`). Assim, a transição associada à atribuição de ouro 218 → 581 foi reproduzida em segunda partida, em aldeã distinta da ID 2197 da teste01.

**Evidência replicada:** 293 associado à variante feminina inicial, 218 associado à função de madeira, 581 associado à mineração de ouro; Parar não restaurou imediatamente o tipo inicial após madeira nas duas partidas. Ainda não se demonstrou exclusividade de 581 para ouro versus pedra, nem que o tipo represente trabalho ativo; os números de diferenças dependem do baseline atualizado a cada idle. Esta atualização registra apenas resultados apresentados no terminal; o CSV detalhado de teste02 não foi disponibilizado para auditoria dos offsets.

## teste02 — primeira observação de mineração de pedra (19/10/2026)

Aldeã local ID individual 2188, classe 4: depois da atribuição de ouro (tipo Genie 581), o operador atribuiu mineração de pedra e executou novamente o modo `--villager-jobs-v1-phase mining`. Resultado: **tipo Genie 220**, 128 campos, 38 diferenças em relação ao último baseline idle, zero sem baseline, `tipos_idle=[218,293]`. Sequência desta sessão: idle 293 → woodcutting 218 → idle/Parar 218 → mineração de ouro 581 → mineração de pedra 220.

Interpretação: tipos 581 e 220 são candidatos **distintos** a funções de mineração de ouro e pedra para aldeã feminina. O tipo 220 foi observado apenas nesta partida; sem replicação independente e sem prova de exclusividade. Ambas as capturas foram rotuladas `mining`, logo a distinção ouro/pedra consta da declaração contextual do operador, não do rótulo CSV. Não promover offset novo nem afirmar trabalho em execução apenas pelo tipo persistente. Próximo aprimoramento do controlador: separar rótulos `mining_gold` e `mining_stone`.
