# Evidência de população — 2026-10-17

AoE2 HD 5.8.INT, sondagem somente leitura guiada pelo HUD. Estados inseridos: população usada/limite 4/5, 4/10, 5/10 e 6/10. O arquivo population_candidates.csv enviado contém duas correspondências float32, ambas para população usada: ResourceTable+0x02C e ResourceTable+0x04C, quatro de quatro observações e duas de duas transições. Nenhuma correspondência para limite populacional. Estes offsets são hipóteses, não endereços revalidados. Próximos testes: aumento e redução de população, unidades em fila, limite alterado sem mudança na população, reinício do processo e comparação visual.

## Hipótese adicional: vagas disponíveis

Hipótese proposta pelo usuário: o limite apresentado no HUD pode ser derivado de `população usada + vagas disponíveis`, em vez de estar armazenado diretamente. Para os estados 4/5, 4/10, 5/10, 6/10 a sequência esperada de vagas é 1, 6, 5, 4. Examinar também `limite - usada`, sem considerar qualquer coincidência prova de semântica. Distinguir população em fila e variações de capacidade por edificações. Status: hipótese não validada.

## Sondagem v2 — vagas disponíveis observadas

CSV `population_candidates_v2.csv` enviado pelo usuário: `ResourceTable+0x010`, `float32`, acompanhou `1|6|5|4` para `free_slots`, 4/4 observações e 3/3 transições; `ResourceTable+0x02C` e `+0x04C`, `float32`, acompanharam `4|4|5|6` para `used`, 4/4 observações e 2/2 transições. Hipótese: `limite = usada + vagas disponíveis`. Ainda não verificado após reinício ou sob fila de produção; sem confirmação semântica definitiva.
