# Evidência de seleção de objetos — Selected Objects API v2.1–v2.6 (21/10/2026)

**Versão-alvo declarada do projeto:** `AoK HD.exe 5.8.INT`, PE32/x86. A captura não contém hash SHA-256, checksum do executável nem identificação automática de build. Portanto, a compatibilidade está condicionada à build declarada; não extrapolar para DE ou outras versões.

## Estado epistemológico

**`validado_experimentalmente`** significa que o campo teve comportamento reproduzido nas capturas fornecidas, com verificações de ponteiro, identidade e flag no leitor. **Não significa** layout definitivamente conhecido, validação independente após reinício, tamanho físico da estrutura comprovado, nem autorização para promovê-lo ao estado `validado` ou `historico_validado`.

## Evidências observadas

A estrutura local do jogador foi identificada na sessão como endereço absoluto `417013788` (somente sessão; **não** registrar como RVA ou ponteiro reutilizável).

- `LocalPlayer+0x160`: primeira posição observada do vetor contíguo de ponteiros (`ptr32`). Registro legado correspondente já existe como âncora experimental e foi preservado, sem reclassificação silenciosa.
- `LocalPlayer+0x164`: segunda entrada observada. Próximas entradas deslocam 4 bytes por objeto.
- `LocalPlayer+0x24C`: posição índice 59 (60ª) identificada com ID de objeto e flag de seleção; 60 unidades é limite observado na **interface** para unidades móveis, não prova de capacidade física da estrutura.
- `LocalPlayer+0x254`: campo `uint32` candidato à contagem de objetos selecionados. Capturas v2.5 e v2.6 reproduziram a correspondência entre seu valor e a contagem observada. Na v2.6: **0 sem seleção, 60 móveis (classe 4), 15 construções (classe 3)**. Em cada captura, `count_before == count_after == count_final`, número de entradas validadas igual à contagem, nenhum erro de ponteiro, duplicação ou flag. Também houve capturas v2.5 com grupos de 32, 40 e 50 unidades.
- `Object+0x38`: a máscara `0x00010000` foi observada ativa para os objetos selecionados nas capturas; os demais bits não têm interpretação validada por este experimento.
- `LocalPlayer+0x15C`: registro anterior de âncora preservado; variou como referência a objeto selecionado, **não** como quantidade, nos experimentos v2.3. Identidade semântica exata ainda desconhecida.
- `LocalPlayer+0x250`: campo adjacente de semântica **desconhecida**, variou de modo incompatível com ponteiro selecionado ou contador simples em algumas capturas. **Não** tratar como 61º ponteiro.
- Estrutura de seleção do jogador não deve ser confundida com tabela global de objetos do World.

## Reprodução e limitações

Os diagnósticos JSON submetidos pelo operador foram nomeados `selected_v2_6_clear.json`, `selected_v2_6_60.json`, `selected_v2_6_buildings.json`, com resultado `consistent_with_candidate_count`. Foram originados pelo leitor em somente leitura do projeto [AoE2HD.control](https://github.com/marcosvilhena87/AoE2HD.control), módulo `aoe2hdcontrol/selected_objects_v2_6.py`.

Procedimento: executar em partida com build correspondente; desselecionar tudo e registrar; selecionar 60 unidades móveis e registrar; desselecionar, selecionar somente construções e registrar. Verificar contador antes/depois/final e ponteiros usando tabela de IDs e bit `0x10000`.

As capturas da v2.6 não comprovam ausência de troca de identidade mantendo contagem constante durante a leitura; leituras não são atômicas. **Não há controle independente após reiniciar o jogo e nenhuma confirmação de limite para construções.** O campo `completeness` é `unknown`. Não há autorização para atribuir a todo `LocalPlayer+0x160..0x250` uma capacidade fixa de 60/64, nem para promover `+0x254` a `validado` canônico sem uma campanha separada.

## Rastreabilidade

Evidências: `selected_v2_4_clear.json`, `selected_v2_4_twenty.json`, `selected_v2_4_buildings.json`, `selected_v2_5_clear.json`, `selected_v2_5_32.json`, `selected_v2_5_40.json`, `selected_v2_5_50.json`, `selected_v2_5_60.json` e as três capturas v2.6 citadas. Os arquivos brutos foram enviados na conversa e **não** são adicionados aqui; este documento resume os resultados informados e examinados, sem representá-los como arquivos versionados neste repositório.
