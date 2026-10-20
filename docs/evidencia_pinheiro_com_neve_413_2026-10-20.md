# Confirmação experimental: Genie Type 413 — Pinheiro com Neve (2026-10-20)

Fonte: `tree_type_confirmation_v1_2.csv`, produzido por `AoE2HD.control --confirm-tree-type-v1-2` e fornecido pelo operador, complementado por screenshot do HUD exibindo `Pinheiro com Neve`, HP 20/20 e madeira 100. O rótulo foi informado manualmente, NÃO lido automaticamente da memória.

Duas capturas da mesma árvore selecionada: Object ID **2131**; ponteiro `0x148901C0`, ponteiro de tipo `0x14A757F8` e Genie Type **413** permaneceram estáveis. Propriedade de seleção identificada por estrutura do jogador local e correspondência na tabela World. Os endereços absolutos são exclusivos desta sessão.

| Captura UTC | +0x34 HP candidato | +0x48 madeira candidata |
|---|---:|---:|
| 2026-10-20 02:56:44 | 20.0 | 100.0 |
| 2026-10-20 02:57:00 | 0.0 | 98.23876190185547 |

Deltas: HP -20, madeira -1.761238. Compatível com queda da árvore e extração de madeira na mesma identidade. Evidência observacional forte para a **variante identificada no HUD como Pinheiro com Neve**; não generalizar `413` para todas as árvores/mapas, nem classificar `+0x34`/`+0x48` como offsets universais a partir deste teste único.

Cadastro em `dados/catalogos/tipos_genie.csv` e espelho `dados/genie_type_ids.csv`: `observado_hud_coleta_uma_sessao`. Recomendável replicar em outra árvore/sessão e testar confirmação visual sincronizada se necessário.


## Segunda árvore — Object ID 2130

Fonte: `tree_type_confirmation_v1_2(1).csv` (captura inicial observada, seleção posteriormente indisponível) e `tree_2130_confirmation.csv` (duas capturas por ID explícito na mesma sessão, fornecidas pelo operador).

Em 2026-10-20T03:02:10.753382+00:00 e 03:02:15.546452+00:00, o Object ID **2130** manteve Genie Type **413**, ponteiro `0x1488EFC8`, owner `0x141D604C` e ponteiro de tipo `0x14A757F8`.

| Medida candidata | Antes | Depois | Delta |
|---|---:|---:|---:|
| Object+0x34 (HP) | 0.0 | 0.0 | 0.0 |
| Object+0x48 (madeira) | 39.25263595581055 | 36.46411895751953 | -2.788517 |

Tempo entre capturas ~4.793 s. Ambas as leituras `captured`, `comparison_status=same_object`, `selection_method=explicit_id`, sem erros. O primeiro registro da árvore 2130, em arquivo anterior, reportou seleção direta com HP 20 e madeira 100; a comparação acima foi feita após a queda (HP 0), mostrando extração posterior. A identidade da árvore depende da prova de seleção inicial da mesma sessão; o modo explicit_id por si só não liga objeto a HUD.

**Conclusão cumulativa:** dois Object IDs distintos (2131 e 2130), ambos Genie 413 e com redução de madeira no mesmo objeto; evidência funcional replicada entre objetos na sessão estudada. Ainda não é uma replicação em sessão/mapa independente nem validação automática do texto do HUD. Não extrapolar offsets para todos os tipos.
