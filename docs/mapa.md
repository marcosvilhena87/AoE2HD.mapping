# Mapa base

```
[ModuleBase + 0x006F2C58] -> Owner
[Owner + 0xC8]           -> World
[World + 0x174]         -> localPlayerIndex
[World + 0x184]         -> players.begin
[World + 0x188]         -> players.end
[World + 0x18C]         -> players.capacity
```

World.players possui entradas históricas de 8 bytes, com `object=controlBlock+0x0C`. Gaia ocupa índice zero nos cenários historicamente registrados.

`WorldPlayer+0x3C` aponta a recursos (food, wood, stone, gold), em quatro floats nos offsets 0x00/0x04/0x08/0x0C.

Seleção experimental: `LocalPlayer+0x15C` e `+0x160`. Identidade experimental: `Object+0x08` (ID), `+0x0C` (tipo), `+0x10` (owner), `+0x34` (HP).

Fila experimental: `Building+0x214` (ponteiro), `+0x218` (high16, grupos), `+0x21C` (low16, unidades).

**Não confirmado:** coleção global em `World+0x88`, campos `CandidateBase+0x178/+0x28C`. A base `0x14D6B188` era de uma sessão e não tem caminho estável verificado.

Veja [CSV](../dados/enderecos.csv) para tipos e status.

## Investigação de população (candidatos ainda não localizados)

O catálogo atual não contém campos confirmados de população atual, limite populacional ou população em produção. A sondagem de pesquisa `AoE2HD.control --probe-population` busca, sem escrita, valores numéricos plausíveis nos primeiros 0x400 bytes de `WorldPlayer` e da `ResourceTable`, comparando-os aos números **informados manualmente a partir da interface** em vários estados. Nenhum deslocamento dentro dessa janela é presumido válido até haver correspondência consistente, mudanças positivas e negativas e repetição após reinício. 

Os resultados são candidatos em arquivo CSV gerado pelo controle, não entradas confirmadas no catálogo. Uma correspondência acidental — sobretudo números 0, 1, 200 etc. — não prova semântica. Não confundir população utilizada com limite, unidades em fila ou habitações.

## Instrumentação de fila de produção (somente leitura)

O leitor histórico `include/ProductionQueueReader.h` usa `Building+0x214` como ponteiro, metade superior do word em `+0x218` para número de grupos e metade inferior do word em `+0x21C` para total de unidades. Grupos são entradas de 4 bytes `<uint16 type, uint16 quantity>` conforme `ProductionQueueEntriesV6.h`; uma releitura dos três campos detecta possíveis alterações concorrentes. A origem de `Building` ainda precisa ser validada por seleção de edifício ou outra descoberta independente. O monitor experimental aceita endereço absoluto **temporário** informado manualmente e registra fila e população no mesmo instante; um resultado coerente não eleva os offsets a validados.

## Campo candidato de treinamento de aldeão

O Cheat Engine observou `0x18769C5C` com valor 17 (configuração 4 Bytes), mas as instruções `mov [eax],cx`, `mov [esi],ax`, `mov [ebx],ax` e `cmp ax,[ebx+0xC44]` manipulam 16 bits. Na captura da comparação, `EBX=0x18769018` e `EBX+0xC44=0x18769C5C`. Assim, `TrainingCandidateBase+0xC44` é offset candidato uint16; não foi demonstrado que `TrainingCandidateBase` seja um objeto `Building` nem que o valor represente uma porcentagem direta. Ver [evidência detalhada](evidencia_treinamento_aldeao_2026-10-17.md).

## Investigação: localizar TrainingCandidateBase a partir da construção selecionada (2026-10-17)

Código histórico inspecionado no commit 62c3a7e1a22dec68af31f6501fda3c4f7f2795a7: `src/production_queue_structure_discovery_v7.cpp`, `src/unit_reference_discovery_v4.cpp`, `src/entity_graph_discovery_v5.cpp` e `include/UnitReferenceDiscovery.h`.

O código v7 demonstra método histórico para obter **o objeto da construção selecionada** em uma sessão: módulo+`0x006F2C58` → Owner; Owner+`0xC8` → World; World+`0x174` → índice local; World+`0x184` → vetor de player entries de 8 bytes; entrada local → ponteiro WorldPlayer; WorldPlayer+`0x15C` e `+0x160` → objeto selecionado (somente se ambos forem idênticos, não nulos). A rotina verifica a vtable de World, identidade `selected+0x08`, seleção estável antes/depois e pede confirmação visual de edifício de produção. Então lê a fila via `Building+0x214/+0x218/+0x21C`. Logo, existe **âncora histórica de seleção** reutilizável, ainda necessitando revalidação nesta versão/sessão.

**Não existe neste código cadeia demonstrada entre Building selecionado e TrainingCandidateBase**. O Cheat Engine apontou o acesso `[EBX+0xC44]` com EBX absoluto `0x18769018`, mas em outra captura ESI/ECX=`0x18866938` ao acessar `[esi+0x220]` e `[ecx+0x150]`. Diferença entre endereços e instruções não comprova que um seja origem do outro. Não assumir que `selected+0xC44` represente o campo de progresso.

Experimento planejado: selecionar Centro Urbano, confirmar `+0x15C/+0x160` e ID estáveis; registrar ponteiro do objeto, comparar com base `0x18769018` **somente se ainda for a mesma sessão**, inspecionar o `selected+0xC42/+0xC44` e opcionalmente uma janela delimitada de referências de primeiro nível à base; alternar seleção para Quartel, desmarcar e reiniciar o processo. Identificar a relação estrutural somente após resultados de controlo positivos/negativos. Não promover offsets.

## Selected Building Training Discovery v1 — protocolo instrumental

A cadeia histórica para objeto selecionado foi encontrada em `src/production_queue_structure_discovery_v7.cpp` (commit anterior à migração). Implementação v1 em `AoE2HD.control` usa `Owner -> World -> PlayerEntry local -> LocalPlayer+0x15C/+0x160`, exige igualdade das âncoras e identidade estável `Object+0x08`. Captura campos `+0xC42/+0xC44` **diretamente** no objeto, apenas como sondagem; inspeciona também, com limite de 0x800 bytes e um único salto, ponteiros candidatos cuja estrutura alvo possua os mesmos campos. Permite opcionalmente comparar com uma base de sessão independente. O relatório preserva resultados inconclusivos e não estabelece relação causal nem cadeia estável até `TrainingCandidateBase`. Teste necessário com Centro Urbano, Quartel, cancelamento e reinício; não promover offsets existentes.

## Resultado de Selected Building Training Discovery v1 (184 linhas)

Capturas idle, training, cancel_or_complete do mesmo objeto selecionado 0x186D0908, ID 2180. Foram 60/62/62 candidatos por estágio, respectivamente; 116 leituras de treinamento acessíveis e 68 ilegíveis. A leitura direta do objeto mostrou C42=0 e C44=38328 invariáveis. Nenhum candidato legível teve C42=83 ou 74. Houve alterações não diagnósticas em alguns ponteiros ou dados, mas nenhuma cadeia direta/de um salto foi identificada. Todas as linhas têm known_base_match=0; a execução não usou --known-training-base, logo esse zero não descarta um endereço específico. O objeto selecionado permaneceu estável durante cada captura. Endereços absolutos são válidos apenas nesta sessão. Próximo experimento: comparar contra a base real obtida por Cheat Engine na mesma sessão e procurar referências delimitadas de dois saltos com controles negativos. Não promover offsets.
