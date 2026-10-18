# Evidência — contador de aldeões (17/10/2026)

Origem: tabela XML do Cheat Engine enviada pelo usuário e identificada como **Contador de aldeões**. Três entradas, todas configuradas como **4 Bytes**, exibem simultaneamente valor decimal **7**:

| Endereço absoluto | Valor | Tipo CE | Status |
|---|---:|---|---|
| `0x143D8F80` | 7 | 4 Bytes | candidato de sessão |
| `0x187B0214` | 7 | 4 Bytes | candidato de sessão |
| `0x187B0DCC` | 7 | 4 Bytes | candidato de sessão |

Distâncias: B−A = `0x43D7294`; C−B = `0xBB8`. **Não há evidência de raiz estável ou relação direta com a ResourceTable**. Não deduzir offsets estruturais subtraindo bases não comprovadas; endereços absolutos tornam-se inválidos após reinício.

## Controles para validação

1. Medir os três valores simultaneamente enquanto a quantidade de aldeões vivos permanece 7; registrar contador exibido pelo jogo e se há unidades em produção.
2. Iniciar treinamento de aldeão sem concluí-lo: contador de **vivos** deve permanecer 7; campos que antecipem 8 podem representar fila/estatísticas.
3. Concluir treinamento e confirmar unidade surgida: esperar 8; repetir com segundo aldeão.
4. Produzir milícia/lanceiro: não deve aumentar o contador de aldeões.
5. Perder um aldeão: esperar redução de 8 para 7, distinguindo contador vivo de total produzido historicamente.
6. Desselecionar edifícios e reiniciar processo; buscar ponteiros ou RVA novamente para validar persistência.

A captura única de valor 7 **não prova** a semântica de qualquer campo. Até haver transições positivas e negativas em controles, status `sessao_observada`/candidato; evitar classificá-lo como validado. A leitura é exclusivamente observacional.

## Ensaios dinâmicos: fila, conclusão, cancelamento e morte

O usuário realizou testes de comportamento nos três candidatos que inicialmente exibiam 7:

| Endereço de sessão | Entrada na fila | Conclusão do treinamento | Cancelamento | Morte de aldeão | Interpretação experimental |
|---|---|---|---|---|---|
| `0x187B0214` | +1 imediatamente (valor 8 observado) | os demais alcançam a contagem | retorna ao valor anterior | diminui, segundo relato "os outros sim" | aldeões vivos **mais** aldeões enfileirados, ou outro contador inclusivo de fila |
| `0x143D8F80` | não foi relatado aumento antecipado | +1 | não foi relatado | **não diminui** | total cumulativo de aldeões concluídos/criados, hipótese; verificar reinício e perdas |
| `0x187B0DCC` | não foi relatado aumento antecipado | +1 | não foi relatado | **diminui** | aldeões vivos, hipótese principal |

**Nota sobre o primeiro campo:** o relato de diminuição na morte é indireto ("os outros sim"), portanto confirmar com pares de leituras antes/depois. **Nota sobre o segundo:** não diminuir ao morrer é compatível com contagem histórica de aldeões produzidos, mas também com valor em cache ainda não atualizado; não inferir sem teste temporal. Não assumir que o primeiro campo equivale exatamente a vivo + todas as unidades em filas simultâneas antes de testar duas filas, duas construções e cancelamento parcial.

Modelo candidato para testar: `fila_inclusiva = vivos + aldeoes_em_fila`, `total_cumulativo = aldeoes_criados`, `vivos = aldeoes_atuais`. Se as três leituras continuarem consistentes ao treinar duas unidades, cancelar uma, concluir uma e perder outra, a identificação poderá ser elevada com novas sessões e cadeias estáveis.

Endereços absolutos da sessão não constituem offsets estáveis. Preservar `uint32` como tipo configurado no Cheat Engine; validar com instruções de acesso posteriormente.

## Correção experimental — fila não é contada integralmente

O usuário esclareceu que `0x187B0214` **aumenta somente em 1** quando há aldeões na fila, mesmo que sejam enfileirados dois ou mais. Portanto, a hipótese anterior `aldeões vivos + quantidade total de aldeões enfileirados` está **refutada** pelos testes. Nova hipótese: `aldeões vivos + indicador binário de presença de treinamento/fila de aldeões` (0 se não há, 1 se existe ao menos um), **ou** reserva única de população relacionada à produção, ainda sem determinação de semântica exata. O campo cai ao valor anterior após cancelar a fila, segundo observação anterior. Falta testar múltiplas construções produzindo simultaneamente, cancelamento parcial mantendo outra unidade na fila, conclusão de uma unidade com outra ainda na fila, mortes durante a fila, e verificar se o deslocamento relativo aos aldeões vivos é sempre 0 ou 1. Não nomear como contagem total de fila; preservar status experimental.

## Instrumentação Villager Counter Resolver v1

`AoE2HD.control` passou a fornecer `--monitor-villagers` (captura read-only de três endereços absolutos informados na sessão, uint32, CSV de amostras e diferenças) e `--resolve-villagers` (varredura conservadora de possíveis referências de um salto nas janelas até 0x400 bytes de World, WorldPlayer local e ResourceTable, com limite configurável até 0x2000). O resolvedor **não tem ainda ponteiros estáveis para os três contadores** e não inventa resolução automática entre processos. Exige três endereços explícitos atuais; nenhum absoluto antigo vira um deslocamento validado apenas por coincidência. Todas as referências eventualmente encontradas continuam candidatas, exigindo teste após reinício, fila e mortes. O código possui testes de memória simulada, mas a execução Windows ainda está pendente.

## Descoberta revisada: partida iniciada com 3 aldeões, sem endereços de sessão antigos

O controlador implementa `--discover-villagers-3`, que coleta candidatos `uint32=3` nos primeiros bytes de `World`, `WorldPlayer` e `ResourceTable` após resolver essas raízes por cadeias previamente documentadas. Não usa qualquer endereço `ABSOLUTO_SESSAO` de partidas anteriores. Depois acompanha somente os candidatos iniciais nas fases `baseline_3`, `queue_1`, `queue_3`, `cancel_partial`, `cancel_all`, `complete_1`, `lose_1` e registra valores, deltas e compatibilidade dos padrões de fila binária, total cumulativo e unidades vivas. Janela limitada, padrão 0x400 bytes por raiz, máximo 0x2000: ausência de candidatos não descarta contadores em outras regiões. Essa descoberta é um procedimento experimental implementado, com testes sintéticos adicionados; **não houve coleta na partida real de 3 aldeões ainda**. O catálogo de endereços permanece inalterado; não promover hipóteses até dados reais e confirmação após reinício.

## Discovery v2 — varredura diferencial sem referencia a enderecos antigos

O controlador passou a disponibilizar `--discover-villagers-v2`: busca valores uint16/uint32/float32 em World, WorldPlayer local, ResourceTable e numero limitado de ponteiros filhos de cada raiz. Monitora os mesmos enderecos ao longo das sete etapas do protocolo com 3 aldeoes e grava somente campos que variaram. Classifica candidatos pelo **padrao relativo das transicoes**, sem exigir que o valor bruto inicial seja 3. Janela padrao configuravel, leitura somente, nenhum offset automaticamente validado. A versao anterior rastreou quatro candidatos iniciais mas nenhum apresentou a assinatura esperada; o resultado negativo nao nega a existencia dos contadores fora da janela. A v2 foi publicada com testes simulados, ainda nao foi executada no Windows contra a partida.

## Resultado real Villager Discovery v2 — 61 regioes, 2349 candidatos variaveis

Arquivo `villager_discovery_v2.csv` fornecido pelo usuario, sete fases. Distribuicao de classificacoes: 2 `living_compatible`, 17 `cumulative_compatible`, 2069 `unclassified`, 261 `insufficient_stages`, zero `queue_indicator_compatible`. Valores nos sete estagios (`baseline_3,queue_1,queue_3,cancel_partial,cancel_all,complete_1,lose_1`): `ResourceTable+0x94` float32 = **3,3,3,3,3,4,3** (melhor candidato aldeoes vivos); `ResourceTable+0x2C` float32 = **4,4,4,4,4,5,4** (mesma variacao mas deslocamento +1; previamente hipotese de populacao utilizada); `ResourceTable+0x4C` float32 = **4,4,4,4,4,5,5** (cumulativo compativel, possivelmente outro total de populacao); `WorldPlayer+0x522` uint16 = **0,0,0,0,0,1,1** (evento cumulativo compativel). Os outros 15 cumulativos podem ser aliases/campos de estado nao relacionados; nao promover em bloco.

Controles necessarios: construir unidade militar sem aldeao, registrar `ResourceTable+0x94/+0x2C/+0x4C`, matar unidade militar, treinar e matar aldeao, testar mais de uma fila e comparar HUD antes/depois. Particularidade: valores 4 em +0x2C/+0x4C mesmo no estado nominal de tres aldeoes podem indicar unidade nao aldeao ou outra definicao de populacao; nao chamar esses dois campos de contador exclusivo de aldeoes sem controle negativo militar. A v2 classifica **padroes relativos**, portanto um campo pode passar com qualquer valor-base. Todos os enderecos dinamicos oriundos dessas estruturas sao offsets candidatos, nao confirmados apos reinicio.

## Resultado Discovery v2 — sete etapas com partida inicial de tres aldeoes

O usuario executou `--discover-villagers-v2 --villager-window 0x800`. Foram capturados 61 blocos de memoria e gerados 2349 candidatos variaveis no CSV, dos quais 17 tem padrao cumulativo completo e 2 tem padrao de contador vivo completo. Nenhum registro foi classificado como indicador binario de fila. Todos os enderecos absolutos abaixo sao especificos da sessao.

A ResourceTable do jogador estava em `0x10164AF0`. Campos float32:

- `ResourceTable+0x094` / `0x10164B84`: **3,3,3,3,3,4,3**; candidato prioritario a quantidade atual de aldeoes vivos, com suporte a subida na conclusao e descida na morte.
- `ResourceTable+0x02C` / `0x10164B1C`: **4,4,4,4,4,5,4**; campo ja catalogado como populacao usada A, nao contar como prova de contador de aldeoes: inicial 4 apesar de haver 3 aldeoes.
- `ResourceTable+0x04C` / `0x10164B3C`: **4,4,4,4,4,5,5**; campo previamente candidato a populacao usada B, mas o fato de nao diminuir com morte indica que deve ser reavaliado: possivel total acumulado/estatistica, ainda sem semantica validada.

Fases na ordem baseline_3, queue_1, queue_3, cancel_partial, cancel_all, complete_1, lose_1. Os demais candidatos `cumulative_compatible` incluem constantes-base distintas e varios caminhos diferentes para o **mesmo endereco** `0x18B93DC0` (por exemplo `local_player+0x18->ptr+0x08` e `local_player+0x20->ptr+0x148`), portanto nao sao descobertas independentes. A classificacao atual permite deslocamento aditivo do valor inicial: correspondencia completa de transicoes nao garante correspondencia da magnitude com numero de aldeoes.

Necessario testar acoes de unidades militares e mortes de aldeoes em nova sessao para verificar especificidade de `+0x94`. O resultado do script nao valida automaticamente que cada estado do jogo foi cumprido. Nao promover candidatos a validados nem reaproveitar enderecos absolutos.

## Instrumentacao de controle militar e aldeao para ResourceTable+0x94

Foi implementado no AoE2HD.control o comando `--verify-villager-094`, que resolve ResourceTable a partir do jogador local e captura somente leitura dos campos float32 `+0x94`, `+0x2C`, `+0x4C` em cinco fases: baseline; milicia concluida; milicia perdida; aldeao concluido; aldeao perdido. Testa o delta relativo da hipotese `+0x94`: 0,0,0,+1,0. O comando produz CSV e informa divergencias, mas nao valida visualmente os eventos do jogo nem promove enderecos. Adicionados testes sinteticos; execucao contra a partida real pendente.

## Validacao comportamental com controle militar — cinco fases (CSV real)

O usuario executou `--verify-villager-094` e compartilhou `villager_validation_094.csv`. As cinco etapas foram registradas com `status=ok` e `stage_pass=1`, sem erros e com raiz `ResourceTable=0x10164AF0` **apenas nesta sessao**.

| Etapa | +0x94 float32 | +0x2C float32 | +0x4C float32 |
|---|---:|---:|---:|
| baseline | 3 | 4 | 5 |
| militia_completed | 3 | 5 | 6 |
| militia_lost | 3 | 4 | 6 |
| villager_completed | 4 | 5 | 7 |
| villager_lost | 3 | 4 | 7 |

Conclusao: `ResourceTable+0x94` passou o teste comportamental especifico para aldeoes vivos (milicia nao altera, aldeao altera e retorna na morte). `+0x2C` acompanha populacao atual, enquanto `+0x4C` e compativel com acumulado de unidades produzidas, e nao exclusivo de aldeoes. Apesar da evidencia funcional forte, a cadeia e o endereco do contador ainda exigem verificacao em partida nova **apos reinicio completo do processo**, alem de controles adicionais; estado no CSV permanece `hipotese` ate isso acontecer. Nao reutilizar `0x10164AF0` noutra sessao. O experimento depende de o operador ter realizado fielmente as etapas indicadas.
