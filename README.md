# AoE2HDControl

Projeto de engenharia reversa e reconstrução de uma **API de controle para Age of Empires II HD (AoK HD.exe 5.8.INT, x86)**.

O **AoE2Control para Definitive Edition** é usado **apenas como referência conceitual** para definir funcionalidades e alvos semânticos. **Offsets, layouts e estruturas da Definitive Edition não são considerados válidos no HD.** Toda estrutura usada aqui deve ser investigada e validada especificamente na versão HD.

Esta é uma **reimplementação do zero**. O repositório histórico [AoE2HDControl.v0](https://github.com/marcosvilhena87/AoE2HDControl.v0) preserva descobertas de engenharia reversa, endereços, offsets e experimentos anteriores; não é necessário transportar sua arquitetura nem aceitar hipóteses antigas sem validação.

## Objetivo de longo prazo

Construir gradualmente uma API para observar o estado do jogo e, em fases posteriores, avaliar mecanismos de controle. Prioridades:

1. Leitura confiável de `World`, jogadores e recursos;
2. Enumeração das unidades de cada jogador **sem necessidade de seleção**;
3. Identificação de aldeões, tipos, posições, saúde e estados;
4. Telemetria de economia, produção e demais eventos;
5. Investigação posterior de comandos de controle, somente com fundamentos específicos do HD.

## Estado atual — primeira implementação

- Localiza o processo e a base do módulo `AoK HD.exe`;
- Lê `World`, jogadores e recursos econômicos em modo somente leitura;
- Valida vtables, limites do vetor e referências de retorno;
- Exibe diagnóstico, sem reutilizar ponteiros entre partidas.

**Teste real informado:** base de módulo `0x00B90000`, `World=0x1929E848`, três jogadores (incluindo Gaia), jogador local no índice 1 e leitura de comida, madeira, pedra e ouro. Isso confirma o caminho básico no cenário testado, não todas as configurações do jogo.

**Ainda não implementado:** enumeração automática de unidades, identificação de aldeões e comandos de controle.

## Compilar no Windows (Visual Studio com C++ e CMake)

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A Win32
cmake --build build --config Release
.\build\Release\aoe2hd_probe.exe
```

Se estiver usando Visual Studio 2022, altere o gerador para `Visual Studio 17 2022`.

## Segurança e limites atuais

O protótipo usa `OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION)` e `ReadProcessMemory`. Não realiza escrita, injeção nem envio de comandos. Durante mudanças de partida, a memória pode variar; valores são rejeitados quando as validações falham. A prontidão estrutural não prova, por si só, prontidão total de gameplay.

Veja [docs/memory-map.md](docs/memory-map.md) para os endereços reutilizados, a origem das evidências e os limites da validação.

## Descoberta experimental de coleções (segunda etapa)

Com o jogo em partida, execute:

```powershell
.\build\Release\aoe2hd_probe.exe --discover-units
```

O modo opcional preserva a leitura de World, jogadores e recursos e examina janelas limitadas (até 0x800 bytes) em World e LocalPlayer, procurando triplas de ponteiros com formato de vetor. Considera strides de 4 ou 8 bytes, no máximo 512 entradas por candidato, 64 amostras e buffer máximo de 8192 bytes. Gera `aoe2hd_unit_candidates.csv` no diretório atual (sobrescrito a cada execução).

**Atenção:** as linhas são apenas hipóteses de coleções, não prova de enumeração de unidades. Contagens podem incluir coleções não relacionadas. A identificação de objetos, donos e aldeões ainda não foi implementada. Compare resultados em diferentes estados da partida; não classifique o maior candidato como coleção verdadeira só por seu tamanho.

## Validação experimental de identidade e propriedade

`--discover-units` agora gera também `aoe2hd_entity_evidence.csv`, com uma linha por caminho observado (ponteiro direto e um nível indireto, slots +0 e +4). Campos: `source,offset,stride,entries,index,word,value,target,path,readable,signature,candidate_id,candidate_type,candidate_owner,ownership,duplicate,status`.

Os campos `candidate_id`, `candidate_type` e `candidate_owner` usam o layout **experimental** da v0: objeto `+0x08`, `+0x0C` e `+0x10`. Propriedade significa apenas que `candidate_owner` coincidiu com um ponteiro de jogador previamente validado, não que a entrada seja uma unidade real. Cada linha mantém `status=hypothesis`. A contagem de coincidências pode incluir duplicações entre estruturas.

Para executar os testes sem o jogo:
```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A Win32
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

O CSV é sobrescrito a cada execução. Não leia resultados durante troca/criação de partida.

## SnapshotDiff: teste controlado de criação de aldeão

O executável `aoe2hd_snapshot_diff.exe` é separado do probe e usa somente `ReadProcessMemory`. Abre o processo uma única vez, valida `World` e jogador local em cada captura, verifica a estabilidade dos limites do vetor de jogadores e recusa a comparação se o processo, `World` ou jogador local mudar.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_snapshot_diff.exe
```

Procedimento: entre numa partida estável, pressione **Enter** para capturar A, crie exatamente **um aldeão**, aguarde a conclusão de sua produção e pressione **Enter** para capturar B. Evite selecionar/trocar de partida ou executar outras ações durante o teste. Os relatórios são salvos no diretório atual e **sobrescritos** na próxima execução:

- `aoe2hd_snapshot_before.csv`: todas as entradas legíveis da captura A;
- `aoe2hd_snapshot_after.csv`: mesmas informações para B;
- `aoe2hd_snapshot_diff.csv`: `source,offset,stride,kind,index,before,after`. `kind` indica `added`, `removed`, `changed`, `buffer_relocated`, `collection_new` ou `collection_missing_after`.

Uma linha `added` significa novo **índice no vetor**, não necessariamente novo objeto. A análise também pode encontrar mudanças por eventos independentes da criação do aldeão. Interpretações concorrentes de stride são mantidas separadas; os dados continuam **hipóteses**, jamais enumerações confirmadas. A captura não é atomicamente sincronizada com o motor do jogo, e estruturas internas podem mudar enquanto são lidas.

Para repetir com segurança, copie/renomeie os CSVs de cada experimento antes de começar outro.

## SnapshotDiff v2 — campos internos e controle

O SnapshotDiff agora captura **todos os words por entrada** (um `uint32` para stride 4; dois para stride 8) e coleta amostras de **64 bytes** dos primeiros **256 endereços distintos alinhados** identificados nos campos dos vetores candidatos. Nao percorre ponteiros recursivamente. Os valores sao evidencias experimentais, nao identidades confirmadas.

Os arquivos `aoe2hd_snapshot_before.csv` e `aoe2hd_snapshot_after.csv` agora possuem a coluna `word`. Tambem sao gerados `aoe2hd_snapshot_before.csv.objects.csv` e `aoe2hd_snapshot_after.csv.objects.csv`, com os dados internos e status de leitura. O `aoe2hd_snapshot_diff.csv` recebe `word` e registros `source=Object,kind=object_field_changed` para enderecos presentes e legiveis em ambos os snapshots. Para esses registros, `offset` indica o **endereco do objeto**, e `word` indica o indice de campo `uint32` dentro dos 64 bytes.

Use primeiro o **experimento-controle**: duas capturas sem realizar acao no jogo, salvando os CSVs sob nomes proprios. Depois repita criando exatamente um aldeao. Mudancas por temporizadores e simulacao normal podem aparecer mesmo sem a acao; compare controles e repeticoes antes de atribuir significado a qualquer campo.

Ainda nao ha deteccao de unidades reais nem prova de que os enderecos inspecionados sejam objetos do jogo. Algumas amostras poderao ser blocos nao relacionados, inacessiveis ou mudarem durante a captura.

## UnitReferenceDiscovery — ancora experimental por selecao

O executavel `aoe2hd_unit_reference_discovery.exe` usa selecao **somente para calibracao**; o objetivo posterior continua sendo enumerar unidades sem selecao.

**Origem dos offsets (v0):** `LocalPlayer+0x15C` e `+0x160` continham o mesmo ponteiro de objeto durante uma selecao simples; `selectedObject+0x08` foi tratado como ID candidato, `+0x10` como proprietario candidato e `+0x34` como HP `float32`. Para aceitar a ancora exigimos os dois slots iguais, objeto alinhado e legivel, HP finito entre 0 e 100000 e verificacao posterior de World, Owner e selecao. Essas heuristicas nao provam que o objeto e aldeao: confirme visualmente que selecionou apenas **um aldeao**.

**Execucao:**

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_unit_reference_discovery.exe
```

O programa examina **somente** os primeiros `0x800` bytes de `World` e do jogador local e buffers de possiveis vetores neles encontrados, cada um com no maximo 2048 bytes (limite global de tamanho admitido do vetor: 8192 bytes). Procura valores diretamente iguais a ancora e, no maximo, um nivel indireto por meio das primeiras duas words do endereco intermediario. Nenhuma varredura ampla de processo nem escrita de memoria. Nenhum resultado implica que a estrutura contenha todos os aldeoes.

O arquivo `aoe2hd_unit_references.csv` (sobrescrito na execucao seguinte) inclui sessao, endereco da ancora, ID/owner candidatos, origem, offset de origem, endereco do hit, caminho `direct`, `indirect_0` ou `indirect_4`, valores e `status=hypothesis`.

**Procedimento:** entre em uma partida, selecione **exatamente um aldeao**, execute o programa e preserve o CSV. Em seguida selecione outro aldeao, repita e compare quais regioes exibem referencias aos dois objetos. Referencias de selecao no proprio jogador local sao esperadas e nao comprovam colecao geral. Priorize estruturas que referenciem unidades mesmo apos mudar a selecao; a enumeracao sem selecao ainda nao foi implementada.

**Limites:** o scanner pode produzir coincidencias acidentais, nao percorre todas as regioes do heap, nao verifica atomicamente todos os buffers e pode perder colecoes reais fora das janelas escolhidas. Nao usar em partidas online/competitivas sem considerar regras do jogo.

## UnitReferenceDiscovery v2 — pesquisa limitada por VirtualQueryEx

O v2 e um executavel **separado**, somente leitura, que reutiliza os validadores de ancora da v1. Examina regioes de memoria `MEM_COMMIT` com protecao legivel retornadas por `VirtualQueryEx`, sem alterar o jogo. Procura **DWORDs alinhados cujo valor seja exatamente o endereco da unidade selecionada**. Os quatro bytes anteriores e posteriores sao registrados quando disponiveis no bloco lido.

### Uso

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_unit_reference_discovery_v2.exe --max-mib 64 --max-seconds 15 --max-hits 1024
```

`--max-mib` aceita 1..256 MiB de leituras tentadas; `--max-seconds` aceita 1..60 segundos; `--max-hits` aceita 1..4096 correspondencias. Padrao: 64 MiB, 15 s, 1024 hits. Blocos de leitura de 64 KiB e nenhuma varredura recursiva. **O limite de tempo e verificado entre blocos, nao interrompe uma chamada de leitura do sistema.**

O v2 gera `aoe2hd_unit_references_v2.csv` com endereco da regiao, protecao, endereco de ocorrencia, palavras vizinhas e indicacao `selection_slot`. Tambem preserva o CSV legado `aoe2hd_unit_references.csv` das regioes pequenas. Ambos sao sobrescritos; ao detectar alteracao da ancora antes do fim, os CSVs incompletos sao excluidos.

### Experimento

1. Selecione exatamente um aldeao e execute o v2.
2. Renomeie/salve o CSV.
3. Selecione outro aldeao e repita.
4. Investigue regioes semelhantes que contenham ambas as ancoras em execucoes diferentes, **especialmente fora dos slots de selecao**. O programa ainda nao valida persistencia sem selecao automaticamente.

**Cuidado:** correspondencia de ponteiro nao comprova lista completa de unidades; o processo pode conter caches, referencias temporarias ou enderecos reutilizados. Regioes inacessiveis, nao alinhadas, fora do teto de bytes/tempo ou que mudem durante a leitura podem passar despercebidas. A busca ampla e direta; a investigacao indireta restrita da v1 continua disponivel no CSV antigo.

## UnitReferenceDiscovery v3 — priorizacao e persistencia

A v3 e um executavel independente, read-only. Ela utiliza os mesmos validadores de selecao da v2. Em vez de esgotar o limite lendo somente enderecos baixos, enumera regioes legiveis por `VirtualQueryEx` e prioriza as regioes que contem `World`, `LocalPlayer` ou a unidade selecionada. Depois examina as demais regioes em ordem crescente. A varredura continua limitada em bytes tentados, tempo e numero de correspondencias.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_unit_reference_discovery_v3.exe --max-mib 64 --max-seconds 15 --max-hits 1024
```

Selecione exatamente **um aldeao** antes de iniciar. O programa registra todas as referencias diretas encontradas, confirma que o objeto permaneceu selecionado ao final da varredura e solicita que voce **selecione outro aldeao sem mudar de partida**. Pressione Enter depois da nova selecao. A v3 rele os enderecos anteriormente encontrados e grava `aoe2hd_unit_references_v3.csv`.

Colunas incluem `anchor`, `anchor_id`, `second_anchor`, `second_id`, `region_base`, `address`, `offset_in_region`, `before`, `after`, `selection_slot`, `observed_value_after`, `persisted`, `nearby_minus8`, `nearby_plus8`, `status`. `persisted=1` significa apenas que o endereco ainda continha o ponteiro original depois da troca de selecao. Nao garante que seja uma colecao de unidades. Proximidade de ponteiros similares deve ser confirmada em diversas repeticoes. O arquivo sera sobrescrito na proxima execucao.

Limites: a busca nao cobre necessariamente toda a memoria, nao atravessa varias indirecoes, nao fornece um snapshot atomico e nao testa se um objeto sobrevive a morte/remocao. Os valores de vizinhanca sao evidencias, nao provas de uma estrutura de vetor. O v3 nao atribui tipo de unidade sem confirmacao adicional.

## UnitReferenceDiscovery v4 — dois aldeoes simultaneamente

Esta versao independente procura referencias **simultaneas** a dois objetos conhecidos, sem depender de manter o primeiro aldeao selecionado. O primeiro aldeao e identificado pela selecao; depois voce seleciona um segundo aldeao e confirma com Enter. A ferramenta valida que a identidade candidata do primeiro objeto ainda pode ser lida e realiza uma varredura bounded somente leitura, com regioes de `World`, `LocalPlayer` e objeto selecionado priorizadas.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_unit_reference_discovery_v4.exe --max-mib 64 --max-seconds 15 --max-hits 2048
```

Apos selecionar o **primeiro** aldeao, inicie o programa; quando solicitado selecione um **segundo** aldeao diferente e pressione Enter. A v4 procura ambos os ponteiros ao mesmo tempo e gera:

- `aoe2hd_unit_references_v4.csv`: todos os enderecos coincidentes, papel da ancora, indicacao do slot de selecao e dois DWORDs vizinhos;
- `aoe2hd_unit_reference_pairs_v4.csv`: pares de referencias a aldeoes distintos cuja distancia na memoria e de no maximo 256 bytes, excluindo os slots diretos de selecao.

**Interpretacao cautelosa:** um par proximo pode indicar um array, um cache ou duas estruturas vizinhas nao relacionadas. A distancia nao demonstra parentesco estrutural ou alcancabilidade a partir de World. A validacao dos ponteiros nao prova que os objetos sejam aldeoes; selecione-os visualmente no jogo. O programa nao percorre cadeias indiretas arbitrarias, nao produz enumeracao automatica e pode perder referencias se esgotar o limite de bytes, tempo ou hits. Os CSVs sao sobrescritos na proxima execucao. Esses limites sao de investigacao, nao de cobertura completa do processo.

## UnitCollectionDiscovery v1 — candidatos a colecoes com duas ancoras

Ferramenta nova, **somente leitura**, separada do UnitReferenceDiscovery. Usa os campos de selecao e identidade experimental da v0 para obter dois aldeoes diferentes da mesma partida. Selecione o primeiro, execute e selecione o segundo quando solicitado. A ferramenta confere que o primeiro objeto continua legivel e com o mesmo ID candidato.

Em vez de executar outra varredura sequencial de 64 MiB, procura triplas candidatas `[begin,end,capacity]` em ate `0x800` bytes de `World` e `LocalPlayer`. Cada buffer tem limite de 4096 bytes de conteudo (capacidade ate 8192); examina strides de 4 e 8 bytes e ate 512 entradas. Para cada campo, testa referencia direta a **ambos** os ponteiros, ou referencia indireta por meio dos primeiros dois DWORDs de um intermediario legivel. Nao identifica automaticamente listas de unidades.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_unit_collection_discovery.exe
```

Arquivo: `aoe2hd_unit_collections_v1.csv` com offsets do root, endereco do buffer, stride, contagem, indices e contadores de coincidencias diretas/indiretas por ancora, alem de `both`. `both=1` e apenas uma hipotese de colecao que contem evidencias de duas unidades. `both=0` NAO demonstra ausencia de unidade no jogo: a busca e restrita aos roots, ao tamanho dos buffers e a um salto indireto. Proximo experimento recomendado: repetir em mais de uma partida e verificar se a estrutura sobrevive a mudancas de selecao. Arquivo sobrescrito a cada execucao.

## UnitCollectionDiscovery v2 — procura orientada por ID

Executavel independente (somente leitura). Captura **dois aldeoes selecionados sequencialmente**, preserva seus IDs candidatos (`object+0x08`) e verifica a estabilidade da sessao antes de exportar. Analisa somente triplas `begin/end/capacity` encontradas ate `0x800` bytes de `World` e `LocalPlayer`, com buffer ate 8192 bytes. Examina palavras alinhadas de 32 bits contendo cada ID, sob hipoteses de stride 4, 8, 12, 16, 24, 32 e 64 bytes. Registra posicao por registro e valores vizinhos; as varias hipoteses de stride para o mesmo buffer NAO representam evidencias independentes.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_unit_collection_discovery_v2.exe
```

Selecione primeiro um aldeao, execute e, quando solicitado, selecione um segundo aldeao distinto **na mesma partida**, pressionando Enter para iniciar a descoberta. Resultado: `aoe2hd_unit_identity_candidates_v2.csv`, com identidade candidata, origem, root, offset, buffer, stride, indice, offset interno e vizinhos. O campo `with_both_ids` no terminal significa apenas que ambos os numeros apareceram em um buffer candidato. **Nao** prova que seja lista de unidades ativas, que seja indexada por ID, nem que um ID resolva o ponteiro correto. Um CSV vazio tambem nao prova ausencia de colecao: a busca cobre apenas vetores candidatos de dois roots e nao percorre arvores, hash maps ou outros indices. Arquivo sobrescrito por novas execucoes.

## EntityGraphDiscovery v1 — caminhos estruturais limitados

Novo executavel `aoe2hd_entity_graph_discovery.exe`, somente leitura. Identifique visualmente dois aldeoes da mesma partida: selecione o primeiro e inicie; selecione o segundo quando solicitado e pressione Enter.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_entity_graph_discovery.exe
```

O algoritmo parte de `World` e `LocalPlayer`, examina ate 256 bytes por no, acompanha campos de 32 bits alinhados com valores plausiveis de ponteiro ate **dois saltos** e deduplica nos por endereco. Limites: ate 512 nos e 8192 arestas, nenhuma leitura de escrita. Nao usa os objetos conhecidos como raizes, pois isso criaria evidencias artificiais. Os IDs candidatos dos dois aldeoes sao conferidos antes da exportacao.

Gera `aoe2hd_entity_graph_edges.csv` (origem, offset, destino, profundidade, correspondencia com cada ancora) e `aoe2hd_entity_graph_summary.csv` (quantidade de nos/arestas, correspondencias, falhas e indicador `capped`). Interpretacao: `is_first=1` ou `is_second=1` identifica uma coincidencia de ponteiro num caminho percorrido, **nao** uma colecao de unidades confirmada. Limites de profundidade/bytes/no e filtros podem impedir encontrar caminhos reais. Se `capped=1`, o resultado e incompleto. CSVs sobrescritos a cada execucao.

## EntityGraphDiscovery v2 — cobertura adaptativa e seleção explícita

A v2 preserva o executável v1 para comparação. Os roots `World` e `LocalPlayer` recebem janelas de `0x400` bytes (incluindo `LocalPlayer+0x15C/+0x160`); nós desconhecidos recebem `0x100` bytes. No máximo dois saltos, 512 nós e 8192 arestas. Os slots de seleção são classificados como `selection_edge=1` e não servem como expansão de nós, evitando interpretar a seleção como descoberta de armazenamento.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_entity_graph_discovery_v2.exe
```

Selecione um aldeão antes de executar e um segundo, diferente, quando solicitado. Saídas: `aoe2hd_entity_graph_edges_v2.csv` e `aoe2hd_entity_graph_summary_v2.csv`. A coluna `selection_edge` distingue arestas originadas nos slots conhecidos, enquanto `selection_matches` e `nonselection_matches` resumem correspondências. Um match não relacionado a esses dois offsets ainda pode ser referência auxiliar de seleção, não uma coleção confirmada. Caminhos são hipóteses e a exploração pode não encontrar índices, tabelas hash ou outros mecanismos não representados por DWORDs alinhados.

## EntityGraphDiscovery v3 — evidência estrutural e busca reversa

A v3 preserva a v2 e adiciona testes de legibilidade nos destinos das arestas, além de uma varredura reversa **somente leitura**, limitada a 64 MiB, em regiões que contêm as âncoras ou os destinos observados no grafo. Os slots `LocalPlayer+0x15C/+0x160` são classificados explicitamente. A varredura reversa procura correspondências exatas a endereços candidatos, sem afirmar que correspondem a uma lista de unidades.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_entity_graph_discovery_v3.exe
```

Selecione um aldeão, execute o programa, selecione outro aldeão da mesma partida e pressione Enter. CSVs: `aoe2hd_entity_graph_edges_v3.csv`, `aoe2hd_entity_graph_summary_v3.csv` e `aoe2hd_entity_graph_reverse_v3.csv`. O resumo informa destinos legíveis, arestas ilegíveis, referências reversas e bytes examinados. Atenção: destino legível não equivale a objeto válido, referências reversas podem ser caches de seleção e 64 MiB não garantem cobertura completa. Não há confirmação de coleção ou enumeração de aldeões sem seleção.

## EntityGraphDiscovery v4 — reverse scan com cotas separadas e persistência A–B–A

Nova ferramenta independente, **somente leitura**, sem alterar v3. Capta aldeão A, pede seleção de aldeão B, percorre regiões legíveis do processo (com regiões de World/LocalPlayer/B priorizadas) procurando exclusivamente os dois endereços de objetos. Aplica limite de resultados **por âncora** para que correspondências repetitivas de B não consumam a cota de A. A busca para por orçamento de bytes ou quando ambas as cotas atingem seu limite.

Após a varredura, solicita que A seja selecionado novamente, valida as identidades candidatas dos dois objetos e relê cada endereço onde havia referência. Registra se o valor permaneceu apontando a A, permaneceu apontando a B, passou a apontar a outro valor ou tornou-se ilegível. Registra também os slots conhecidos `LocalPlayer+0x15C/+0x160`. A persistência sozinha **não** confirma uma lista de unidades.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_entity_graph_discovery_v4.exe --max-mib 128 --per-anchor 2048
```

Arquivos: `aoe2hd_entity_graph_reverse_v4.csv` (endereços e persistência A–B–A) e `aoe2hd_entity_graph_summary_v4.csv` (cobertura efetiva, contadores, saturação por âncora). Os limites de bytes são orçamento, não cobertura completa; nem todas as regiões são necessariamente percorridas. Análise reversa é de ponteiros diretos, não busca de indireções ilimitadas. Saídas são sobrescritas em uma nova execução.

## EntityGraphDiscovery v5 — raízes de jogadores, cobertura e persistência

Versão independente e somente leitura que preserva a v4. O procedimento experimental é **A → B → A**: selecione A antes de iniciar, B após o primeiro prompt, e A novamente depois da varredura. Confere o ID candidato dos dois objetos e estabilidade de World/LocalPlayer.

A v5 extrai ponteiros de jogadores a partir da coleção conhecida `World+0x184/+0x188` (entradas de 8 bytes), registra raízes em CSV e **prioriza as regiões** que contêm essas raízes e as duas âncoras. Isso ainda não é uma travessia completa a partir de cada jogador. A busca de referências é direta, com cotas independentes por âncora, e contabiliza os bytes tentados e os realmente lidos por região. Depois da seleção final, `unchanged_first` e `unchanged_second` significam conteúdo efetivamente preservado no mesmo endereço; `switched_second_to_first` significa referência que passou de B para A. `both_anchors_seen=1` é somente coexistência de referências no intervalo examinado, **não** confirmação de mecanismo de enumeração.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_entity_graph_discovery_v5.exe --max-mib 128 --per-anchor 2048
```

Resultados: `aoe2hd_entity_roots_v5.csv` (raízes), `aoe2hd_entity_coverage_v5.csv` (regiões tentadas, bytes efetivos, erros), `aoe2hd_entity_references_v5.csv` (referências A/B, valores após retorno a A) e `aoe2hd_entity_summary_v5.csv`. Regiões não visitadas não têm linhas no CSV de cobertura: use `regions_available` e `regions_examined` para distinguir a cobertura incompleta. `attempted_full` indica apenas que toda a região foi **tentada**, não que seus bytes foram inteiramente legíveis. Resultados de referência permanecem hipóteses.

## UnitArrayCandidateValidator v1

Ferramenta independente, **somente leitura**, voltada à hipótese de uma coleção com entradas de 8 bytes. Selecione o aldeão A, execute o validador, selecione B e pressione Enter. A ferramenta faz busca automática pelos dois endereços na memória legível dentro do orçamento configurado e procura referências distantes exatamente 8 bytes. Examina dez posições próximas de cada candidato com campos ponteiro/metadado, ID candidato do objeto em +0x08 e HP em +0x34. Procura referências diretas à região candidata nos primeiros 0x800 bytes de World, LocalPlayer e jogadores conhecidos.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_unit_array_candidate_validator.exe --max-mib 128
\`\`\`

Quando solicitado, crie um aldeão **sem selecioná-lo** e pressione Enter para realizar nova leitura dos candidatos existentes. Se não for possível criar uma unidade, pressione Enter sem fazer alterações; isso é controle, não teste de criação. Saídas:

- \`aoe2hd_unit_array_candidates_v1.csv\`: amostras de vizinhança e plausibilidade do objeto;
- \`aoe2hd_unit_array_root_refs_v1.csv\`: referências diretas ou próximas à base candidata em raízes conhecidas;
- \`aoe2hd_unit_array_delta_v1.csv\`: instantâneo após o teste.

**Limitações**: o relatório delta registra o estado posterior, e a comparação temporal detalhada pode ser feita pela junção por \`candidate,slot\` com o CSV inicial; não infere automaticamente criação de objeto. Proximidade de 8 bytes, leitura plausível de HP ou IDs distintos não confirma coleção. O orçamento de memória pode deixar regiões sem varredura. Os endereços de uma execução não devem ser reutilizados na próxima.

## UnitArrayCandidateValidator v2

Mantém a v1 intacta. A v2 não exige manter o segundo aldeão selecionado após criar um novo: valida diretamente o World e os IDs candidatos de A/B. Registra regiões tentadas e bytes efetivamente lidos em \`aoe2hd_unit_array_coverage_v2.csv\`. Armazena as entradas candidatas antes da criação e emite comparação por slot com colunas before/after e changed em \`aoe2hd_unit_array_delta_v2.csv\`. Os demais resultados são \`aoe2hd_unit_array_candidates_v2.csv\` e \`aoe2hd_unit_array_root_refs_v2.csv\`.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_unit_array_candidate_validator_v2.exe --max-mib 128
\`\`\`

Selecione A inicialmente, depois B quando solicitado; após a primeira leitura, crie um aldeão usando o Centro Urbano sem selecionar a unidade recém-criada, então pressione Enter. Se nenhum par for encontrado, os arquivos de candidatos e delta podem conter apenas cabeçalhos: isso é resultado inconclusivo, não falha do jogo. A busca permanece limitada por bytes e não demonstra cobertura de todo o processo.

## UnitArrayCandidateValidator v3 — descoberta dirigida e duas varreduras

Mantém os executáveis v1/v2. A v3 prioriza regiões legíveis contendo World, LocalPlayer, jogadores conhecidos ou qualquer dos dois objetos-âncora e conserva o restante na fila até esgotar o orçamento. Busca referências exatas aos dois objetos e preserva amostras locais mesmo quando apenas uma âncora foi encontrada. O protocolo é A selecionado → B selecionado → criação de aldeão sem exigir manter B selecionado → segunda varredura. Cada fase tem orçamento de memória próprio.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_unit_array_candidate_validator_v3.exe --max-mib 128
\`\`\`

Relatórios: \`aoe2hd_unit_array_candidates_v3.csv\` e \`aoe2hd_unit_array_root_refs_v3.csv\` para pares, \`aoe2hd_unit_array_delta_v3.csv\` para amostras before/after dos slots vistos antes da criação, \`aoe2hd_unit_array_rescan_v3.csv\` para referências A/B localizadas independentemente em cada fase, e \`aoe2hd_unit_array_coverage_v3.csv\` / \`aoe2hd_unit_array_coverage_after_v3.csv\` para regiões visitadas, parciais e **não visitadas**.

A v3 ainda não descobre automaticamente a identidade/ponteiro do terceiro aldeão, nem prova que uma entrada extra resulta da criação. Em \`pairs=0\`, os relatórios de pares podem ficar vazios, mas o delta das amostras de âncora única e o CSV da segunda varredura podem fornecer evidências adicionais. Os testes da plataforma Windows precisam ser executados localmente.

## UnitArrayCandidateValidator v4 — persistência entre seleções A–B–A

Nova ferramenta independente e **somente leitura**. Faz três buscas com o **mesmo orçamento** e na mesma ordem de regiões: A selecionado, B selecionado e novamente A selecionado. Em cada fase procura referências de 4 bytes aos objetos-âncora, cruza endereços e distingue referências que acompanham a seleção de referências que permaneceram apontando para A ou B. Procura distância de exatamente 8 bytes entre referências estáveis a A e B, sem afirmar que isso prova um array. Também exporta referências sem classificação.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_unit_array_candidate_validator_v4.exe --max-mib 128
\`\`\`

Antes de iniciar selecione A; após a primeira varredura, selecione B e pressione Enter; após a segunda, selecione A e pressione Enter novamente. Saídas: \`aoe2hd_unit_array_references_v4.csv\`, \`aoe2hd_unit_array_pairs_v4.csv\` e \`aoe2hd_unit_array_summary_v4.csv\`.

**Limitações:** o programa compara valores encontrados por varredura, não snapshots simultâneos. Ausência de correspondência em determinada fase é codificada como zero no relatório, e **não significa** que o endereço armazenava zero. A ordem de regiões é fixa para a sessão; endereços fora do orçamento não são examinados. Em particular, \`stable_B\` significa que uma referência permaneceu em B nas fases B e retorno a A, não que tenha sido observada na primeira fase. Continua sendo uma hipótese, não uma enumeração de unidades.

## PlayerUnitCollectionDiscovery v1 — vetores a partir dos jogadores

Ferramenta independente, somente leitura, que muda a estratégia de busca global por ponteiros para uma investigação de coleções **ligadas diretamente a World, LocalPlayer e entradas de jogadores**. As duas unidades selecionadas, A e B, servem exclusivamente como âncoras para verificar candidatos; não se usam slots de seleção para localizar coleções.

Lê janelas de até 0x800 bytes de cada raiz; procura triples de ponteiros \`begin/end/capacity\` plausíveis e, para cada tripla, testa entradas diretas com stride 4/8/12/16 e região de até 64 KiB. Registra candidatos com e sem as duas âncoras, com resultado \`dual_anchor_hypothesis\` quando ambas aparecem numa mesma interpretação.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_player_unit_collection_discovery.exe
\`\`\`

Selecione A antes de iniciar, B quando solicitado e pressione Enter. Gera \`aoe2hd_player_unit_collections_v1.csv\` e \`aoe2hd_player_unit_collections_summary_v1.csv\`.

**Limites:** examina somente *vetores diretos* nestas raízes, não listas encadeadas, indireções ou subobjetos; uma coincidência de ponteiros não prova propriedade nem enumeração. A ferramenta não varre toda a memória. Requer compilação/teste no Windows para validar a integração.

## PlayerUnitCollectionDiscovery v2 — inspeção estrutural das entradas

Preserva a v1. Parte de World/LocalPlayer e dos jogadores conhecidos, procura vetores plausíveis de stride 8 e inclui explicitamente a tripla de World+0x88 quando válida. Examina os campos +0 e +4 de cada entrada, tenta leitura direta de ID (+0x08) e HP (+0x34) do objeto candidato e uma única indireção. A validação é heurística: IDs/HP plausíveis não comprovam classe nem propriedade da unidade.

Selecionar aldeão A, iniciar a ferramenta, selecionar aldeão B e pressionar Enter. Depois da primeira captura, selecionar Centro Urbano se necessário, criar um aldeão sem selecioná-lo e pressionar Enter. A ferramenta valida World e identidade candidata de A/B diretamente, sem exigir seleção constante, e compara os mesmos slots antes/depois.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_player_unit_collection_discovery_v2.exe
\`\`\`

Arquivos: \`aoe2hd_player_unit_vectors_v2.csv\` (vetores de stride 8), \`aoe2hd_player_unit_entries_v2.csv\` (todos os campos e duas fases), \`aoe2hd_player_unit_roots_v2.csv\` (quantidade de vetores plausíveis e palavras legíveis por raiz) e \`aoe2hd_player_unit_delta_v2.csv\` (alterações nos mesmos slots).

Limitações importantes: somente vetores diretos nas raízes e uma indireção no conteúdo da entrada; não percorre grafos arbitrários ou listas encadeadas. Vetores recém-criados ou realocados depois do snapshot inicial não serão descobertos automaticamente na segunda fase. Uma mudança de ID ou HP pode ser causada por outros eventos do jogo. Os testes de integração Windows devem ser executados localmente.

## PlayerUnitCollectionDiscovery v3 — classificação estrutural

Executável independente, somente leitura. Parte das âncoras A e B obtidas por seleção manual e depois inspeciona \`World+0x88\` como vetor de **ponteiros de 4 bytes**, verificando endereço, vtable, campo candidato a ID em +0x08 e HP candidato em +0x34. O objeto é classificado como âncora conhecida, semelhante por vtable, sem HP típico, plausível desconhecido ou ilegível. Essas classes são evidências comparativas, nunca prova definitiva de tipo. Examina ainda uma indireção a partir dos campos dos jogadores conhecidos, excluindo os slots de seleção do LocalPlayer e os próprios objetos A/B como raízes, para testar possíveis vetores em subobjetos.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_player_unit_collection_discovery_v3.exe
\`\`\`

Selecione A antes de iniciar; quando solicitado, selecione outro aldeão B e pressione Enter. CSVs: \`aoe2hd_player_objects_v3.csv\` (objetos apontados por World+0x88), \`aoe2hd_player_subobjects_v3.csv\` (vetores encontrados em subobjetos dos jogadores) e \`aoe2hd_player_summary_v3.csv\` (contadores). A v3 não exige criação de terceiro aldeão. A investigação é limitada a janelas de 0x800 bytes dos jogadores, subobjetos examinados em 0x400 bytes, vetores pequenos e uma indireção; não faz descoberta exaustiva. A execução Windows ainda precisa ser validada.

## PlayerUnitCollectionDiscovery v4 — subobjetos de Player+0x14

Nova ferramenta independente, somente leitura. Selecionar A, executar, selecionar outro aldeão B e pressionar Enter. A v4 enumera os jogadores conhecidos, segue **somente** o ponteiro Player+0x14 e captura até 0x400 bytes por subobjeto próprio distinto. Registra campos e testa uma hipótese limitada de tabela contígua de ponteiros com contagem adjacente (ordens pointer/count e count/pointer; 2–2048 entradas). Descarta como raiz World e os endereços das duas âncoras. Após a primeira captura, pode selecionar o Centro Urbano e criar um terceiro aldeão sem selecioná-lo; pressione Enter para comparar os mesmos campos, validando diretamente World e as duas identidades.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_player_unit_collection_discovery_v4.exe
\`\`\`

Arquivos: \`aoe2hd_player_owned_layout_v4.csv\` (campos brutos por jogador), \`aoe2hd_player_owned_evidence_v4.csv\` (hipóteses count/pointer e ocorrências A/B) e \`aoe2hd_player_owned_delta_v4.csv\` (comparação dos campos antes/depois). A ausência de mudanças nos 0x400 bytes monitorados não prova ausência de mutação em outras estruturas; a versão não segue ponteiros de tabelas realocadas nem identifica automaticamente a terceira unidade. As correspondências permanecem hipóteses, não enumeração confirmada.

## PlayerUnitCollectionDiscovery v5 — rastreamento de conteúdo indireto

Ferramenta independente, somente leitura. Com aldeão A selecionado, inicie a v5, selecione B e pressione Enter. A ferramenta enumera as raízes dos jogadores e segue \`Player+0x14\`; lê os primeiros 0x400 bytes de cada subobjeto distinto, filtra valores alinhados como possíveis ponteiros e captura até **2048 endereços de destino distintos** com snapshots de **128 bytes** cada. Deduplica destinos comuns e registra todas as origens \`jogador → subobjeto → campo → destino\`.

Após a captura, crie um aldeão (é permitido selecionar o Centro Urbano) sem selecionar o recém-criado e pressione Enter. A v5 valida diretamente os IDs candidatos de A/B e World, relê os mesmos endereços, computa bytes alterados e procura ponteiros diretos para A/B dentro dos 128 bytes.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_player_unit_collection_discovery_v5.exe
\`\`\`

CSV: \`aoe2hd_player_indirect_delta_v5.csv\` (mudanças por destino), \`aoe2hd_player_indirect_paths_v5.csv\` (origem dos destinos) e \`aoe2hd_player_indirect_summary_v5.csv\` (resumo). A leitura atual exige que os 128 bytes estejam inteiramente legíveis; candidatos parciais ficam fora. Quando o destino se torna ilegível após a ação, \`changed_bytes=0\` significa comparação indisponível, **não** ausência de alteração. O experimento observa somente endereços capturados antes da criação e não descobre automaticamente novas tabelas ou a identidade da unidade criada. Mudanças de conteúdo não constituem comprovação de uma coleção de unidades.

## WorldDynamicUnitDiscovery v1 — diferencial controlado

Executável independente e **somente leitura**. Captura os primeiros **0x800 bytes** de World e dos objetos de jogadores conhecidos em três fases: inicial, controle (tempo decorrido sem criar unidade) e após **criar exatamente um aldeão sem selecioná-lo**. Não usa os slots de seleção para encontrar coleções. A mudança exclusiva da fase de criação é marcada como \`creation_only\`, porém isso não demonstra causalidade: recursos, tempo, animações e outras ações também podem mudar.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_world_dynamic_unit_discovery.exe
\`\`\`

Comece com um aldeão conhecido selecionado para validar a sessão, e mantenha a partida ativa. Quando solicitado, apenas deixe transcorrer um intervalo sem criar unidades e pressione Enter (controle). Depois, crie um aldeão; é permitido selecionar o Centro Urbano; pressione Enter após a conclusão. Saídas: \`aoe2hd_world_dynamic_fields_v1.csv\`, \`aoe2hd_world_dynamic_roots_v1.csv\` e \`aoe2hd_world_dynamic_summary_v1.csv\`.

**Limitações:** compara janelas estruturais fixas, não heaps arbitrários ou conteúdo de ponteiros. Apenas mudanças alinhadas de 4 bytes são registradas. A conclusão é limitada à janela de cada raiz que permaneceu inteiramente legível nas três fases. A versão ainda não testa uma segunda criação nem comprova contadores de unidade; exige confirmação em múltiplos experimentos. Integração Windows ainda não verificada na sessão de desenvolvimento.

## WorldDynamicUnitDiscovery v2 — duas criações com controles intercalados

Executável independente e read-only. Depois de selecionar um aldeão para validar a sessão, execute o programa, **selecione o Centro Urbano e mantenha a mesma seleção nas cinco capturas**. Os cinco snapshots são: inicial, controle 1 (nenhuma criação), criação 1 (exatamente um aldeão concluído), controle 2 (nenhuma criação), criação 2 (mais um aldeão concluído). A ferramenta valida ambos os slots de seleção antes/depois de cada captura e a identidade da primeira âncora no final; abandona o experimento se a seleção variar. Examina até 0x800 bytes de World e dos objetos de jogadores conhecidos. \`clean_repeat\` exige estabilidade nos dois controles e modificação nas duas criações; \`increment_one\` exige adicionalmente incremento de exatamente 1 nos dois eventos, interpretando campos como uint32.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_world_dynamic_unit_discovery_v2.exe
\`\`\`

CSV: \`aoe2hd_world_dynamic_fields_v2.csv\`, \`aoe2hd_world_dynamic_roots_v2.csv\`, \`aoe2hd_world_dynamic_summary_v2.csv\`. Campos \`clean_repeat\` e \`increment_one\` são **hipóteses correlacionais**, não prova de contagem de aldeões. As duas criações também consomem recursos e avançam o tempo. Esta versão não distingue tipos de estruturas além de comparação de words, não examina listas em heaps e não usa leitura de seleção para enumerar unidades. A compilação no Windows deve ser validada no ambiente de execução do jogo.

## WorldDynamicUnitDiscovery v3 — classificação de contador candidato

Executável independente, read-only. Investiga especificamente o campo \`LocalPlayer+0x37C\` (offset 892), cujos 32 bits foram observados em v2 como \`0 → 65536 → 131072\`. A v3 lê separadamente a metade inferior (\`+0x37C\`) e a superior (\`+0x37E\`) como **inteiros não sinalizados de 16 bits**, acompanhando a mesma janela de 64 bytes a partir de \`Player+0x370\` nos jogadores conhecidos. O programa não afirma antecipadamente que o campo conte aldeões.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_world_dynamic_unit_discovery_v3.exe
\`\`\`

Procedimento: selecione primeiro um aldeão para inicializar a âncora. Quando o programa solicitar, selecione o Centro Urbano, que **deve permanecer selecionado em todas as capturas**. Capture baseline, controle sem alterações voluntárias, criação concluída de um aldeão, segunda criação concluída, evento opcional de uma unidade que não seja aldeão, e evento opcional de uma pesquisa. Para as duas etapas opcionais, informe explicitamente \`y\` se o evento ocorreu ou \`n\` se foi pulado. Caso só tenha um Centro Urbano e não consiga produzir outra unidade sem mudar a seleção, pule o evento (\`n\`); não mude a seleção para completar uma etapa opcional.

Arquivos: \`aoe2hd_world_dynamic_counter_values_v3.csv\` (word32 + halfwords de cada jogador em cada fase), \`aoe2hd_world_dynamic_counter_deltas_v3.csv\` (mudanças por evento, com \`event_completed\`) e \`aoe2hd_world_dynamic_counter_neighborhood_v3.csv\` (janela estrutural de 64 bytes). As capturas não são simultâneas para jogadores diferentes e \`event_completed\` é uma declaração do operador; sem confirmação independente, não comprova causalidade. A leitura exige manter identidades/World e a seleção; o campo ainda pode ser contador de fila, estatística ou outro estado, não população atual nem coleção de unidades.

## WorldDynamicUnitDiscovery v4 — eventos de fila e conclusão

Ferramenta independente, somente leitura. Examina as janelas de 0x800 bytes de World e de cada jogador conhecido, acompanhando também o campo suspeito \`LocalPlayer+0x37C\` dentro da janela. Em vez de atribuir uma alteração à criação em geral, registra seis capturas: baseline, controle, início da fila, treinamento, conclusão e pós-controle. A seleção do Centro Urbano deve permanecer idêntica do começo ao fim; qualquer mudança invalida a coleta. Cada estágio de evento recebe uma declaração explícita do operador (\`y\`/\`n\`); a ferramenta **não** verifica autonomamente que o aldeão surgiu ou que a fila foi iniciada. Por isso todos os relatórios marcam \`independent_event_verified=0\`.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_world_dynamic_unit_discovery_v4.exe
\`\`\`

Saídas: \`aoe2hd_world_dynamic_fields_v4.csv\` (valores nos seis estágios, alterações de 32 bits e flags de mudanças exclusivas à fila ou à conclusão), \`aoe2hd_world_dynamic_roots_v4.csv\` (cobertura dos snapshots) e \`aoe2hd_world_dynamic_stages_v4.csv\` (confirmações manuais dos eventos). Caso não consiga capturar o treinamento ainda em andamento, informe \`n\`; não interprete o estágio correspondente como treinamento confirmado. A classificação de mudanças é correlacional: alterações de recursos, tempo e outras variáveis podem coincidir com as etapas. Esta versão **não possui um identificador independente de aldeões nascidos** e não confirma população ou enumeração de unidades.
