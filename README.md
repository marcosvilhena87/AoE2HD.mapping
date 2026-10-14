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
