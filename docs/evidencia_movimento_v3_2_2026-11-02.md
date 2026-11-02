# Evidência v3.2 — movimento do Object ID 2630 (02/11/2026)

Origem: saída PowerShell fornecida pelo operador após o comando
`probe-position-cursor-v3-2 --id 2630 --samples 30 --interval 0.5`,
em sessão de Age of Empires II HD (`AoK HD.exe 5.8.INT`, versão
declarada, hash do executável não medido nesta captura).

Arquivo: [trajetoria_2630_v3_2.csv](../dados/trajetoria_2630_v3_2.csv).

- Campos observados: `Object+0x3C` e `Object+0x40` lidos como float32.
- Object ID de sessão: `2630`; classe `4`, Genie Type ID `83`
  identificados em captura anterior da mesma investigação.
- 30 amostras; intervalo solicitado 0,5 s (timestamps individuais
  não fornecidos).
- Primeira posição: (47.69685363769531, 31.5).
- Última posição: (63.80374526977539, 34.9776725769043).
- X aumentou ao longo das 30 leituras. Y caiu até a amostra 11 e
  aumentou depois. Compatível com deslocamento real.
- Captura anterior em repouso fornecida pelo operador: 12 amostras
  (32.5, 35.5), sem variação durante movimentação somente do cursor.
- Distância entre pontos inicial e final não equivale à distância
  percorrida na trajetória.

**Status probatório:** `validado_experimentalmente_escopo`, restrito
ao movimento observado deste objeto e sessão. NÃO promover os offsets
a `validado` no catálogo canônico; não inferir estabilidade em outras
builds, tipos de objetos, ponteiros de cursor ou posição da câmera.

**Limites:** CSV reconstruído fielmente a partir da transcrição de
30 valores exibidos no PowerShell; JSON bruto `position_movement_v3_2.json`
não foi disponibilizado aqui; não há tempo medido por amostra, hash da
build ou repetição após reinício. Nenhuma evidência direta de
`cursor_world` ou de projeção isométrica foi obtida.
