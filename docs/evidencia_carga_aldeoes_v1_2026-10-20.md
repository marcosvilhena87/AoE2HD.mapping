# Villager Carry Capacity Discovery v1 — proposta de validação

**Campo investigado:** `Object+0x48` `float32`. **Status: hipótese.**
Evidência anterior: `docs/identificacao_objetos_mundo_v9_2_2026-10-18.md`, protocolos v9.8 e v9.9, nos quais fontes de recursos diminuíram enquanto objetos coletores candidatos aumentaram em quantidades quase iguais. A mesma posição relativa nos objetos não prova que seja capacidade máxima.

No controlador `AoE2HD.control`, o modo `--discover-villager-carry-capacity-v1` lê um aldeão local por ID estável, verifica classe 4, captura o valor bruto e suas diferenças temporais, o maior valor observado em uma sequência válida, quedas candidatas a depósito e falhas de leitura. O rótulo de fase e recurso é informado pelo operador; nenhum limite máximo é presumido.

## Critérios de validação futura
1. Registrar ciclo completo vazio → coleta → transporte → depósito, verificando simultaneamente o estoque do jogador e a fonte.
2. Mostrar que um platô precede o retorno ao depósito, sem ordens manuais adicionais, em múltiplos ciclos.
3. Repetir com madeira, alimentos, ouro e pedra, e testar tecnologias que alterem transporte, se aplicáveis.
4. Repetir após reiniciar o jogo, mantendo identificação individual e controles negativos.
5. Diferenciar **maior valor observado na captura** de **capacidade máxima efetiva**: os dois não são equivalentes.

Registro adicionado sem promover o endereço a validado.