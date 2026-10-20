# Confirmação experimental: Genie Type 413 — Pinheiro com Neve (2026-10-20)

Fonte: `tree_type_confirmation_v1_2.csv`, produzido por `AoE2HD.control --confirm-tree-type-v1-2` e fornecido pelo operador, complementado por screenshot do HUD exibindo `Pinheiro com Neve`, HP 20/20 e madeira 100. O rótulo foi informado manualmente, NÃO lido automaticamente da memória.

Duas capturas da mesma árvore selecionada: Object ID **2131**; ponteiro `0x148901C0`, ponteiro de tipo `0x14A757F8` e Genie Type **413** permaneceram estáveis. Propriedade de seleção identificada por estrutura do jogador local e correspondência na tabela World. Os endereços absolutos são exclusivos desta sessão.

| Captura UTC | +0x34 HP candidato | +0x48 madeira candidata |
|---|---:|---:|
| 2026-10-20 02:56:44 | 20.0 | 100.0 |
| 2026-10-20 02:57:00 | 0.0 | 98.23876190185547 |

Deltas: HP -20, madeira -1.761238. Compatível com queda da árvore e extração de madeira na mesma identidade. Evidência observacional forte para a **variante identificada no HUD como Pinheiro com Neve**; não generalizar `413` para todas as árvores/mapas, nem classificar `+0x34`/`+0x48` como offsets universais a partir deste teste único.

Cadastro em `dados/catalogos/tipos_genie.csv` e espelho `dados/genie_type_ids.csv`: `observado_hud_coleta_uma_sessao`. Recomendável replicar em outra árvore/sessão e testar confirmação visual sincronizada se necessário.
