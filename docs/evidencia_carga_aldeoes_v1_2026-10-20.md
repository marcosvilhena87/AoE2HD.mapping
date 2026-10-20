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
## Captura real `carry_wood.csv` — sessão `carga01` (2026-10-20)

Monitor v1 do `AoE2HD.control`, fase manual `other`, recurso rotulado `wood`. **180/180 leituras válidas**, sem erros. Objeto `2174`, Genie Type ID `123`, classe `4`, ponteiro do objeto `0x14074FC0` estável na captura.

O candidato `Object+0x48` `float32` atingiu **10.0** nas amostras **43–44, 90–95, 133–138 e 177–180**. Ocorreram três reduções abruptas de **10.0 para 0.0** nas amostras **45, 96 e 139**, todas sinalizadas por `peak_candidate=1` e `reset_candidate=1`. A quarta sequência terminou em 10.0 antes do fim da observação. No início da captura, o valor já era aproximadamente 0.39897, portanto o primeiro ciclo começou parcialmente em andamento.

**Conclusão limitada:** há evidência temporal forte de acúmulo de carga até limite observado de 10.0 e descarregamentos candidatos repetidos para esse aldeão ao coletar madeira. A capacidade máxima universal ou formal do jogo NÃO está validada; uma queda não confirma depósito sem verificação simultânea do estoque do jogador. `Object+0x48` permanece **hipotese** até controles com madeira do jogador, fonte, vários aldeões, reinício e tecnologias. Não interpretar `max_observed_in_run` isoladamente como parâmetro de capacidade.
