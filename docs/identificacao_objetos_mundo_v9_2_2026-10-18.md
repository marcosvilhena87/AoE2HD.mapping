# World Object Identification v9.2 — resultados reais

Experimento no AoE2 HD somente leitura, arquivos `world_object_identification_v9_2.csv` e `world_object_identification_v9_2_groups.csv`, faixa IDs [1,4097).

## Panorama

- 2.195 objetos `unsupported_layout`, agrupados em 24 tuplas distintas (owner, ponteiro de tipo, ponteiro de definição).
- 12 amostras automáticas representativas, mais cinco controles guiados, todos `captured` sem erros no CSV.
- O grupo predominante possui **1.962 entradas** (89,4% dos incompatíveis): owner `0x10643344`, `Object+0x0C=0x103860F0`, `Object+0x78=0`. Exame do alvo `+0x10` como uint16: 351.
- Os ponteiros são endereços temporários desta sessão, não offsets estáticos.

## Controles guiados (rótulos visuais declarados pelo operador)

| Rótulo | Object ID | Owner | Object+0x0C | Object+0x78 | Target+0x10 uint16 |
|---|---:|---|---|---|---:|
| tree | 2115 | 0x10643344 | 0x103860F0 | 0 | 351 |
| animal | 2075 | 0x14A92AE4 | 0x1912EEE8 | 0 | 594 |
| gold | 2030 | 0x10643344 | 0x10724420 | 0 | 66 |
| stone | 2059 | 0x10643344 | 0x10724510 | 0 | 102 |
| relic | 2013 | 0x10643344 | 0x1459C318 | 0x14A0BE58 | 285 |

Os cinco nomes são identificações manuais do que foi selecionado; a ferramenta não valida a aparência. Tipo +0x12 também coincidiu com +0x10 nestes cinco controles, mas NÃO representa garantia geral: na v8 +0x12 divergiu em construções.

## Amostragem automática

Exemplos de alvos da leitura alternativa:
- Grupo de 1.962: ID 70, tipo candidato 351;
- Grupo de 51: ID 2213, tipo candidato 458;
- Grupo de 36: ID 2026, tipo candidato 66;
- Grupo de 26: ID 68, tipo candidato 709;
- Grupo de 25: ID 2264, tipo candidato 69;
- Grupo de 23: ID 2, tipo candidato 264;
- Grupo de 21: ID 2056, tipo candidato 102;
- Grupo de 12: ID 2201, tipo candidato 455.

O controle tree (351) coincide com o grupo de 1.962; gold (66) com o grupo de 36; stone (102) com o grupo de 21. Ainda não há nomenclatura validada dos outros grupos.

## Interpretação

A presença de `Object+0x78=0` não impede que `Object+0x0C -> Target+0x10` seja legível em objetos do cenário. O status `unsupported_layout` da v9.1 significa layout incompatível com a leitura conjunta do tipo e classe, não ausência de ID específico.

O ponteiro `0x10643344` concentra muitos objetos rotulados árvore, ouro, pedra e relíquia: candidato a Gaia/owner ambiental, **não validado**. O animal teve outro owner igual ao observado como local na v9.1, compatível com controle por jogador. Não assumir que todas as árvores são tipo 351 nem que todos os animais são 594 antes de novos controles e sessões.

Próximo aprimoramento: separar detecção de **tipo** e **classe** para que `Object+0x78=0` não seja classificado como falha total; manter campo class_id como ausente, identificar tipo independentemente e rotular os owners por controles. Preservar todos os valores desconhecidos sem criar equivalências sem validação adicional.
