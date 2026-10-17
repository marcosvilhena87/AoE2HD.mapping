# AoE2HD.memory_map

**Repositório exclusivo de registro de endereços de memória** para Age of Empires II HD (`AoK HD.exe 5.8.INT`, x86).

Armazena endereços confirmados historicamente, candidatos e hipóteses **mesmo sem validação**. Não hospeda mais scanners, bots, executáveis, projetos CMake nem testes.

## Catálogo

- [dados/enderecos.csv](dados/enderecos.csv): 34 registros contextualizados, incluindo RVAs, offsets e endereços absolutos temporários.
- [docs/criterios.md](docs/criterios.md): diferenças entre evidência, hipótese e confirmação.
- [docs/mapa.md](docs/mapa.md): cadeias de memória conhecidas e campos experimentais.
- [docs/proveniencia.md](docs/proveniencia.md): como recuperar todos os programas e experimentos antigos.

## Regras de atualização

1. **Nunca apagar um candidato por falta de validação.** Registrar sua incerteza.
2. Não tratar endereço absoluto de uma sessão como endereço estável.
3. Identificar a raiz de cada offset (World, Player, Object, Building etc.).
4. Conservar em observações as evidências contrárias e as interpretações alternativas.
5. Preservar no histórico Git o código antigo, sem trazê-lo de volta à branch principal.

**Nota de cobertura:** os 34 registros são os campos com contexto semântico identificado na documentação e em componentes centrais. Não representam extração exaustiva de todos os números literais dos programas históricos. Candidatos adicionais ainda exigem triagem do commit anterior.

**Código anterior preservado:** [revisão 62c3a7e1](https://github.com/marcosvilhena87/AoE2HD.memory_map/tree/62c3a7e1a22dec68af31f6501fda3c4f7f2795a7).


## Integração permanente com AoE2HDControl

O projeto [AoE2HD.control](https://github.com/marcosvilhena87/AoE2HD.control) utiliza este repositório como **fonte de verdade** dos endereços de memória.

**Regra de desenvolvimento:** sempre que uma investigação ou teste do AoE2HDControl descobrir, revisar, refutar ou identificar um novo candidato de endereço, registrar a evidência aqui na mesma etapa de desenvolvimento, antes de considerar a integração concluída.

- Inserir novos endereços em `dados/enderecos.csv`, incluindo `raiz`, endereço/offset, descrição, tipo, status e origem.
- Preservar candidatos não validados e endereços refutados; nunca promovê-los a validados apenas por valores plausíveis.
- Documentar cadeias, dependências e limitações em `docs/mapa.md` e critérios/provas em `docs/criterios.md`, quando pertinentes.
- Anotar versão do binário, passos de reprodução, cenários e evidências positivas/negativas; endereços absolutos de sessão não devem ser apresentados como estáveis.
- Somente atualizar o catálogo espelhado no AoE2HD.control depois de atualizar este repositório, para evitar divergência.
- Sem descobertas novas, não alterar artificialmente a contagem ou o status dos 34 registros existentes.

Esta regra é aplicada nas sessões de desenvolvimento em que houver acesso ao GitHub; não representa uma tarefa de monitoramento autônomo em segundo plano.
