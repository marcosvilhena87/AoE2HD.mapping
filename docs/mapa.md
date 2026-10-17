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
