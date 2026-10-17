# Evidência de população — 2026-10-17

AoE2 HD 5.8.INT, sondagem somente leitura guiada pelo HUD. Estados inseridos: população usada/limite 4/5, 4/10, 5/10 e 6/10. O arquivo population_candidates.csv enviado contém duas correspondências float32, ambas para população usada: ResourceTable+0x02C e ResourceTable+0x04C, quatro de quatro observações e duas de duas transições. Nenhuma correspondência para limite populacional. Estes offsets são hipóteses, não endereços revalidados. Próximos testes: aumento e redução de população, unidades em fila, limite alterado sem mudança na população, reinício do processo e comparação visual.

## Hipótese adicional: vagas disponíveis

Hipótese proposta pelo usuário: o limite apresentado no HUD pode ser derivado de `população usada + vagas disponíveis`, em vez de estar armazenado diretamente. Para os estados 4/5, 4/10, 5/10, 6/10 a sequência esperada de vagas é 1, 6, 5, 4. Examinar também `limite - usada`, sem considerar qualquer coincidência prova de semântica. Distinguir população em fila e variações de capacidade por edificações. Status: hipótese não validada.

## Sondagem v2 — vagas disponíveis observadas

CSV `population_candidates_v2.csv` enviado pelo usuário: `ResourceTable+0x010`, `float32`, acompanhou `1|6|5|4` para `free_slots`, 4/4 observações e 3/3 transições; `ResourceTable+0x02C` e `+0x04C`, `float32`, acompanharam `4|4|5|6` para `used`, 4/4 observações e 2/2 transições. Hipótese: `limite = usada + vagas disponíveis`. Ainda não verificado após reinício ou sob fila de produção; sem confirmação semântica definitiva.

## Sondagem adicional — seis estados (3 aldeões e 1 batedor iniciais)

Relato do usuário: início com 4 unidades (3 aldeões e 1 batedor), construção de uma casa e treinamento de mais 4 aldeões. CSV `population_candidates(1).csv`, 6 estados do HUD: `4/5`, `4/10`, `5/10`, `6/10`, `7/10`, `8/10`.

| Campo | Offset | Tipo | Série observada | Correspondência | Transições |
|---|---|---|---|---|---|
| Vagas (`limit-used`) | `ResourceTable+0x010` | `float32` | `1,6,5,4,3,2` | 6/6 | 5/5 |
| População usada A | `ResourceTable+0x02C` | `float32` | `4,4,5,6,7,8` | 6/6 | 4/4 |
| População usada B | `ResourceTable+0x04C` | `float32` | `4,4,5,6,7,8` | 6/6 | 4/4 |

A fórmula `limite = vagas + população usada` reproduziu os seis valores do HUD. Não indica por si só se `+0x02C` e `+0x04C` possuem semânticas diferentes (p.ex. população real e outro contador). Sem testes com unidade em fila, cancelamento, morte, diferentes tipos de unidade ou nova execução após reinício. Preservar os três como `hipotese`; não converter correlação em validação definitiva.

## Replicação com milícias — seis estados

Fonte: `population_candidates(2).csv` enviado pelo usuário e log da sondagem. Situação informada: três aldeões e um batedor iniciais, construção de uma casa e treinamento de quatro milícias. Estados HUD: `4/5`, `4/10`, `5/10`, `6/10`, `7/10`, `8/10`.

- `ResourceTable+0x010`, float32, `free_slots`: `1|6|5|4|3|2` (6/6 estados; 5/5 transições).
- `ResourceTable+0x02C`, float32, `used`: `4|4|5|6|7|8` (6/6 estados; 4/4 transições).
- `ResourceTable+0x04C`, float32, `used`: `4|4|5|6|7|8` (6/6 estados; 4/4 transições).

Replicação observacional em outra composição de unidades (milícias, não aldeões). Ainda não distingue `+0x02C` de `+0x04C`, nem testa fila de produção, cancelamento, perdas de unidades ou comparação temporal antes/depois de reinício do processo. Os três continuam hipóteses, sem promoção de status.

## Teste antes/durante/depois de treinar aldeão

Fonte: log PowerShell e `population_candidates(3).csv` fornecidos pelo usuário. Estados anotados: antes `4/5`, durante `4/5`, depois `5/5`. Os três candidatos persistiram em 3/3 estados e uma transição: `ResourceTable+0x010` (float32, vagas livres) `1|1|0`; `ResourceTable+0x02C` (float32, usada A) `4|4|5`; `ResourceTable+0x04C` (float32, usada B) `4|4|5`. Conclusão restrita: os três campos não divergiram dos números informados durante a fila nesta amostragem. O filtro de correspondência pode excluir outros campos de fila/reserva, que exigem captura bruta sem filtragem. Sem prova de que os dois contadores de população usada tenham semântica idêntica. Status: hipótese.

## Monitor bruto contínuo: treinamento de aldeão (152 amostras)

Fonte: `population_timeline.csv` enviado pelo usuário, amostragem de 0,25 s, rótulo `treinamento_aldeao`; de `2026-10-17T21:10:18.734208+00:00` a `2026-10-17T21:10:56.483322+00:00`. 152 leituras `ok`, nenhum erro, ponteiro da ResourceTable `0x17F14318` (somente desta sessão).

- Amostras 1–128: `ResourceTable+0x010=1`, `+0x02C=4`, `+0x04C=4`.
- Amostra 129 (21:10:50.733454 UTC) até 152: `+0x010=0`, `+0x02C=5`, `+0x04C=5`.
- `used_b - used_a` foi zero em 152/152 amostras, e o limite reconstruído (`vagas+usada`) foi 5 em 152/152.
- Somente uma transição efetiva observada; não há timestamps separados para início, cancelamento ou conclusão de fila, logo não inferir semânticas idênticas nem ausência universal de reserva antecipada.
- Status mantido `hipotese`. Próximo experimento: registrar rótulos e horários para início da fila, cancelamento, conclusão e morte, ou comparar após reinício.
