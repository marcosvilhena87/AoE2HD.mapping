# Critérios de evidência

Versão-alvo: `AoK HD.exe 5.8.INT` (PE32/x86). Offsets não devem ser transferidos automaticamente para a Definitive Edition ou outra compilação.

- **historico_validado** — documentação histórica relata teste real; não significa revalidação neste commit.
- **semantica_parcial** — valores observados; sua interpretação não foi completamente determinada.
- **ancora_experimental** — referência utilizada para correlacionar seleção e objetos; não constitui coleção.
- **experimental_implementado** — há leitor/algoritmo histórico, mas isso não é prova semântica independente.
- **validado_experimentalmente** — comportamento reproduzido em capturas identificadas, ainda sem validação independente ou confirmação definitiva da semântica e dos limites de estrutura.
- **hipotese** — candidato que exige repetição, controles negativos e validação por ação observável.
- **sessao_observada** — endereço absoluto de uma execução. Não reutilizar após reiniciar.

RVA significa deslocamento relativo à base do executável; offset significa deslocamento relativo a uma estrutura. `World+0x88` não é igual a `[World+0x88]`. Endereços de sessão, offsets e RVAs jamais devem ser mesclados por igualdade numérica.

Para promover candidato: registrar versão do binário, objeto/ação, estado antes/depois, captura-controle, teste sem seleção, repetição após reinício e eventuais falhas. Ponteiros similares, IDs plausíveis e valores de HP finitos não provam que um objeto é uma unidade real ou que a coleção é completa.
