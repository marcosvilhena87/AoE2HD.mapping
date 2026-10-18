# Hipótese de correspondência entre UnitClass XS e classe interna (2026-10-18)

## Relação observada

A API Lua do AoE2Control (Age of Empires II: Definitive Edition) documenta `UnitClass.BUILDING=903`, `UnitClass.VILLAGER=904` e `UnitClass.INFANTRY=906`. A rotina experimental `unit_class_validation_v2` do `AoE2HD.control` (Age of Empires II HD 5.8.INT, x86) retornou respectivamente `3`, `4` e `6` para construção, três aldeões e duas unidades de infantaria (milícia/lanceiro), com correspondência aos valores esperados conforme log apresentado pelo usuário em 2026-10-17.

| Classe | Valor experimental HD | UnitClass AoE2Control | Diferença |
|---|---:|---:|---:|
| Construção | 3 | 903 | +900 |
| Aldeão | 4 | 904 | +900 |
| Infantaria | 6 | 906 | +900 |

**Hipótese de tradução:** `class_xs = class_internal + 900`, ou `class_internal = class_xs - 900`. A documentação de XS da Forgotten Empires descreve explicitamente as funções `xsGetUnitClass` e `xsGetObjectClass` como retornando a classe acrescida de 900, fazendo dessa convenção algo documentado na camada XS. Isso **não prova** que o AoE2Control realiza a mesma soma internamente, nem que classes de todas as versões do jogo correspondam sem exceções.

## Evidências e fontes

- AoE2Control — enum `UnitClass`: https://aoe2control.github.io/enums/
- Forgotten Empires — XS scripting, `xsGetUnitClass` e `xsGetObjectClass` (`+900`): https://www.forgottenempires.net/age-of-empires-ii-definitive-edition/xs-scripting-in-age-of-empires-ii-definitive-edition
- Experimento local relatado pelo usuário (2026-10-17): `villager_A=4`, `villager_B=4`, `villager_C=4`, `militia=6`, `spearman=6`, `building=3`. A saída de terminal foi apresentada na conversa; não se afirma possuir CSV anexado ou validação entre reinícios.

## Previsões ainda não testadas no HD

| UnitClass na API | Previsão HD | Observação |
|---|---:|---|
| CAVALRY (912) | 12 | Não validado |
| SIEGE_WEAPON (913) | 13 | Não validado |
| TREE (915) | 15 | Não validado |
| FISHING_SHIP (921) | 21 | Não validado |
| TOWER (952) | 52 | Não validado |

**Correção de nomenclatura:** no enum do AoE2Control, `918` é `MONASTERY` (mosteiro), não `MONK`. Portanto, `918 -> 18` não deve ser usado como previsão para um monge sem confirmação independente.

## Protocolo de validação

1. Em `AoK HD.exe 5.8.INT`, selecionar individualmente unidades e objetos de classes adicionais, com confirmação visual de seu tipo e identidade.
2. Ler a classe da definição da unidade pelo fluxo já implementado, guardando ID individual, owner, objeto, ponteiro de definição, classe observada, classe XS esperada, versão e contexto da captura.
3. Verificar correspondências e não correspondências, controles negativos (outra classe/unidade), estabilidade ao desmarcar e repetição após reinício da partida e do processo.
4. Registrar discrepâncias e exceções, sem forçar conversão em casos desconhecidos.
5. Atualizar catálogo de offsets apenas se houver **novo endereço ou offset** descoberto; a relação `+900` é uma regra de interpretação de valores, não um endereço de memória.

## Estado de confiança

- **Documentado:** a convenção `+900` na interface XS descrita pela Forgotten Empires.
- **Observado experimentalmente:** correspondência para classes internas 3, 4 e 6 na versão HD indicada.
- **Hipótese pendente:** validade geral no HD, possíveis exceções e mecanismo interno exato do AoE2Control.

Não promover offsets a validados apenas por esta hipótese. Não mudar o formato ou a contagem do CSV de endereços por se tratar de documentação sem novo endereço identificado.
