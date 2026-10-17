# Evidência de população — 2026-10-17

AoE2 HD 5.8.INT, sondagem somente leitura guiada pelo HUD. Estados inseridos: população usada/limite 4/5, 4/10, 5/10 e 6/10. O arquivo population_candidates.csv enviado contém duas correspondências float32, ambas para população usada: ResourceTable+0x02C e ResourceTable+0x04C, quatro de quatro observações e duas de duas transições. Nenhuma correspondência para limite populacional. Estes offsets são hipóteses, não endereços revalidados. Próximos testes: aumento e redução de população, unidades em fila, limite alterado sem mudança na população, reinício do processo e comparação visual.

## Hipótese adicional: vagas disponíveis

Hipótese proposta pelo usuário: o limite apresentado no HUD pode ser derivado de `população usada + vagas disponíveis`, em vez de estar armazenado diretamente. Para os estados 4/5, 4/10, 5/10, 6/10 a sequência esperada de vagas é 1, 6, 5, 4. Examinar também `limite - usada`, sem considerar qualquer coincidência prova de semântica. Distinguir população em fila e variações de capacidade por edificações. Status: hipótese não validada.
