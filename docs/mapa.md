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
