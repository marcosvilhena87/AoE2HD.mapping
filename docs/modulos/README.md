# Catálogo modular de endereços

Fonte temática dos registros, sem modificar seus estados originais. O catálogo legado [`dados/enderecos.csv`](../../dados/enderecos.csv) permanece disponível como **espelho consolidado** e deverá ser atualizado juntamente com os módulos. Cada linha consta em exatamente um módulo; a classificação modular não constitui confirmação técnica.

| Módulo | Registros | Escopo |
|---|---:|---|
| [Núcleo e estruturas globais](../../dados/modulos/nucleo.csv) | 12 | RVAs e cadeia Owner → World; campos estruturais do mundo. |
| [Jogadores e seleção](../../dados/modulos/jogadores.csv) | 7 | Entradas do vetor de jogadores, identidade e seleção local. |
| [Recursos, população e contadores](../../dados/modulos/recursos_populacao.csv) | 10 | Recursos econômicos, contadores de unidades e população, inclusive candidatos. |
| [Unidades, classes e tipos](../../dados/modulos/unidades_identidade.csv) | 12 | Identidade de objetos, tipos específicos, classes internas e estruturas candidatas. |
| [Construções e fila de produção](../../dados/modulos/construcoes_producao.csv) | 3 | Campos da fila de produção vinculados a construções. |
| [Treinamento e progresso](../../dados/modulos/treinamento.csv) | 7 | Candidatos estruturais de treinamento; não implica cadeia já validada. |
| [Endereços absolutos temporários](../../dados/modulos/sessoes_temporarias.csv) | 13 | Capturas absolutas específicas de execução; nunca assumir estabilidade entre sessões. |

**Total:** 64 registros. O cabeçalho CSV permanece `raiz,offset_ou_endereco,descricao,tipo,status,origem`.

## Regras de manutenção

1. Novos endereços e hipóteses entram no módulo correspondente, conservando origem, status, tipo e distinção entre RVA, offset e endereço absoluto.
2. Atualizar também `dados/enderecos.csv` na **mesma alteração**, sem excluir candidatos ou registros refutados; o arquivo consolidado conserva a ordem histórica quando possível.
3. Cada registro do consolidado deve corresponder a uma linha em exatamente um módulo (incluindo repetições legítimas, se houver). Verificar multiconjunto de linhas para detectar perda, modificação ou duplicação.
4. Evidências e experimentos existentes permanecem em [`docs/`](../); a documentação de cadeias permanece em [`docs/mapa.md`](../mapa.md).
5. Não converter `sessao_observada` em validado e não alterar estados por reorganização.

## Validação rápida de integridade

A contagem de linhas não vazias sem cabeçalho deve ser idêntica entre `dados/enderecos.csv` e a soma dos arquivos de `dados/modulos/`. Para conferir integralmente, comparar o **multiconjunto** das linhas completas, sem depender da ordenação. Nenhum arquivo executável, bot ou scanner é necessário.
