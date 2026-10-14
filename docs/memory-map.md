# Mapa de memoria — ponto de partida

**Alvo documentado:** `AoK HD.exe` 5.8.INT, PE32 (x86). Dados extraidos da investigacao no repositorio historico [AoE2HDControl.v0](https://github.com/marcosvilhena87/AoE2HDControl.v0), especialmente `include/WorldReader.h`, `src/WorldReader.cpp` e `docs/structures.md`.

## RVAs, nao enderecos de sessao

| Item | Valor | Evidencia historica |
|---|---:|---|
| Ponteiro global Owner | RVA `0x006F2C58` | cadeia validada |
| Vtable World | RVA `0x005D329C` | RTTI/instanciacao |
| Vtable Gaia | RVA `0x005DC8A0` | snapshots |
| Vtable HumanOrCoop | RVA `0x005DCAE4` | snapshots |
| Vtable Computer | RVA `0x005DC618` | snapshots |
| Owner.World | `+0xC8` | cadeia validada |
| World.localPlayerIndex | `+0x174` | snapshots |
| World.players begin/end/cap | `+0x184/+0x188/+0x18C` | vetor com entradas de 8 bytes |
| WorldPlayer.world | `+0x04` | back-pointer |
| WorldPlayer.discriminator | `+0x08` | valores observados 2/1/3; semantica nao concluida |
| WorldPlayer.resourceTable | `+0x3C` | recursos individuais validados |
| ResourceTable food/wood/stone/gold | `+0x00/+0x04/+0x08/+0x0C` | quatro float32 |
| PlayerEntry.object / controlBlock | `+0x00/+0x04` | object = controlBlock + 0x0C |

O buffer `World.mPlayers` tem entradas de oito bytes (Gaia + ate oito jogadores). O vetor pode existir e estar vazio; durante a construcao, as entradas podem estar temporariamente incompletas. O indice 0 e Gaia, e o indice local deve ser diferente de zero na configuracao historica validada.

**Observacao:** readiness estrutural nao equivale a comprovacao de que todos os sistemas do gameplay estao operantes. Nao reutilizar enderecos runtime absolutos de exemplos historicos. Todos os offsets sao especificos da versao documentada e devem ser testados novamente no executavel real.

## Proximos marcos

1. Confirmar World/jogadores/recursos em varias partidas, reinicios e configuracoes.
2. Investigar enumeracao de unidades sem selecao com evidencias observaveis.
3. Identificar classe/tipo de aldeao e validar contagem por jogador.
4. Apenas depois acrescentar telemetria mais sofisticada.
