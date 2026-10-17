# Evidência de progresso de treinamento de aldeão — 2026-10-17

Fonte: Cheat Engine, tabela XML e resultados de "Find out what writes/accesses this address" fornecidos pelo usuário. Alvo informado: AoK HD.exe (versão 5.8.INT no projeto; confirmar a versão da sessão).

## Endereço observado e tipo

- Endereço absoluto **da sessão**: `0x18769C5C`.
- Cheat Engine: `VariableType=4 Bytes`, `LastState Value=17`; isso **não comprova** armazenamento uint32 nem escala 0–100.
- As instruções observadas leem/escrevem **16 bits** (`ax`/`cx`), indicando um campo `uint16` candidato no endereço, possivelmente com outro campo de 16 bits adjacente.
- `EDI=0x18769C5A` e `ESI/EBX=0x18769C5C` nas operações de zeragem; candidato vizinho em `0x18769C5A`, sem semântica confirmada.

## Evidência estrutural importante

Na instrução `AoK HD.exe+0x318E3A`:

```asm
cmp ax, [ebx+0x00000C44]
EBX=0x18769018
```

A soma `0x18769018 + 0x0C44 = 0x18769C5C` corresponde **exatamente** ao endereço investigado. Portanto **CandidateTrainingBase + 0x0C44** é um offset estrutural candidato `uint16`, sujeito a confirmar que `0x18769018` é de fato a base da estrutura de treinamento e que se mantém estável entre sessões.

## Trechos de código observados (RVAs no executável)

- `+0x4CB0B3`: `mov [esi], ax` depois de `xor eax,eax`; ESI=`0x18769C5C` (zera campo de 16 bits). Na sequência `mov ecx,[ecx+0x150]`, com ECX=`0x18866938` nessa captura.
- `+0x4BE396`: `mov [ebx], ax` depois de `xor eax,eax`; EBX=`0x18769C5C` (zera o mesmo campo). Na sequência `mov ecx,[esi+0x220]`, ESI=`0x18866938`.
- `+0x0BF111`: `mov [eax],cx`; EAX=`0x18769C5C`, CX=`0x0011` (decimal 17), escreve 16 bits.
- `+0x318E3A`: `cmp ax,[ebx+0xC44]`; EBX=`0x18769018`. Compara 16 bits nesse endereço, **não** escreve nele.

Dois blocos de capturas ocorreram entre aproximadamente 18:21:56–18:23:17 no relógio apresentado pelo usuário; registros "First seen/Last seen" não indicam que a fila começou ou terminou nesses instantes.

## Relação com fila e população

O histórico já contém `Building+0x214/+0x218/+0x21C` para fila de produção, porém **não há evidência de que `0x18769018` seja o objeto `Building`**. `CandidateTrainingBase+0x0C44` e `Building+0x214` não devem ser unidos em uma mesma estrutura sem verificação.

Hipótese: campo guarda progresso ou contador relacionado ao treinamento. Alternativas: temporizador, etapa de animação, ID ou estado da interface. Testar leituras `uint16` de `+0x0C44` e `+0x0C42` antes/durante/após treinamento, cancelamento, seleção e reinício. Comparar com o percentual do HUD; uma observação igual a 17 não estabelece relação percentual nem unidade de tempo.

**Status:** offset `+0x0C44` como `hipotese`; endereços absolutos como `sessao_observada`. Código de instrução representa RVAs de funções, não endereços de dados de treinamento.

## Monitor uint16 de dois ciclos — CSV com 486 amostras

Fonte: `training_timeline.csv` recebido do usuário (fase `ciclo_aldeao`, base absoluta de sessão `0x18769018`). Amostragem `0.1 s`; **486/486 leituras ok, zero erros, 54 mudanças exibidas**. Tempo total registrado de `0.0009` a `48.500529` s.

| Transição | Amostra | Tempo relativo (s) | `+0xC44` | `+0xC42` |
|---|---:|---:|---:|---:|
| Inicial/inativo | 1 | 0.0009 | 0 | 65535 (0xFFFF) |
| Ciclo 1 em andamento | 93 | 9.200524 | 1 | 83 (0x0053) |
| Último valor do ciclo 1 | 179 | 17.800662 | 53 | 83 |
| Retorno ao inativo | 185 | 18.400351 | 0 | 65535 |
| Ciclo 2 em andamento | 231 | 23.000598 | 2 | 83 |
| Último valor do ciclo 2 | 394 | 39.300529 | 99 | 83 |
| Retorno ao inativo | 399 | 39.800324 | 0 | 65535 |

O contador `+0xC44` cresce quase linearmente em passos próximos a três a cada 0,5 s, comportamento **fortemente compatível** com progresso percentual 0–100. O campo adjacente `+0xC42` comuta conjuntamente entre 0xFFFF (sentinela de inatividade candidata) e 0x0053 (identificador/estado de unidade candidato). A queda intermediária 53→0 é compatível com cancelamento, e 99→0 é compatível com conclusão, **sem marcas de ação no log**. Nenhum dos dois comportamentos confirma sozinho a semântica exata. A base `0x18769018` continua absoluta da sessão, sem cadeia estável verificada. Recomenda-se confirmar a porcentagem no HUD e comparar uma segunda classe de unidade para validar o papel de 0x0053.

## Investigação da semântica de 0x0053 (83) — referência de unidades

Foi pesquisado o catálogo atual e o código histórico do próprio repositório: `TrainingCandidateBase+0xC42` continua registrado apenas como hipótese, e os leitores históricos de fila (`ProductionQueueEntriesV6.h`, `ProductionQueueEnumerationV7.h`) decodificam `uint16 type` e `uint16 quantity`, **sem tabela local que converta o número 83 em nome de unidade**.

Referências externas independentes identificam `83 (0x0053)` como **Villager (Male)** (aldeão masculino):

- https://aoe2control.github.io/enums/ (`UnitObjectType.VILLAGER_MALE = 83`);
- https://airef.github.io/tables/objects.html (`83 Villager Male`, `293 Villager Female`, `74 Militia`);
- https://ksneijders.github.io/AoE2ScenarioParser/api/AoE2ScenarioParser/datasets/units.html (`VILLAGER_MALE=83`, `VILLAGER_FEMALE=293`).

**Inferência atual**: o valor `+0xC42 = 0x0053` em ambos os treinamentos de aldeão é compatível com **ID do tipo de unidade em treinamento**, e `0xFFFF` fora de treinamento é compatível com sentinela de fila vazia/nenhuma unidade ativa. A evidência reforça a hipótese, mas **não a valida diretamente na versão HD 5.8.INT**: tabelas externas podem descrever interfaces/versões diferentes e coincidência com um único tipo não exclui estados alternativos. Confirmar com milícia (ID de referência 74 / 0x004A) e, se possível, outras unidades; monitorar alternância +0xC42 de 0xFFFF ao ID durante a fila, retorno em cancelamento/conclusão e comportamento após reinício. Não alterar o status `hipotese` sem validação.

## Replicação com milícia — evidência independente do ID de unidade

Fonte: `training_timeline(1).csv` enviado pelo usuário, contendo **721 linhas de amostras** acumuladas por append: 486 da sessão anterior de aldeão e 235 da sessão de milícia. A segunda sessão reinicia `sequence=1` e `elapsed_seconds` do zero. Portanto **não combinar os tempos relativos nem inferir continuidade de uma única execução**. São 721 leituras `ok`, nenhum erro. O usuário informa que **treinou uma milícia**.

Na nova sessão de milícia (235 amostras):

| Evento | Amostra | Tempo relativo (s) | `+0xC42` uint16 | `+0xC44` uint16 |
|---|---:|---:|---:|---:|
| Antes | 1 | ~0.0 | 65535 (0xFFFF) | 0 |
| Início observado de progresso | 49 | ~4.8 | **74 (0x004A)** | 2 |
| Último valor crescente | 181 | ~18.0 | **74 (0x004A)** | 96 |
| Retorno à inatividade | 186 | ~18.5 | 65535 (0xFFFF) | 0 |

Há **137 amostras** com `+0xC42=74`, 98 com `+0xC42=65535` na sessão da milícia. A sessão anterior com aldeão possui 260 amostras `+0xC42=83` e 226 com `+0xC42=65535`; total acumulado: 137 (ID 74), 260 (ID 83), 324 (sentinela 65535). Coincidência específica entre o tipo de unidade treinado e o valor observado: aldeão masculino **83/0x0053**, milícia **74/0x004A** (ver links externos e ressalvas acima).

**Conclusão delimitada:** `TrainingCandidateBase+0xC42` é agora candidato fortemente corroborado de **ID do tipo de unidade na produção ativa**; `+0xC44` é candidato fortemente corroborado de progresso percentual (ciclo crescente até 96, depois zero). O retorno simultâneo a 0xFFFF/0 é compatível com fila/produção inativa. Ainda não há confirmação da cadeia de ponteiros, reidentificação após reiniciar, equivalência exata ao indicador de HUD em múltiplos pontos, nem controles para unidades diferentes e várias entradas na fila. **Manter status de catálogo `hipotese`**, embora com evidência observacional forte.
