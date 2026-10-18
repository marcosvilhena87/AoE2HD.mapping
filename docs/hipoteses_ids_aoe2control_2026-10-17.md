# Hipoteses de IDs do AoE2Control DE para o AoE2 HD

Fonte: https://aoe2control.github.io/enums/
A API do Definitive Edition define classes e tipos numericos; nao ha garantia de offsets ou codificacao iguais na HD 5.8.INT.

| Categoria | Enum API DE | Numero API | Hipotese interna |
|---|---|---:|---:|
| Aldeao | UnitClass.VILLAGER | 904 | 4 |
| Construcao | UnitClass.BUILDING | 903 | 3 |
| Infantaria | UnitClass.INFANTRY | 906 | 6 |
| Lanceiro/classe | UnitClass.SPEARMAN | 950 | 50 |
| Aldeao masculino/tipo | UnitObjectType.VILLAGER_MALE | 83 | 83 |
| Milicia/tipo (referencia) | UnitObjectType MILITIA | 74 | 74 |
| Lanceiro/tipo (referencia) | UnitObjectType SPEARMAN | 93 | 93 |

A conversao classe interna = classe da API - 900 e apenas uma HIPOTESE para leitura de memoria HD. Valores 4/904/83 sao dominios distintos. Um match numerico nao prova tipo ou classe e nao revela nenhum offset.

## Evidencia e testes necessarios

Nos testes HD anteriores, tres aldeoes tinham Object+0x78 apontando para estruturas com valor uint32 4 em +0x0C e +0x94. Milicia e lanceiro mostraram low16 74 e 93 em algumas leituras a partir de Object+0x0C. Esses achados nao identificam semanticamente os campos.
Repetir em nova sessao e comparar aldeoes em diferentes atividades, milicia, lanceiro e edificios. Exigir exclusividade e estabilidade dos campos; incluir falsos positivos como controle. Nao validar offsets so por correspondencia com numeros do DE.

## Integracao ao scanner

O modulo unit_definition_pointer_discovery.py passa a rotular resultados candidatos como class_candidate ou type_candidate, preservando os valores inteiros de 32 e 16 bits. Esses rotulos nao alteram o catalogo nem classificam unidades automaticamente.
