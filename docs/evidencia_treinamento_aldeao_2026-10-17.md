# Evidência de progresso de treinamento de aldeão — 2026-10-17

Fonte: Cheat Engine, tabela XML e resultados de "Find out what writes/accesses this address" fornecidos pelo usuário. Alvo informado: AoK HD.exe (versão 5.8.INT no projeto; confirmar a versão da sessão).

## Endereço observado e tipo

- Endereço absoluto **da sessão**: `0x18769C5C`.
- Cheat Engine: `VariableType=4 Bytes`, `LastState Value=17`; isso **não comprova** armazenamento uint32 nem escala 0–100.
- As instruções observadas leem/escrevem **16 bits** (`ax`/`cx`), indicando um campo `uint16` candidato no endereço, possivelmente com outro campo de 16 bits adjacente.
- `EDI=0x18769C5A` e `ESI/EBX=0x18769C5C` nas operações de zeragem; candidato vizinho em `0x18769C5A`, sem semântica confirmada.

## Evidência estrutural importante

Na instrução `AoK HD.exe+0x318E3A`:

```asm
cmp ax, [ebx+0x00000C44]
EBX=0x18769018
```

A soma `0x18769018 + 0x0C44 = 0x18769C5C` corresponde **exatamente** ao endereço investigado. Portanto **CandidateTrainingBase + 0x0C44** é um offset estrutural candidato `uint16`, sujeito a confirmar que `0x18769018` é de fato a base da estrutura de treinamento e que se mantém estável entre sessões.

## Trechos de código observados (RVAs no executável)

- `+0x4CB0B3`: `mov [esi], ax` depois de `xor eax,eax`; ESI=`0x18769C5C` (zera campo de 16 bits). Na sequência `mov ecx,[ecx+0x150]`, com ECX=`0x18866938` nessa captura.
- `+0x4BE396`: `mov [ebx], ax` depois de `xor eax,eax`; EBX=`0x18769C5C` (zera o mesmo campo). Na sequência `mov ecx,[esi+0x220]`, ESI=`0x18866938`.
- `+0x0BF111`: `mov [eax],cx`; EAX=`0x18769C5C`, CX=`0x0011` (decimal 17), escreve 16 bits.
- `+0x318E3A`: `cmp ax,[ebx+0xC44]`; EBX=`0x18769018`. Compara 16 bits nesse endereço, **não** escreve nele.

Dois blocos de capturas ocorreram entre aproximadamente 18:21:56–18:23:17 no relógio apresentado pelo usuário; registros "First seen/Last seen" não indicam que a fila começou ou terminou nesses instantes.

## Relação com fila e população

O histórico já contém `Building+0x214/+0x218/+0x21C` para fila de produção, porém **não há evidência de que `0x18769018` seja o objeto `Building`**. `CandidateTrainingBase+0x0C44` e `Building+0x214` não devem ser unidos em uma mesma estrutura sem verificação.

Hipótese: campo guarda progresso ou contador relacionado ao treinamento. Alternativas: temporizador, etapa de animação, ID ou estado da interface. Testar leituras `uint16` de `+0x0C44` e `+0x0C42` antes/durante/após treinamento, cancelamento, seleção e reinício. Comparar com o percentual do HUD; uma observação igual a 17 não estabelece relação percentual nem unidade de tempo.

**Status:** offset `+0x0C44` como `hipotese`; endereços absolutos como `sessao_observada`. Código de instrução representa RVAs de funções, não endereços de dados de treinamento.

## Monitor uint16 de dois ciclos — CSV com 486 amostras

Fonte: `training_timeline.csv` recebido do usuário (fase `ciclo_aldeao`, base absoluta de sessão `0x18769018`). Amostragem `0.1 s`; **486/486 leituras ok, zero erros, 54 mudanças exibidas**. Tempo total registrado de `0.0009` a `48.500529` s.

| Transição | Amostra | Tempo relativo (s) | `+0xC44` | `+0xC42` |
|---|---:|---:|---:|---:|
| Inicial/inativo | 1 | 0.0009 | 0 | 65535 (0xFFFF) |
| Ciclo 1 em andamento | 93 | 9.200524 | 1 | 83 (0x0053) |
| Último valor do ciclo 1 | 179 | 17.800662 | 53 | 83 |
| Retorno ao inativo | 185 | 18.400351 | 0 | 65535 |
| Ciclo 2 em andamento | 231 | 23.000598 | 2 | 83 |
| Último valor do ciclo 2 | 394 | 39.300529 | 99 | 83 |
| Retorno ao inativo | 399 | 39.800324 | 0 | 65535 |

O contador `+0xC44` cresce quase linearmente em passos próximos a três a cada 0,5 s, comportamento **fortemente compatível** com progresso percentual 0–100. O campo adjacente `+0xC42` comuta conjuntamente entre 0xFFFF (sentinela de inatividade candidata) e 0x0053 (identificador/estado de unidade candidato). A queda intermediária 53→0 é compatível com cancelamento, e 99→0 é compatível com conclusão, **sem marcas de ação no log**. Nenhum dos dois comportamentos confirma sozinho a semântica exata. A base `0x18769018` continua absoluta da sessão, sem cadeia estável verificada. Recomenda-se confirmar a porcentagem no HUD e comparar uma segunda classe de unidade para validar o papel de 0x0053.

## Investigação da semântica de 0x0053 (83) — referência de unidades

Foi pesquisado o catálogo atual e o código histórico do próprio repositório: `TrainingCandidateBase+0xC42` continua registrado apenas como hipótese, e os leitores históricos de fila (`ProductionQueueEntriesV6.h`, `ProductionQueueEnumerationV7.h`) decodificam `uint16 type` e `uint16 quantity`, **sem tabela local que converta o número 83 em nome de unidade**.

Referências externas independentes identificam `83 (0x0053)` como **Villager (Male)** (aldeão masculino):

- https://aoe2control.github.io/enums/ (`UnitObjectType.VILLAGER_MALE = 83`);
- https://airef.github.io/tables/objects.html (`83 Villager Male`, `293 Villager Female`, `74 Militia`);
- https://ksneijders.github.io/AoE2ScenarioParser/api/AoE2ScenarioParser/datasets/units.html (`VILLAGER_MALE=83`, `VILLAGER_FEMALE=293`).

**Inferência atual**: o valor `+0xC42 = 0x0053` em ambos os treinamentos de aldeão é compatível com **ID do tipo de unidade em treinamento**, e `0xFFFF` fora de treinamento é compatível com sentinela de fila vazia/nenhuma unidade ativa. A evidência reforça a hipótese, mas **não a valida diretamente na versão HD 5.8.INT**: tabelas externas podem descrever interfaces/versões diferentes e coincidência com um único tipo não exclui estados alternativos. Confirmar com milícia (ID de referência 74 / 0x004A) e, se possível, outras unidades; monitorar alternância +0xC42 de 0xFFFF ao ID durante a fila, retorno em cancelamento/conclusão e comportamento após reinício. Não alterar o status `hipotese` sem validação.

## Replicação com milícia — evidência independente do ID de unidade

Fonte: `training_timeline(1).csv` enviado pelo usuário, contendo **721 linhas de amostras** acumuladas por append: 486 da sessão anterior de aldeão e 235 da sessão de milícia. A segunda sessão reinicia `sequence=1` e `elapsed_seconds` do zero. Portanto **não combinar os tempos relativos nem inferir continuidade de uma única execução**. São 721 leituras `ok`, nenhum erro. O usuário informa que **treinou uma milícia**.

Na nova sessão de milícia (235 amostras):

| Evento | Amostra | Tempo relativo (s) | `+0xC42` uint16 | `+0xC44` uint16 |
|---|---:|---:|---:|---:|
| Antes | 1 | ~0.0 | 65535 (0xFFFF) | 0 |
| Início observado de progresso | 49 | ~4.8 | **74 (0x004A)** | 2 |
| Último valor crescente | 181 | ~18.0 | **74 (0x004A)** | 96 |
| Retorno à inatividade | 186 | ~18.5 | 65535 (0xFFFF) | 0 |

Há **137 amostras** com `+0xC42=74`, 98 com `+0xC42=65535` na sessão da milícia. A sessão anterior com aldeão possui 260 amostras `+0xC42=83` e 226 com `+0xC42=65535`; total acumulado: 137 (ID 74), 260 (ID 83), 324 (sentinela 65535). Coincidência específica entre o tipo de unidade treinado e o valor observado: aldeão masculino **83/0x0053**, milícia **74/0x004A** (ver links externos e ressalvas acima).

**Conclusão delimitada:** `TrainingCandidateBase+0xC42` é agora candidato fortemente corroborado de **ID do tipo de unidade na produção ativa**; `+0xC44` é candidato fortemente corroborado de progresso percentual (ciclo crescente até 96, depois zero). O retorno simultâneo a 0xFFFF/0 é compatível com fila/produção inativa. Ainda não há confirmação da cadeia de ponteiros, reidentificação após reiniciar, equivalência exata ao indicador de HUD em múltiplos pontos, nem controles para unidades diferentes e várias entradas na fila. **Manter status de catálogo `hipotese`**, embora com evidência observacional forte.

## Instrumentação Training Monitor v2 (2026-10-17)

O repositório `AoE2HD.control` implementou a leitura contínua conjunta `TrainingCandidateBase+0xC42` e `+0xC44` como `uint16` e a classificação **experimental** dos valores conhecidos: 83 = aldeão masculino, 74 = milícia, 65535 = sentinela de produção inativa candidata. Nova saída CSV contém `unit_name`, `production_state`, `cycle_id`, `event`, `event_confidence` e `cycle_peak`. A passagem sentinela→ID é classificada como início observado; ID→sentinela é rotulada heurística de conclusão provável com pico >=95, cancelamento provável com pico <90, indeterminado entre 90–94. Esses limiares são **critérios instrumentais provisórios** e não validam causas de gameplay. Não houve descoberta de novos offsets nem mudança do status `hipotese`; ainda não existe cadeia estável à base da estrutura.

## Nova partida: associação dinâmica com objeto selecionado

Cheat Engine reportado pelo usuário: progresso absoluto `0x1838FEDC` com valor 16; instrução `AoK HD.exe+0x318E3A` lê `[EBX+0xC44]`, com `EBX=0x1838F298`. A soma é exata: `0x1838F298+0xC44=0x1838FEDC`. Comparado à sessão anterior (`0x18769018+0xC44=0x18769C5C`), o offset interno permanece constante e a base absoluta varia; não transportar endereços absolutos entre partidas.

Em `AoK HD.exe+0x4CB0B3`, após `mov [esi],ax` em `ESI=0x1838FEDC`, a próxima instrução é `mov ecx,[ecx+0x150]`, sendo `ECX=0x186D0908`. Em `AoK HD.exe+0x4BE396`, após `mov [ebx],ax` com `EBX=0x1838FEDC`, a próxima instrução é `mov ecx,[esi+0x220]`, sendo `ESI=0x186D0908`. O endereço `0x186D0908` coincide com o objeto selecionado ID 2180 obtido pelo Selected Building Training Discovery em captura anterior; é importante verificar se o processo/partida permaneceu a mesma sessão para correlacionar identidades. Em `AoK HD.exe+0x0BF111`, `mov [eax],cx`, `EAX=0x1838FEDC`, `CX=16`, `EDX=0x53`. **EDX=83** é compatível com aldeão masculino, mas o registrador não demonstra sozinho a origem desse ID.

**Novos offsets candidatos de investigação:** `SelectedBuilding+0x150` e `SelectedBuilding+0x220` são acessados por leitura nas instruções seguintes à escrita do campo; ainda não se provou que contenham ponteiros, sua semântica ou ligação com `TrainingCandidateBase`. São pontos de observação para captura dos valores efetivamente lidos antes, durante e depois do treinamento. A relação estável descoberta entre partidas é exclusivamente `TrainingCandidateBase+0xC44`, não o endereço absoluto nem a cadeia seleção→base.

## Alternância entre Centro Urbano e Quartel — relato do operador

O usuário informou que o endereço investigado **permanece o mesmo ao alternar entre Centro Urbano e Quartel**. No contexto da captura anterior, o candidato principal é `TrainingCandidateBase=0x1838F298` (campo de progresso `0x1838FEDC`), mas a mensagem não nomeia explicitamente qual dos endereços permaneceu constante. Portanto, a identidade do endereço deve ser confirmada no próximo log; não converter a observação em validação de estrutura específica.

Hipóteses concorrentes: estrutura compartilhada de interface/seleção, estrutura global de treinamento, ou base ligada a objeto que permanece inalterado embora a seleção mude. Teste diferencial: com CU treinando aldeão e Quartel treinando milícia simultaneamente, registrar o valor de `+0xC42/+0xC44` antes/durante/depois de alternar seleção, sem parar a produção. Comparar `selected object ID`, `selected pointer` e `TrainingCandidateBase` em uma mesma captura. Se C42 alternar entre 83 e 74 junto com a seleção, favorecer interface/seleção; se persistir independente da seleção, investigar ligação global/fixa. Repetir após reiniciar o processo. Status permanece `hipotese`.

## Nova partida — endereço observado 0x13DEFE54

O usuário compartilhou uma entrada Cheat Engine XML, `VariableType=4 Bytes`, `RealAddress=0x13DEFE54`, `LastState Value=0`, após iniciar uma nova partida. **Não foram fornecidos registradores ou instruções de acesso nesta captura**. Se corresponder ao mesmo campo histórico `TrainingCandidateBase+0xC44`, a base aritmética seria `0x13DEF210` (`0x13DEF210 + 0xC44 = 0x13DEFE54`), mas esta base é apenas **derivada condicional**, não verificada. Valor zero não discrimina entre progresso inativo e qualquer outra variável; tampouco o tipo `4 Bytes` do CE comprova armazenamento uint32, dado que capturas anteriores mostraram instruções de 16 bits. Próximo controle: iniciar treinamento, observar transição crescente e recuperar `cmp ax,[ebx+0xC44]` para confirmar EBX na nova sessão; comparar também `+0xC42` com os IDs 83 (aldeão) e 74 (milícia). Não reutilizar a base absoluta de partidas anteriores.

## Nova partida: alternância de tipos em uma única TrainingCandidateBase

Origem: `training_nova_partida.csv` enviado pelo usuário, captura de 262 amostras com `--training-base 0x13DEF210` e intervalo nominal 0,1 segundo. Total: 262 leituras `ok`, 0 erros; `+0xC42` foi 65535 em 189 amostras, 83 em 36 e 74 em 37. A mudança crítica ocorreu entre amostras 136 (t=13,500597 s, ID 74, progresso 9) e 137 (t=13,600723 s, ID 83, progresso 35), **sem valor 65535 intermediário nas amostras capturadas**. Outras transições: #83 t=8,200139 s para ID 83/C44=2; #96 t=9,500253 s para inativo; #114 t=11,300709 s para ID 74/C44=2; #160 t=15,900645 s para inativo; #176 t=17,500367 s para ID 74/C44=47; #190 t=18,900645 s para inativo.

Hipótese reforçada: a mesma base mostra diferentes unidades e seus progressos, possivelmente um espelho do **objeto de produção selecionado** na UI. Alternativas: contexto global/última estrutura consultada; transições entre amostras podem ter sido perdidas. O arquivo não contém série temporal da construção selecionada, portanto não prova causalidade seleção→mudança da estrutura. Importante: o heurístico do Training Monitor v2 `cancelamento_provavel` pode gerar **falsos positivos** ao trocar seleção; transição ativo→65535 é observação de desaparecimento do campo, não demonstra cancelamento. Próximo teste: amostrar no mesmo instante os dois slots de seleção `LocalPlayer+0x15C/+0x160`, ID do objeto selecionado e `C42/C44`, alternando entre Centro Urbano e Quartel com produções simultâneas; registrar eventos como `fim_observado_causa_indeterminada` quando não houver confirmação independente.

## Instrumentação Training Monitor v3

Foi integrado ao AoE2HD.control o comando `--monitor-training-v3`, que lê antes/depois os campos de seleção experimentais `LocalPlayer+0x15C/+0x160`, o ponteiro e ID do objeto selecionado e, entre essas duas leituras, `TrainingCandidateBase+0xC42/+0xC44`. Se a seleção muda durante a amostra, a captura é descartada com erro explícito. O CSV registra cada amostra e separa `selecao_mudou`, `selecao_e_treinamento_mudaram`, `treinamento_deixou_de_aparecer`, `tipo_exibido_mudou` e `progresso_mudou`. Nenhum desses eventos implica cancelamento ou conclusão sem evidência independente. Mantidos todos os offsets como hipóteses e não descoberta uma cadeia estável para TrainingCandidateBase. Ainda pendente executar os testes no Windows e capturar uma nova partida.

## Pointer Scanner — cadeias relativas ao módulo (capturas de 2026-10-17)

Quatro imagens fornecidas pelo usuário mostram rotas do Cheat Engine até o campo absoluto de sessão `0x18769C5C` (valor 23). Rotas candidatas prioritárias, transcritas exatamente na ordem das colunas Offset 0, Offset 1 etc. do scanner (hexadecimal):

- `AoK HD.exe+0x006F4170`: `F0, C44` (curta).
- `AoK HD.exe+0x006DA4B4`: `C, 8A4, C44` (curta, outra raiz).
- `AoK HD.exe+0x006F4170`: `F0, F0, F4, C44`.
- `AoK HD.exe+0x006EEF98`: `774, 4, 2C, F0, F0, F0, C44`.

Há dezenas de rotas alternativas, inclusive de `AoK HD.exe+0x005B....`, `+0x006DA4B4` e `THREADSTACK`. Múltiplos caminhos no mesmo processo não fornecem provas independentes nem estabilidade após reinício. A ordem apresentada pelo scanner deve ser interpretada segundo a semântica real de desreferenciação do Cheat Engine, sem tratá-la automaticamente como uma sequência de offsets diretos. Cadeias globais podem localizar uma estrutura compartilhada de interface/produção, não necessariamente um Building. Os RVA base são novos candidatos, não endereços verificados.

Validação necessária: repetir pointer scan/rescan depois de reiniciar o executável, confirmar novo endereço alvo via progresso, resolver as duas rotas curtas com leitura de memória, comparar `C42` e `C44`, testar produção de aldeão/milícia e alternar seleção. Não promover os registros antes de controles positivos/negativos.

## Nova captura do Pointer Scanner: mesmo caminho, novo alvo

Duas capturas de tela posteriores apontam para `0x14E8C11C` (valor exibido 21), enquanto as anteriores apontavam para `0x18769C5C` (valor 23). A base de treinamento **derivada condicionalmente** é `0x14E8B4D8`, pois `0x14E8B4D8+0xC44=0x14E8C11C`. Os caminhos abaixo reaparecem nos dois conjuntos:

- `AoK HD.exe+0x006F4170` — offsets Cheat Engine: `F0, C44`.
- `AoK HD.exe+0x006DA4B4` — offsets Cheat Engine: `C, 8A4, C44`.
- Também persistem caminhos maiores pela mesma raiz e por `+0x006F3D90`, `+0x006EEF98` etc.

**Inferência:** repetição forte de candidatos com raízes RVA de módulo e alvo absoluto diferente. As imagens isoladamente não atestam se houve encerramento/reinicialização completa de `AoK HD.exe` nem se os ponteiros foram reavaliados em memória ao vivo depois de rescan; confirmar isso com operador. Preservar ordem de offsets como exibida pelo CE (sua expressão aninhada usual é avaliada a partir do último offset para o primeiro); não assumir travessia em ordem esquerda→direita. Próxima verificação: avaliar as duas cadeias curtas via leitura externa no novo processo, comparar endereço resolvido ao alvo CE, `uint16 C42` em `0x14E8C11A` e `uint16 C44` em `0x14E8C11C` enquanto unidades 83 e 74 são treinadas, e repetir após novo reinício. Não promover a historico_validado ainda.

### Confirmação do operador: reinício completo do processo

O usuário confirmou explicitamente que as duas novas capturas com destino `0x14E8C11C` foram realizadas **após fechar completamente e reabrir `AoK HD.exe`**. Isso eleva a evidência das rotas de RVA `+0x006F4170` (`F0,C44`) e `+0x006DA4B4` (`C,8A4,C44`) de mera repetição entre partidas para **reprodução observada entre processos distintos**: a captura anterior tinha destino `0x18769C5C`, e a posterior `0x14E8C11C`. Ainda não houve resolução independente das cadeias pelo `AoE2HD.control` nem conferência conjunta do ID em `C42` e progresso em `C44` no novo processo. Portanto, conservar `hipotese` até leitor externo confirmar, em várias capturas, o destino, valores e controles negativos.

## Training Pointer Resolver v1 — implementação no controlador

O `AoE2HD.control` agora dispõe de `--resolve-training` e `--monitor-training-v3 --auto-training-base`. As duas rotas de RVA da captura CE são percorridas com leituras ptr32, offsets `F0,C44` e `C,8A4,C44`. A aceitação requer igualdade do endereço final, segunda resolução dos caminhos e estabilidade dos campos `uint16` (C42 e C44), valores conhecidos 83/74/65535 e progresso bruto na faixa 0–100 (sentinela acompanhada de zero). Resultados `consensus_candidate` permanecem experimentais; erros/divergências interrompem a ativação automática. O monitor reavalia ambas as cadeias a cada amostra para detectar invalidação. Os testes com memória simulada foram adicionados; execução real após reinício ainda pendente. **Não promover offsets antes do teste com executável.**

## Treinamento ativo e seleção correlacionada — Training Monitor v3

Fonte: `training_auto_v3.csv` enviado pelo usuário. O arquivo foi anexado em modo append e contém **quatro execuções** separadas por reinício de `sequence=1`: 212, 465, 446 e 248 amostras (total 1371); todas `status=ok`, zero erros. A execução final corresponde à saída de terminal fornecida, com resolução automática `consensus_candidate`: destino de ambas as rotas `0x14E8C11C`, base `0x14E8B4D8`, estado inicial `C42=65535`, `C44=0`.

Na última execução: o objeto selecionado `0x10509D58` (ID de objeto 1996) coincidiu com `C42=83` em 62 amostras, predominando aldeão; o objeto `0x102DF200` (ID 2305) coincidiu com `C42=74` em 46 amostras, predominando milícia. Valores cruzados temporários: na amostra 94 (objeto 1996, `C42=74`, progresso 6), o ID mudou para 83/progresso 23 na amostra 95; padrão similar nas amostras 113 (objeto 1996, ID74/progresso17) e 114 (ID83/progresso34). Trocas de seleção e atualização do campo não são estritamente síncronas a 0,1s; uma captura estável antes/depois da seleção não prova que o espelho da UI foi atualizado. Duas quedas para sentinela durante seleção estável: amostra 175, objeto 1996; amostra 195, objeto 2305; a causa (cancelamento/conclusão/seleção lógica da UI) não pode ser determinada apenas pelo CSV.

**Evidência atual**: o resolvedor conseguiu acompanhar treino ativo e convergência das rotas durante uma execução real sem erro. Os dois objetos têm associação forte a seus respectivos tipos de produção, consistente com CU/Quartel, mas os tipos de edifício não foram diretamente verificados pela captura. Hipótese principal: `TrainingCandidateBase` é contexto/espelho compartilhado relacionado à seleção e produção ativa, não estrutura per-Building. Não promover offsets por esta evidência sozinha. Próxima instrumentação: incorporar janela de estabilização da seleção, registrar defasagem seleção→C42 e usar estados transitórios explícitos em vez de inferir causa de encerramento.
