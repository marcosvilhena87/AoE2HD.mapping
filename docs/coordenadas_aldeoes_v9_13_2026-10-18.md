# Coordenadas dos aldeões — evidência v9.13 (18/10/2026)

## Origem
Experimento de leitura, somente observação, usando o AoE2HD.control:
`--monitor-villager-coordinates-v9-13`.
Foram registradas 20 amostras de três objetos de tipo Genie 83, identificados pelos IDs individuais 2001, 2002 e 2003. O relatório teve 60 linhas de posições, sem erros de leitura comunicados.

## Endereços candidatos

| Expressão relativa | Formato | Interpretação experimental | Estado |
| --- | --- | --- | --- |
| `Object+0x3C` | `float32` | Coordenada X candidata | forte candidato |
| `Object+0x40` | `float32` | Coordenada Y candidata | forte candidato |

Os objetos são resolvidos pela tabela `World+0xE8`, com verificação do ID em `Object+0x08` e do tipo `83` em `[Object+0x0C]+0x10`. Os offsets são relativos ao ponteiro do objeto, não endereços absolutos estáveis.

## Observações

| ID individual | +0x3C inicial → final | +0x40 inicial → final | Comportamento |
| --- | --- | --- | --- |
| 2001 | 14,5000 → 23,9375 | 41,5000 → 48,2708 | Movimento detectado nas amostras 6–19 |
| 2002 | 14,5000 → 14,5000 | 35,5000 → 35,5000 | Parado nas 20 amostras |
| 2003 | 21,5000 → 21,5000 | 34,5000 → 34,5000 | Parado nas 20 amostras |

O ID 2001 teve deslocamento compatível com duas pernas em direções diferentes. Os controles parados mantiveram valores constantes, reforçando a hipótese de coordenadas de posição e reduzindo a possibilidade de serem temporizadores de animação.

## Limitações e próximos controles

- A associação formal a X e Y do mapa, o sentido dos eixos e a escala em tiles ainda não foram validados com pontos de referência visuais conhecidos.
- A evidência se aplica aos aldeões do tipo 83 observados, não automaticamente a todas as unidades.
- Confirmar com um segundo aldeão em deslocamentos controlados e posições conhecidas antes de promover para `validado`.
- Não confundir IDs individuais da partida com IDs de tipo nem promover ponteiros absolutos de sessão ao catálogo de offsets.
