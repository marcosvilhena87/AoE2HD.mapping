# Evidência Selected Objects API v2.7 — 21/10/2026

## Proveniência
Seis arquivos JSON fornecidos pelo operador na conversa: `selected_v2_7_clear.json`, `selected_v2_7_60.json`, `selected_v2_7_buildings.json`, `selected_v2_7_group_A.json`, `selected_v2_7_group_B.json` e `selected_v2_7.json`. Arquivos brutos **não versionados neste repositório**. Gerados por `AoE2HD.control`, API v2.7 (branch `feat/selected-objects-v2-7`), comparando duas inspeções v2.6 de cada captura.

## Observações

| Captura | Selecionados | Classes | Tipos Genie | Estado |
|---|---:|---|---|---|
| clear | 0 | — | — | repeated_observations_agree |
| 60 | 60 | classe 4 (60) | 83:29, 293:29, 118:2 | repeated_observations_agree |
| buildings | 15 | classe 3 (15) | 70:15 | repeated_observations_agree |
| group_A | 60 | classe 4 (60) | 293:33, 83:25, 118:2 | repeated_observations_agree |
| group_B | 60 | classe 4 (60) | 83:31, 293:27, 118:2 | repeated_observations_agree |
| selected_v2_7.json | 60 | classe 4 (60) | 83:29, 293:29, 118:2 | repeated_observations_agree |

As 12 observações internas da v2.6 foram consistentes com o contador candidato; não houve anomalia registrada. Para cada captura, as duas sequências de `(object_id,address)` foram idênticas.

**Comparação entre arquivos independentes A e B:** ambos selecionaram 60 unidades, com 46 Object IDs em comum, 14 exclusivos de A e 14 exclusivos de B. Isto comprova mudança observada de identidade entre capturas de igual contagem; **não** testa alteração concorrente no intervalo entre as duas leituras dentro da mesma captura.

## Conclusões e limites

- Evidência favorável à repetibilidade da enumeração de objetos selecionados na sessão investigada, com contagem 0/15/60 e diferenças reais de identidade entre seleções com 60 unidades.
- **Não** comprova que o detector capture mudança concorrente com contador estável; neste conjunto, as duas observações de cada execução concordaram. A detecção de reordenação também não foi evidenciada dinamicamente.
- Leituras não são atômicas: mudanças ABA podem passar despercebidas. `completeness=unknown` permanece.
- Nenhum hash do executável, reinício independente ou teste em outra build foi anexado. A referência `AoK HD.exe 5.8.INT` continua sendo a versão-alvo declarada do projeto, não verificação por hash destes JSON.
- Permanecem **inalterados** os status canônicos dos offsets `LocalPlayer+0x160`, `LocalPlayer+0x254` e `Object+0x38`. Não estabelecer tamanho físico de vetor nem layout definitivo.
