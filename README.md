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

## WorldDynamicUnitDiscovery v5 — teste de fila de aldeões

Executável separado e **somente leitura**, sem substituir as versões anteriores. Inspeciona dois candidatos de 16 bits (metade alta): \`LocalPlayer+0x37E\` como possível estado de fila, e \`LocalPlayer+0x522\` como possível estatística de conclusão. Captura uma janela de \`LocalPlayer+0x360..0x543\` (words de 4 bytes) em sete etapas: **baseline, fila 1, fila 2, fila 3, cancelar 1, conclusão 1, conclusão 2**. A ferramenta pergunta explicitamente quantos aldeões estão visivelmente na fila (incluindo o que está em treinamento) e quantos foram concluídos desde a linha de base, além de pedir \`y/n\` para confirmação visual. Essas contagens são observações do operador, **não verificações automáticas do jogo**.

Antes de iniciar a ferramenta, selecione um aldeão conhecido para obter a âncora. Depois, selecione um Centro Urbano com fila vazia e mantenha-o selecionado até o fim. Garanta população e recursos suficientes. Entre capturas, se alguma unidade concluir antes do momento esperado, **informe os valores realmente observados**; não force a contagem prevista nem marque a observação como confirmada sem verificar. O treinamento continua avançando enquanto o programa aguarda; para um experimento mais controlado, use velocidade baixa ou pause quando possível.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_world_dynamic_unit_discovery_v5.exe
\`\`\`

Saídas: \`aoe2hd_world_dynamic_queue_values_v5.csv\`, \`aoe2hd_world_dynamic_queue_deltas_v5.csv\`, \`aoe2hd_world_dynamic_queue_neighborhood_v5.csv\`. \`queue_matches_all\` e \`completion_matches_all\` exigem que as variações dos campos acompanhados reproduzam as contagens informadas em todas as transições confirmadas. Nenhuma correspondência demonstra, por si só, que exista uma coleção de objetos-aldeões no mapa; apenas valida a hipótese de um contador relacionado à fila/conclusão. Não há evento verificado independentemente, e a ferramenta não verifica o conteúdo da fila via estruturas internas.

## ProductionQueueStructureDiscovery v1 — fila do Centro Urbano

Ferramenta independente, **somente leitura**. Ao contrário da exploração dos campos dos jogadores, esta versão parte de **um Centro Urbano selecionado**, validado manualmente pelo operador. Inspeciona 4096 bytes do objeto selecionado e procura ponteiros alinhados em seus campos; acompanha até 256 destinos indiretos distintos que permitam leitura completa de 128 bytes. Descoberta e captura são limitadas para controlar custo e volume de dados, e nenhuma classificação de tipo por vtable é presumida.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_production_queue_structure_discovery.exe
\`\`\`

**Antes de iniciar, selecione um Centro Urbano com fila vazia**. Confirme \`y\` que a seleção é realmente um Centro Urbano e não a altere em nenhum momento. O programa realiza sete capturas: fila vazia, 1, 2 e 3 aldeões na fila, cancelamento de 1, conclusão de 1 e conclusão de mais 1. A cada etapa informe a quantidade *realmente observada* na fila e as conclusões acumuladas, inclusive se houve conclusão inesperada entre capturas. Um \`queue_delta_matches_all\` verdadeiro significa apenas correlação ao longo dos seis intervalos confirmados, não propriedade comprovada de uma fila. Os três relatórios são \`aoe2hd_production_queue_fields_v1.csv\`, \`aoe2hd_production_queue_paths_v1.csv\` e \`aoe2hd_production_queue_phases_v1.csv\`.

**Limites:** acompanhamento de endereços fixos obtidos na captura inicial; objetos realocados ou novos destinos não são redescobertos depois. O comando não altera a memória do jogo, não descobre automaticamente aldeões no mapa e depende de confirmação visual; \`independent_verified=0\` indica que a ferramenta não confirma eventos diretamente. A seleção e o endereço do Centro Urbano permanecem constantes.

## ProductionQueueStructureDiscovery v2 — validação do layout da fila

Ferramenta independente de leitura de memória, preservando a v1. Com um **Centro Urbano selecionado e fila vazia**, confirma-se manualmente a identidade e mantém-se a seleção durante as sete etapas (vazio, +1, +2, +3, cancelar uma ordem, primeira e segunda conclusões). A ferramenta lê **TownCenter+0x21C** como palavra de 32 bits e compara separadamente o `low16` (posição +0x21C) e `high16` (+0x21E) às contagens visualmente informadas. Também fotografa os primeiros **4096 bytes** do objeto e até **256 destinos legíveis de 256 bytes** alcançados por um ponteiro direto no primeiro snapshot. Os candidatos a tripla `begin/end/capacity` são apenas hipóteses estruturais, não registros de fila validados.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_production_queue_structure_discovery_v2.exe
```

Arquivos: `aoe2hd_production_queue_layout_fields_v2.csv` (campos que mudam nas sete etapas), `aoe2hd_production_queue_layout_candidates_v2.csv` (triplas candidatas nos snapshots iniciais) e `aoe2hd_production_queue_layout_events_v2.csv` (fila observada, confirmação do operador e metades de 16 bits do contador). A seleção e o ID do Centro Urbano são revalidados durante o procedimento. As contagens observadas **não** são verificadas independentemente: `independent_verified=0`. A ferramenta não descobre destinos alocados após o primeiro snapshot, não acompanha o conteúdo de vetores candidatos em profundidade e não identifica unidades existentes no mapa.

## ProductionQueueStructureDiscovery v3 — ponteiro dinâmico TownCenter+0x214

Executável independente e somente leitura. **Selecione diretamente um Centro Urbano com fila vazia antes de iniciar.** A ferramenta obtém a âncora da seleção, solicita confirmação humana de que se trata de Centro Urbano e, em sete etapas (vazio, 1, 2, 3, cancelamento, conclusão 1, conclusão 2), relê os campos \`TownCenter+0x214\`, \`+0x218\` e \`+0x21C\`. Se \`+0x214\` for um ponteiro alinhado legível, captura os primeiros **512 bytes** do endereço atual em cada etapa. Ao contrário das versões anteriores, **segue também ponteiros que só aparecem depois que a fila começa a ser preenchida**. Comparações de palavras só são válidas se ambos os snapshots forem legíveis **e o ponteiro for idêntico**; caso contrário uma linha sinaliza comparação inválida (não ausência de mudanças). Valida continuamente a seleção e a identidade do Centro Urbano.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_production_queue_structure_discovery_v3.exe
\`\`\`

CSVs: \`aoe2hd_production_queue_pointer_v3.csv\` (ponteiro/contagem em cada fase e legibilidade), \`aoe2hd_production_queue_pointer_words_v3.csv\` (conteúdo completo dos 512 bytes legíveis por fase) e \`aoe2hd_production_queue_pointer_deltas_v3.csv\` (campos modificados em transições comparáveis, além de uma linha de invalidação quando necessário). A confirmação da fila continua visual, e não uma verificação independente. O conteúdo de 512 bytes pode não conter a estrutura completa; ponteiros internos ainda não são percorridos.

## ProductionQueueStructureDiscovery v4 — tipo e quantidade da fila

Experimento independente **somente leitura**, comparando *execuções separadas* com tipos diferentes de construções produtoras (ex.: Centro Urbano, Quartel, Estábulo). Não pressupõe que o layout do Centro Urbano seja válido para outras construções: apenas registra os campos candidatos \`Building+0x214\`, \`+0x218\`, \`+0x21C\` e os 512 bytes apontados por \`+0x214\` quando a leitura é válida. O primeiro DWORD do destino é interpretado **experimentalmente** como duas metades (\`low16\`, possível ID do tipo; \`high16\`, possível quantidade). Essa interpretação não é garantida.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_production_queue_structure_discovery_v4.exe
\`\`\`

Antes de iniciar, selecione *uma* construção produtora com fila vazia, confirme o tipo informado e **mantenha-a selecionada**. Capture as sete etapas: vazia, adicionar 1, adicionar 2 do mesmo tipo, adicionar 3 do mesmo tipo, cancelar 1, concluir 1, concluir 2. Informe o tamanho realmente observado e uma etiqueta para o tipo produzido. Repita a ferramenta em outra partida/construção (por exemplo, Quartel produzindo Milícia), e **renomeie/mova os CSVs entre execuções** para não sobrescrevê-los. Arquivos: \`aoe2hd_production_queue_types_v4.csv\` e \`aoe2hd_production_queue_type_changes_v4.csv\`.

A verificação das contagens continua sendo **visual**, não automática; \`independent_verified=0\`. Endereços absolutos variam entre execuções; diferenças do ponteiro invalidam comparações entre seus conteúdos. Não há identificação validada das entradas da fila nem enumeração de unidades existentes. Campos de outros edifícios podem ter semântica totalmente diferente; não classifique ponteiros ou contadores somente por semelhança numérica.

## ProductionQueueStructureDiscovery v5 — fila mista no Quartel

Ferramenta read-only independente: selecione um **Quartel vazio** e mantenha-o selecionado em todas as etapas. Captura campos \`Building+0x214/0x218/0x21C\`, 512 bytes do destino de \`+0x214\` e os primeiros 128 DWORDs deste destino, com metades de 16 bits separadas. Para cada fase, o operador informa **ordem visual exata** (separada por \`|\`), quantidade e confirmação. Etapas previstas: vazio; uma Milícia; Milícia + Lanceiro; Milícia + Lanceiro + Milícia; cancelar uma Milícia (se possível); conclusão da primeira unidade; conclusão da próxima. **Não invente confirmações:** caso o jogo só permita cancelar outra posição da fila, informe a ordem efetiva. Também é possível que a construção não permita uma das combinações; registre o estado real ou interrompa e relate o impedimento.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_production_queue_structure_discovery_v5.exe
\`\`\`

Relatórios: \`aoe2hd_production_queue_mixed_stages_v5.csv\`, \`aoe2hd_production_queue_mixed_words_v5.csv\` e \`aoe2hd_production_queue_mixed_deltas_v5.csv\`. A comparação ignora alterações de conteúdo entre endereços distintos e marca transições não comparáveis. \`root_count_matches\` e \`entry_count_matches_comparable\` são correspondências de diferenças quantitativas, não prova do tipo ou formato interno. Para descobrir a organização da fila mista, compare os campos por fase e por tipo solicitado. **Eventos e tipos continuam dependentes da observação do operador**, sem verificação independente da produção. Esta versão ainda não varre ponteiros de segundo nível nem enumera unidades existentes no mapa.

## ProductionQueueStructureDiscovery v6 — reconstrução de entradas e agrupamento

Executável separado, apenas leitura. Selecione um **Quartel com fila inicialmente vazia** e mantenha a seleção. A v6 segue \`Building+0x214\` em cada fase e lê 512 bytes quando possível; interpreta os primeiros 32 DWORDs como possíveis registros de 4 bytes contendo \`type_id low16\` e \`quantity high16\`. Compara as ordens visualmente confirmadas com possíveis alinhamentos de início no buffer, em dois modelos: **uma entrada por ordem** e **uma entrada por sequência consecutiva do mesmo tipo**, que pode ter quantidade maior que um. Correspondências são **candidatas**: não comprovam quais slots estão ativos.

O protocolo possui oito fases: vazia; Milícia; Milícia/Lanceiro; Milícia/Lanceiro/Lanceiro; Milícia/Lanceiro/Lanceiro/Milícia; cancelar última Milícia (quando possível); concluir primeira Milícia; concluir primeiro Lanceiro. O programa sempre solicita a fila **efetivamente observada**, usando \`M\` para Milícia e \`S\` para Lanceiro; por exemplo \`M|S|S|M\`; use \`empty\` quando vazia. Se o jogo não permitir a ação exata, informe o estado real e não confirme dados não observados. Também registra \`Building+0x218\` e \`+0x21C\` para confrontar metadados.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_production_queue_structure_discovery_v6.exe
\`\`\`

Arquivos: \`aoe2hd_production_queue_entries_v6.csv\` (slots decodificados), \`aoe2hd_production_queue_alignment_v6.csv\` (possíveis alinhamentos e modelos) e \`aoe2hd_production_queue_metadata_v6.csv\` (ponteiro e contadores por fase). Endereços absolutos não são portáveis entre partidas. Os eventos são relatados pelo operador, sem prova automática da ação; buffers com dados antigos podem gerar falsos positivos. Esta versão não enumera unidades já criadas no mapa.

## ProductionQueueStructureDiscovery v7 — enumeração ativa de filas de unidades

Ferramenta independente, read-only. Selecione uma construção produtora (ex.: Quartel) e mantenha sua seleção. Não execute pesquisa tecnológica durante o experimento: **pesquisa não pode ser enfileirada** como unidades, e seu estado interno não foi mapeado. A ferramenta usa \`Building+0x214\` como ponteiro para registros candidatos, \`Building+0x218\` (high16) como número de grupos e \`Building+0x21C\` (low16) como total de unidades. Lê até 512 bytes e enumera **somente os primeiros N grupos**, ignorando slots antigos além do limite. Cada grupo é um par \`uint16_t type, quantity\`. Rejeita inconsistências de contagens, quantidades nulas, tipos iguais em grupos adjacentes, excesso de grupos/unidades e falhas de leitura. Expande os grupos para a sequência de IDs e compara com a observação visual, que **não é uma confirmação automática independente**.

\`\`\`powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_production_queue_structure_discovery_v7.exe
\`\`\`

O programa solicita oito capturas (vazio, primeira, segunda, terceira, quarta inclusão, cancelamento e duas conclusões), mas é obrigatório informar a sequência *realmente* vista; use \`M\`=Milícia (74), \`S\`=Lanceiro (93), \`V\`=Aldeão (83), separadas por \`|\`, ou \`empty\`. CSVs: \`aoe2hd_production_queue_enumeration_v7.csv\` e \`aoe2hd_production_queue_groups_v7.csv\`. Esta primeira versão cobre esses três IDs conhecidos, não é um decodificador universal de tipos de unidade; o usuário deve validar o objeto de construção e a execução no jogo. Um snapshot vazio com ambos contadores zero é válido mesmo com ponteiro antigo persistente. O teste automatizado cobre enumeração, agregação, limite de grupos e condições inválidas. **Não enumera aldeões já existentes no mapa.**

## ProductionQueueReader — leitor reutilizável

`include/ProductionQueueReader.h` expõe `aoe2hd::ReadProductionQueue(building, readWord, readBytes)`, separando a lógica de leitura da interface do experimento v7. Retorna `ProductionQueueSnapshot` com `pointer`, `groupCount`, `unitCount`, `queue.groups`, `queue.expanded`, `status` e `valid()`. O leitor é genérico, somente leitura, demanda no máximo `4 * groupCount` bytes (até 512), respeita os limites de grupos/unidades e relê ponteiro e contadores após a cópia para detectar alterações durante a captura. Esses controles reduzem leituras inconsistentes, mas não garantem atomicidade frente a modificações rápidas do jogo. Pesquisa tecnológica e unidades vivas do mapa continuam fora de escopo.

A v7 utiliza agora o leitor reutilizável em vez de duplicar sua decodificação. Testes de regressão cobrem fila agrupada `74,93,93,74`, fila vazia, ponteiro inválido e mutação simulada entre leituras. Para validar no Windows: `cmake --build build-clean --config Release` e `ctest --test-dir build-clean -C Release --output-on-failure`.

## BuildingReferenceDiscovery v1 — referências persistentes a construções

Experimento independente e **somente leitura** destinado a descobrir referências a uma construção que sobrevivam à mudança da seleção. A ferramenta começa com **uma construção produtora conhecida selecionada** (essa seleção é apenas uma âncora experimental); examina regiões legíveis da memória do processo `AoK HD.exe`, com orçamento máximo de **64 MiB de bytes varridos** e **até 4 MiB por região**. Registra endereços cujas palavras de 32 bits apontam exatamente para o objeto selecionado. Depois solicita que você **selecione outro objeto ou desmarque a construção**, sem destruí-la, e relê as referências para identificar quais ainda apontam para ela. Também chama `ProductionQueueReader` com o endereço-âncora depois de mudar a seleção, verificando a possibilidade de leitura de sua fila sem mantê-la selecionada.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_building_reference_discovery_v1.exe
```

Arquivo: `aoe2hd_building_reference_persistence_v1.csv`, com endereço da referência, valor posterior e sinais de permanência nos primeiros 0x800 bytes de World/LocalPlayer. Saída de console: `persistent_refs`, `queue_read_valid`, `groups`, `units`. Referências encontradas são **candidatas**, não uma lista de construções: podem pertencer a cache, UI ou objetos transitórios. A varredura é deliberadamente limitada e não prova ausência de referências fora da cobertura. Mesmo se a fila continuar legível após a troca de seleção, isso apenas comprova leitura de um endereço **anteriormente conhecido**; enumeração autônoma de construções ainda exige descobrir a estrutura proprietária dos objetos e reproduzir a descoberta sem seleção inicial. A ferramenta não pesquisa tecnologias nem enumera aldeões presentes no mapa.

## BuildingObjectDiscovery v2 — duas construções e persistência

Ferramenta **somente leitura** para investigar referências a duas construções distintas sem confundir cache de seleção com coleção de objetos. Selecione a construção A, depois B (confirme ambas), e a ferramenta examinará até **96 MiB de regiões legíveis**, com limite de **4 MiB por região**, procurando referências diretas de 32 bits aos dois endereços. Depois selecione um terceiro objeto, diferente de A e B, e pressione Enter. O programa relê referências, conta as que persistem e verifica se ambas aparecem numa vizinhança de 128 bytes, classificando somente candidatos à mesma estrutura. Também consulta ProductionQueueReader para A e B após a terceira seleção.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_building_object_discovery_v2.exe
```

Relatórios: `aoe2hd_building_object_candidates_v2.csv` e `aoe2hd_building_object_summary_v2.csv`. Indicadores: `persistent_A`, `persistent_B`, `near_both`, `queue_A_valid`, `queue_B_valid`. **Limitação essencial:** a varredura procura endereços diretos, não coleções indexadas por IDs, arrays indiretos ou regiões excluídas pelo orçamento. A e B ainda são obtidas por seleção inicial, portanto persistência não demonstra descoberta autônoma de construções nem de aldeões no mapa. Não destrua os objetos durante a execução.

## BuildingObjectDiscovery v3 — caracterização de referências persistentes

Experimento **read-only** com construção A conhecida e uma ou mais referências candidatas. Primeiro selecione A e confirme. Informe o endereço da referência da v2 em hexadecimal (por exemplo `10880718`), ou informe `0` para repetir uma busca direta limitada a 96 MiB. **Endereços absolutos podem mudar entre partidas/processos**: sempre valide o endereço usando a âncora atual; o programa não presume que a referência antiga permanecerá válida.

Em seguida capture cinco estágios, mantendo A viva: `A` (selecionar A), `B` (outra construção), `C` (unidade diferente), `D_optional` (preferencialmente outra construção do mesmo tipo que A), `A_again` (voltar a A). A ferramenta registra o valor da referência em cada estágio e até 64 bytes anteriores e posteriores em palavras de 32 bits. Os resultados não comprovam por si só uma coleção, seu proprietário ou autonomia de descoberta.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
.\build-clean\Release\aoe2hd_building_object_discovery_v3.exe
```

Arquivos: `aoe2hd_building_reference_stages_v3.csv` e `aoe2hd_building_reference_windows_v3.csv`. Compare a coluna `points_to_A` entre estágios e procure vizinhos com transições coerentes, sem inferir estrutura apenas pela proximidade. A implementação ainda não enumera construções nem aldeões sem seleção.

### BuildingObjectDiscovery v3 — diagnóstico e retomada de captura

A v3 agora registra `aoe2hd_building_reference_diagnostics_v3.csv`, indicando separadamente `selection_unavailable`, `anchor_identity_changed_or_unreadable`, `owner_changed_or_unreadable` e `world_changed_or_unreadable`. Falhas transitórias de seleção permitem tentar a mesma fase novamente (pressione Enter após selecionar um objeto); `q` encerra conservando as capturas parciais. Os CSVs de estágios e janelas são descarregados em disco a cada etapa concluída. Se a âncora ou o contexto da partida mudar, o experimento interrompe sem inferir persistência. Protocolo do save: A = Centro Urbano 1, B = Quartel, C = Aldeão, D = Centro Urbano 2, A_again = Centro Urbano 1. As etiquetas de tipo são orientações ao operador, não classificações automáticas.

## BuildingObjectDiscovery v4 — Persistent Reference Neighborhood Analysis

Experimento somente leitura: selecione o **Centro Urbano A**, confirme sua identidade e informe uma referência persistente atual em hexadecimal. Informe `0` para procurar referências diretas a A em até 96 MiB (a busca lista candidatas e solicita a escolha de uma). Endereços absolutos de execuções anteriores **não são universais**; recomenda-se redescobrir candidatos em cada sessão. O programa examina 4 KiB alinhados à referência escolhida, de `ref - 2048` até `ref + 2047`, e repete cinco vezes: A, Quartel B, Aldeão C, Centro Urbano D e A novamente.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_building_object_discovery_v4.exe
```

Relatórios: `aoe2hd_building_reference_stages_v4.csv` (identidade da seleção e permanência da referência) e `aoe2hd_building_reference_window_words_v4.csv` (palavras de 32 bits da janela, valores, referências a A/seleção e potenciais ponteiros com IDs plausíveis). As saídas são descarregadas em disco a cada etapa concluída. O leitor verifica apenas valores parecidos com ponteiros como hipóteses; um ID numérico plausível e um ponteiro legível **não provam** objeto válido ou estrutura de coleção. A v4 ainda usa a seleção para estabelecer A e não enumera aldeões automaticamente.

## BuildingObjectDiscovery v5 — análise de estruturas repetidas

Experimento somente leitura baseado na v4. Após selecionar Centro Urbano A, informe referência atual em hexadecimal ou `0` para procurar candidatas; escolha uma referência plausivelmente persistente. Em cinco etapas (A, Quartel B, Aldeão C, Centro Urbano D, A_again), captura 4 KiB em torno da referência e testa distâncias **496 e 992 bytes** entre palavras de 32 bits.

A métrica `equal_nonzero` conta pares de valores iguais e não nulos; `nonzero_pairs` conta pares onde os dois valores são não nulos; `equal_zero` conta pares ambos iguais a zero, reportados separadamente. `nonzero_match_rate=equal_nonzero/nonzero_pairs` quando há pares elegíveis. Campos constantes e duplicação de estruturas não equivalem, isoladamente, a registros de objetos. Resultados precisam ser confrontados com endereços e IDs de objetos conhecidos em diferentes capturas.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_building_object_discovery_v5.exe
```

CSV: `aoe2hd_building_reference_stages_v5.csv`, `aoe2hd_building_reference_window_words_v5.csv` e `aoe2hd_building_stride_evidence_v5.csv`. O programa não prova coleção persistente ou enumeração de unidades não selecionadas.

## BuildingObjectDiscovery v6 — stride validation against controls

Experimento read-only, mantendo as cinco etapas de seleção A (Centro Urbano 1), B (Quartel), C (Aldeão), D (Centro Urbano 2) e A_again. Selecione inicialmente A, informe `0` para redescobrir referências na partida atual e escolha a candidata persistente. O programa lê uma janela de 4 KiB e avalia **481 espaçamentos**, de **128 a 2048 bytes**, em passos de 4 bytes.

Para cada espaçamento, registra pares comparados, pares com dois valores não nulos, correspondências não nulas, pares de zeros, correspondências de valores comuns (>3 ocorrências em uma mesma janela) e valores menos comuns (até 3 ocorrências). A razão `nonzero_match_rate` não inclui pares com zero. Essas métricas são exploratórias: a taxa pode continuar elevada por campos constantes, autocorrelação e comparações dependentes; **nenhum espaçamento é declarado estrutura confirmada automaticamente**.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_building_object_discovery_v6.exe
```

CSV: `aoe2hd_building_reference_stages_v6.csv`, `aoe2hd_building_reference_window_words_v6.csv`, `aoe2hd_building_stride_evidence_v6.csv` (496 e 992 bytes para comparação histórica) e `aoe2hd_building_stride_ranking_v6.csv` (todos os espaçamentos). O arquivo de ranking registra resultados por etapa, **não classificação ordenada final**: analise e ordene as métricas com controles e verificação cruzada entre etapas. A v6 ainda não encontra uma coleção confirmada de objetos nem enumera aldeões sem seleção.

## BuildingObjectDiscovery v7 — Object Identity Cross-Validation

Experimento **somente leitura** para cruzar identidades de quatro construções conhecidas com uma janela de 4 KiB ao redor de referência candidata. Registre, em sequência, Centro Urbano A, Quartel B, Centro Urbano D e Campo de Arqueiros E, confirmando o endereço e ID de cada objeto. Selecione A novamente e informe `0` para redescobrir referências na sessão; escolha a referência persistente. Depois faça seis capturas: A, B, um Aldeão C, D, E e A novamente. Os quatro endereços/IDs são coletados **antes** do experimento e usados como gabarito; a busca da janela não utiliza o ponteiro da seleção corrente para declarar correspondências.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_building_object_discovery_v7.exe
```

Resultados: `aoe2hd_building_object_identity_hits_v7.csv` (posição da referência e objeto-âncora coincidente) e `aoe2hd_building_object_identity_stages_v7.csv` (contagem de referências e objetos distintos em cada etapa). Os objetos precisam permanecer vivos e acessíveis. **Correspondências exatas demonstram apenas que a janela contém ponteiros para âncoras conhecidas**; não demonstram coleção completa, propriedade ou enumeração autônoma de construções e aldeões. Os testes cobrem comparação de múltiplos endereços conhecidos.

## PlayerObjectCollectionDiscovery v1 — busca multiâncora em World / LocalPlayer

Ferramenta read-only que registra por seleção **cinco âncoras conhecidas** (Centro Urbano A, Quartel B, Centro Urbano D, Campo de Arqueiros E, Aldeão C) e pede a seleção de um **sexto objeto distinto** antes da análise. A partir das raízes `World` e `LocalPlayer`, examina offsets de `0x000` a `0x3FC` em incrementos de quatro bytes. Se um valor é um ponteiro plausível para uma janela legível de 4096 bytes, conta quantas palavras dessa janela coincidem com os endereços capturados e, separadamente, quantos objetos **distintos** foram encontrados. O sexto objeto serve como controle de seleção.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_player_object_collection_discovery_v1.exe
```

Relatório: `aoe2hd_player_object_collection_candidates_v1.csv`. Indicador `with_2plus_distinct_anchors` corresponde a janelas com pelo menos duas âncoras diferentes, **não** a coleções confirmadas. A versão examina apenas janelas com ponteiros diretos nos primeiros 1.024 bytes de cada raiz, não segue múltiplos níveis de indireção nem cobre listas indexadas apenas por IDs. Ainda requer seleção para registrar as âncoras e não enumera aldeões automaticamente. Uma ausência de resultado não prova que o jogo não tenha uma coleção de objetos.

## PlayerObjectCollectionDiscovery v2 — percurso de até três níveis

Versão somente leitura do experimento multiâncora. Registra, mediante seleção inicial, Centro Urbano A, Quartel B, Centro Urbano D, Campo de Arqueiros E e Aldeão C. Depois selecione **um sexto objeto diferente** antes de iniciar as leituras. A busca parte das estruturas `World` e `LocalPlayer` e percorre ponteiros alinhados encontrados nos primeiros 1024 bytes de cada nó, com profundidade de **até 3 arestas de ponteiro**; possui orçamento de **512 nós**, **2 MiB de bytes lidos/tentados** e limite de candidatos enfileirados. Os endereços de objetos conhecidos são usados como gabarito de comparação; o percurso não os obtém da seleção no momento da busca. A saída registra o caminho de cada referência encontrada, número de âncoras distintas e se a busca foi truncada.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_player_object_collection_discovery_v2.exe
```

Relatórios: `aoe2hd_player_object_collection_paths_v2.csv` (profundidade, origem e referência à âncora) e `aoe2hd_player_object_collection_summary_v2.csv` (nós, leituras, orçamento, truncamento e cobertura). **O percurso é exploratório e pode conter caches de seleção, ponteiros transitórios ou caminhos redundantes**; não prova coleção de objetos, não é descoberta autônoma das âncoras e não enumera aldeões existentes sem seleção. Uma busca sem resultados, especialmente se truncada, não demonstra ausência de uma coleção.

## PlayerObjectCollectionDiscovery v3 — travessia equilibrada e telemetria

Esta versão mantém a leitura externa do processo em modo somente leitura e coleta cinco âncoras por seleção inicial: Centro Urbano A, Quartel B, Centro Urbano D, Campo de Arqueiros E e Aldeão C. Selecione um sexto objeto distinto para iniciar a investigação.

A busca examina até 3 arestas de ponteiro partindo de `World` e `LocalPlayer`, usando uma janela de **1.024 bytes** por nó. Cotas por raiz e por profundidade: **1, 48, 96 e 96** tentativas respectivamente para profundidades 0 a 3. Orçamento global: **2 MiB**; fila por raiz/profundidade: **512 candidatos**. Rejeições por endereço inválido, duplicação e falha de leitura são contabilizadas. As filas são percorridas por nível; os relatórios registram truncamentos e também nós sem correspondência com âncoras.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_player_object_collection_discovery_v3.exe
```

Relatórios: `aoe2hd_player_object_collection_paths_v3.csv` para correspondências com âncoras; `aoe2hd_player_object_collection_nodes_v3.csv` com cada nó processado e status; `aoe2hd_player_object_collection_coverage_v3.csv` com cobertura por raiz e profundidade. A fila pode conter muitos ponteiros que apenas parecem válidos; portanto estes resultados ainda não demonstram uma coleção de entidades nem a enumeração autônoma de aldeões. Mesmo `truncated=0` se refere apenas ao universo limitado examinado, não à memória total do jogo.

## PlayerObjectCollectionDiscovery v4 — Structural Candidate Ranking

Pesquisa **somente leitura** partindo de `World` e `LocalPlayer`; registra cinco âncoras (Centro Urbano A, Quartel B, Centro Urbano D, Campo de Arqueiros E, Aldeão C) por seleção e solicita um sexto objeto distinto como controle. A v4 usa `VirtualQueryEx` para verificar toda a extensão dos 1024 bytes de cada nó antes de `ReadProcessMemory`. Mantém até 3 arestas de ponteiro, cotas de tentativas de 1/48/96/96 por raiz e profundidade, fila de até 512 candidatos por compartimento e limite total de 2 MiB em leituras tentadas. Procura âncoras mesmo quando o destino do ponteiro não é legível.

Cada nó legível recebe métricas exploratórias de densidade de ponteiros legíveis (até 64 destinos distintos amostrados), quantidade de valores distintos e padrões plausíveis de início/fim de intervalo. A pontuação dá maior peso a referências conhecidas e ordena **candidatos dentro de cada ramificação**, com rodízio entre ramificações para reduzir monopolização. O rodízio não prova que os ramos sejam coleções. As referências possíveis são amostradas (até 32 valores por nó); a ferramenta pode omitir caminhos. Não valida automaticamente ponteiros individuais como entidades reais.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_player_object_collection_discovery_v4.exe
```

Saídas: `aoe2hd_player_object_collection_paths_v4.csv` (referências a âncoras), `aoe2hd_player_object_collection_nodes_v4.csv` (status, indicadores estruturais, pontuação e origem), `aoe2hd_player_object_collection_coverage_v4.csv` (cotas, falhas de leitura, rejeições prévias, duplicações e truncamento por nível). O experimento não enumera aldeões sem seleção, e ausência de correspondência não prova ausência de uma coleção.

## PlayerObjectCollectionDiscovery v5 — Collection-Aware Structural Discovery

Novo experimento de leitura externa **somente leitura**. Registre cinco âncoras por seleção: Centro Urbano A, Quartel B, Centro Urbano D, Campo de Arqueiros E e Aldeão C; use depois um sexto objeto diferente como controle. A ferramenta coleta duas travessias delimitadas de `World` e `LocalPlayer`, com troca de seleção entre elas. As cotas e o algoritmo de descoberta de nós são herdados da v4 (até 3 níveis, janelas de 1 KiB, orçamento de 2 MiB por travessia).

Para cada nó recuperado na segunda captura, calcula a sequência mais longa de valores consecutivos que apontam a regiões legíveis, o número de âncoras distintas encontradas, possíveis relações entre ponteiros `[begin,end,capacity]` e até 64 destinos com IDs numericamente plausíveis em `+0x08`. Esses indícios **não** certificam a classe nem a identidade dos objetos. A pontuação utiliza esses indicadores sem teto artificial fixo de 40 pontos. A coluna `persisted_node` indica que o mesmo endereço foi recuperado nas duas capturas; **não** prova que seus bytes ou ponteiros internos tenham permanecido iguais.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_player_object_collection_discovery_v5.exe
```

Relatórios: `aoe2hd_player_object_collection_candidates_v5.csv` (caminho, persistência do endereço e indicadores estruturais na segunda captura) e `aoe2hd_player_object_collection_coverage_v5.csv` (orçamentos, contagens e truncamento em cada travessia). O CSV de candidatos pode ser ordenado por `score` para análise. A v5 **não** enumera aldeões sem seleção, não verifica se um candidato é uma coleção real e não cobre tabelas representadas apenas por IDs.

## PlayerObjectCollectionDiscovery v6 — Targeted Candidate Validation

Ferramenta de leitura externa **somente leitura** que testa até 16 endereços candidatos da v5 (hexadecimais, separados por espaço), em vez de ampliar novamente a travessia genérica. Registra cinco âncoras via seleção (Centro Urbano A, Quartel B, Centro Urbano D, Campo de Arqueiros E e Aldeão C), exige um sexto objeto distinto e depois coleta duas leituras de 1024 bytes por endereço com mudança de seleção.

Use a lista padrão oferecida apenas **na mesma sessão de processo** do experimento anterior; caso tenha reiniciado o jogo, informe endereços reidentificados na sessão atual, pois os absolutos podem mudar. A validação registra correspondências exatas com âncoras, sequências de palavras interpretadas como ponteiros, campos `begin/end/capacity` plausíveis, identificadores numéricos em `objeto+0x08` e hash FNV-1a de toda a janela para comparar conteúdo entre capturas. A existência de um número plausível em `+0x08` **não confirma** um objeto de jogo, e `content_equal` indica igualdade do hash, não identidade sem possibilidade de colisão.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_player_object_collection_discovery_v6.exe
```

Relatório: `aoe2hd_player_object_collection_targeted_v6.csv` (legibilidade nas duas capturas, igualdade de hash, possíveis vetores, sequência de ponteiros, IDs plausíveis e correspondência exata com as cinco âncoras). A ferramenta investiga **endereços de candidatos fornecidos manualmente**, não descobre coleções por si só. A ausência de correspondências não exclui estruturas baseadas em IDs ou fora das janelas testadas.

## PlayerObjectCollectionDiscovery v7 — Controlled Entity Creation Differential Analysis

Experimentação externa **somente leitura** em três etapas A/controle/B. A ferramenta registra as cinco âncoras usuais por seleção inicial e exige um sexto objeto não registrado. Depois recebe de 1 a 16 endereços candidatos **válidos no processo atual** (hexadecimais separados por espaços, sem lista automática de endereços antigos). O mesmo conjunto de janelas de 1024 bytes é lido nas três etapas:

- **A:** anote a contagem de aldeões; não crie unidades e pressione Enter.
- **Controle:** deixe a simulação avançar sem criar ou destruir objetos; pressione Enter.
- **Intervenção:** crie exatamente um aldeão, espere sua conclusão, **não o selecione**, e pressione Enter.

O CSV `aoe2hd_player_object_collection_differential_v7.csv` registra offset, valores antes/controle/depois, estabilidade no controle, incremento exato de um, novos valores com formato de ponteiro e validação pontual de leitura do ID em `+0x08`. O CSV `aoe2hd_player_object_collection_summary_v7.csv` apresenta contagens por janela. As diferenças são *correlações*: campos de simulação podem variar por razões diversas; repita com um segundo aldeão e outra sessão para confirmar. O experimento só investiga as janelas fornecidas, não enumera diretamente aldeões e não cobre automaticamente estruturas de ID/índice externas a elas.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_player_object_collection_discovery_v7.exe
```

## PlayerObjectCollectionDiscovery v8 — Targeted Pointer Transition Validation

A v8 valida **somente leitura** os campos `base+0x178` e `base+0x28C` de um endereço-base informado manualmente, válido na **sessão atual do processo**. A hipótese inicial veio de `0x14D6B188` na sessão da v7, mas **não reutilize esse valor caso tenha reiniciado o jogo**. A resolução automática desse endereço entre processos ainda não foi implementada. As cinco âncoras conhecidas são coletadas inicialmente; depois, um sexto objeto distinto é selecionado para o controle de entrada.

O programa faz três capturas após comandos Enter separados: A (sem criar unidades), CONTROLE (simulação avançando sem criar nem destruir unidades) e INTERVENÇÃO (crie um aldeão, aguarde terminar e **não o selecione**). Para cada campo, registra se o ponteiro permaneceu igual no controle, mudou após intervenção e se o novo destino permitiu uma leitura de 12 bytes, inclusive a palavra em `destino+0x08`. A classificação `candidate=1` exige mudança somente na intervenção e legibilidade do destino; isso **não comprova** que a estrutura guarda aldeões, apenas identifica uma transição a ser investigada.

```powershell
git pull --ff-only origin main
cmake -S . -B build-clean -G "Visual Studio 18 2026" -A Win32
cmake --build build-clean --config Release
ctest --test-dir build-clean -C Release --output-on-failure
.\build-clean\Release\aoe2hd_player_object_collection_discovery_v8.exe
```

Saída: `aoe2hd_player_object_collection_transitions_v8.csv`, com os três valores, IDs numéricos e indicadores para cada deslocamento. A memória pode ser alterada assincronamente pelo jogo; portanto o experimento deve ser repetido antes de concluir causalidade. Endereços antigos podem estar ilegíveis ou ter sido realocados.
