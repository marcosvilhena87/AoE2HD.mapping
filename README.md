# AoE2HD.mapping

**Repositório de mapeamento de endereços de memória e identificadores de objetos** para Age of Empires II HD (`AoK HD.exe 5.8.INT`, x86).

Armazena endereços confirmados historicamente, candidatos e hipóteses **mesmo sem validação**. Não hospeda mais scanners, bots, executáveis, projetos CMake nem testes.

## Catálogos modulares

Os catálogos existentes em `dados/catalogos/` permanecem canônicos.
Seus módulos temáticos em `dados/catalogos/modulos/` agrupam tipos Genie
(aldeões, profissões, construções, militares, recursos naturais e desconhecidos),
classes de objetos (conhecidas e desconhecidas) e identidades individuais
(por sessão). Os sete módulos de endereços em `dados/modulos/` são mantidos.

Todos os registros, origens e estados de evidência são preservados.
`dados/genie_type_ids.csv` e `dados/enderecos.csv` mantêm a compatibilidade
com consumidores existentes, incluindo o AoE2HD.control.

Atualize módulo, catálogo canônico e espelho legado aplicável na mesma mudança.
Valide a integridade com `python scripts/validar_catalogos.py`;
ausências, divergências e duplicações provocam falha na validação.

## Catálogos por domínio

- [Tipos Genie](dados/catalogos/tipos_genie.csv): 41 identificadores e evidências; espelho do caminho compatível `dados/genie_type_ids.csv`.
- [Classes de objetos](dados/catalogos/classes_objetos.csv): classes internas observadas e status semântico.
- [Identidades individuais](dados/catalogos/identidades_individuais.csv): Object IDs vinculados à sessão de observação; não representam IDs de tipo estáveis.
- [Estruturas de memória](dados/catalogos/estruturas_memoria.csv): espelho integral de `dados/enderecos.csv`, preservando os 64 registros e os módulos históricos.

Para testar a integridade e a sincronização dos arquivos:

```bash
python scripts/validar_catalogos.py
```

O GitHub Actions também executa essa verificação. O repositório `AoE2HD.control` continua consumindo `dados/genie_type_ids.csv` para manter compatibilidade. Os arquivos históricos e seus status não foram substituídos.

## Catálogo

- [dados/enderecos.csv](dados/enderecos.csv): 64 registros contextualizados, incluindo RVAs, offsets e endereços absolutos temporários.
- [docs/modulos/README.md](docs/modulos/README.md): índice de 7 módulos temáticos em `dados/modulos/`, com contagens e regras de sincronização.
- [docs/criterios.md](docs/criterios.md): diferenças entre evidência, hipótese e confirmação.
- [dados/genie_type_ids.csv](dados/genie_type_ids.csv): catálogo separado com 41 Genie Type IDs, nomes candidatos e origem experimental.
- [docs/genie_type_ids.md](docs/genie_type_ids.md): mapeamento por sexo, unidades, edifícios e tipos desconhecidos.
- [docs/mapa.md](docs/mapa.md): cadeias de memória conhecidas e campos experimentais.

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

- Inserir novos endereços no módulo pertinente de `dados/modulos/` e espelhar em `dados/enderecos.csv`, incluindo `raiz`, endereço/offset, descrição, tipo, status e origem.
- Preservar candidatos não validados e endereços refutados; nunca promovê-los a validados apenas por valores plausíveis.
- Documentar cadeias, dependências e limitações em `docs/mapa.md` e critérios/provas em `docs/criterios.md`, quando pertinentes.
- Anotar versão do binário, passos de reprodução, cenários e evidências positivas/negativas; endereços absolutos de sessão não devem ser apresentados como estáveis.
- Somente atualizar o catálogo espelhado no AoE2HD.control depois de atualizar este repositório, para evitar divergência.
- Sem descobertas novas, não alterar artificialmente a contagem ou o status dos 64 registros existentes.

Esta regra é aplicada nas sessões de desenvolvimento em que houver acesso ao GitHub; não representa uma tarefa de monitoramento autônomo em segundo plano.

## Integridade automatizada do catálogo

O workflow [Validar catálogo de memória](.github/workflows/validar-catalogo.yml) é executado em push ou pull request que altere dados, validador, testes ou workflow; também pode ser iniciado manualmente em **Actions → Validar catálogo de memória → Run workflow**.

Para executar localmente, na raiz do repositório:

```bash
python -m unittest discover -s tests -v
python scripts/validar_catalogo.py
```

O validador compara todas as seis colunas de cada registro como **multiconjunto**, sem depender da ordem dos arquivos. Detecta ausência em qualquer direção, duplicação integral, conflito de registros com a mesma combinação de raiz e endereço, problemas de cabeçalho, campos vazios e falta de módulos. Se houver divergência, o comando retorna código 1 e o workflow falha. Não corrige nem promove endereços automaticamente.

## Evidências recentes de coleta por proximidade

- [Experimento de coleta próxima ao Centro da Cidade (20/10/2026)](docs/evidencia_coleta_proxima_tc_2026-10-20.md): Object IDs de sessão, coordenadas X/Y candidatas, estoque candidato em `Object+0x48`, ranking do TC, validação de âncoras e limitações. Não acrescenta endereços absolutos estáveis nem altera os status dos catálogos.
