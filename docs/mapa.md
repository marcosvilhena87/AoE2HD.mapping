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
