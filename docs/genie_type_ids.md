# Catálogo dos Genie Type IDs — Age of Empires II HD

Este catálogo contém **41 identificadores distintos** extraídos de registros e módulos do `AoE2HD.control`, da documentação v8/v9 do memory_map e das capturas v2/v3/v4.2 descritas nas conversas experimentais. Arquivo de referência: [dados/genie_type_ids.csv](../dados/genie_type_ids.csv).

**Importante:** Genie Type IDs são **valores de entidades**, não endereços nem offsets. Não devem ser inseridos em `dados/enderecos.csv`. O campo validado para ler o tipo continua sendo `[Object+0x0C]+0x10` como uint16 na versão `AoK HD.exe 5.8.INT`; isso não valida automaticamente o significado de todos os valores lidos.

## Aldeões masculinos (base em primeiro lugar; restantes por código)

| ID | Profissão candidata |
|---:|---|
| 83 | Aldeão |
| 56 | Pescador |
| 118 | Construtor |
| 120 | Coletor |
| 122 | Caçador |
| 123 | Lenhador |
| 124 | Mineiro de Pedra |
| 259 | Agricultor |
| 579 | Garimpeiro |
| 592 | Pastor |

## Aldeões femininos (base em primeiro lugar; restantes por código)

| ID | Profissão candidata |
|---:|---|
| 293 | Aldeão |
| 57 | Pescador |
| 212 | Construtor |
| 214 | Agricultor |
| 216 | Caçador |
| 218 | Lenhador |
| 220 | Mineiro de Pedra |
| 354 | Coletor |
| 581 | Garimpeiro |
| 590 | Pastor |

O mapeamento dos aldeões é **candidato de variante Genie associada à profissão observada no HUD**, não indicador de execução instantânea de uma tarefa. Evidências: `AoE2HD.control/docs/villager_job_discovery_v3.md`, e capturas v2/v3 (inclusive sequência do operador) e `villager_job_discovery_v3.py`. Identificações replicadas não devem ser confundidas com as rotuladas em uma única captura.

## Outros controles de tipo

Tipos com identidade confirmada funcionalmente nos experimentos v8: 12 Quartel; 68 Moinho; 70 Casa; 74 Milícia; 93 Lanceiro; 109 Centro da Cidade (Idade das Trevas); 448 Batedor; 562 Armazém de Madeira. Tipos 4 Arqueiro e 125 Monge foram observados como controles opcionais na mesma investigação. Tipo 83 aldeão masculino consta também dos controles v8.

## Tipos registrados sem semântica comprovada

Valores `48, 65, 87, 101, 104, 126, 285, 618, 619, 620` foram observados na investigação v9/v9.1, mas **não recebem nomes presumidos**. O ID `594` foi observado em quatro objetos locais com `Object+0x78=0` no diagnóstico v4.2, e associado a **Ovelha** por catálogo externo, ainda sujeito a verificação visual independente nesta versão HD; não constitui prova de identidade funcional na captura.

## Rastreabilidade e uso

- Proveniência original de tipos não aldeões: [validacao_tipos_unidades_v8_2026-10-18.md](validacao_tipos_unidades_v8_2026-10-18.md).
- Variantes masculinas e femininas: `AoE2HD.control/docs/villager_job_discovery_v3.md`.
- Tipo 594: diagnóstico `AoE2HD.control/docs/villager_scanner_diagnostics_v4_2.md` e CSV experimental.
- Estado `tipo_observado_sem_rotulo` preserva identificadores sem inventar interpretação; `candidato_catalogo_externo` não é validação de captura.
- A incorporação destes IDs não altera os 64 registros de endereços existentes, seus módulos ou os respectivos estados de evidência.
