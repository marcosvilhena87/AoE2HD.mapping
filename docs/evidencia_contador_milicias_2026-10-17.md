# Investigacao do contador de milicias

O controlador AoE2HD.control disponibiliza `--discover-militia`, uma busca somente leitura de campos uint16, uint32 e float32 nas estruturas conhecidas e ponteiros filhos limitados. Protocolo guiado: baseline, milicia +1, morte da milicia, producao de duas milicias, perda de uma, producao de aldeao com milicia restante. Assinatura relativa esperada para milicias vivas: `0,+1,0,+2,+1,+1`. Todos os candidatos permanecem **nao validados** ate captura real e controles adicionais. A descoberta nunca utiliza enderecos absolutos de sessoes antigas. Nenhum offset de milicia foi identificado ate o momento, portanto nao ha nova linha no catalogo de enderecos.

## Resultado real da Discovery de milicias (seis fases)

O arquivo `militia_discovery.csv` gerado no jogo mostrou 43 regioes monitoradas, 1730 registros variaveis e tres interpretacoes classificadas como compativeis, correspondentes a **dois enderecos fisicos distintos**:

- `WorldPlayer local+0x36C`, absoluto apenas nesta sessao `0x1440E388`: tanto uint16 como uint32 observaram `0,1,0,2,1,1` (mesmos bytes; tipo ainda nao diferenciado).
- `ResourceTable+0xA0`, absoluto apenas nesta sessao `0x102BAC70`: float32 observou `1,2,1,3,2,2`. Tem deslocamento inicial +1, sem semantica exclusiva demonstrada.

As fases sao baseline, conclusao de 1 milicia, morte dessa milicia, conclusao de 2 milicias, morte de 1 dentre as duas, conclusao de aldeao. O offset `WorldPlayer+0x36C` e o melhor candidato a milicias vivas e responde corretamente ao controle de aldeao, mas **nao foi validado apos reinicio completo**. `ResourceTable+0xA0` permanece candidato de significado desconhecido, embora tenha transicoes correlatas. Devem ser realizados controles com outras unidades militares e um novo processo para confirmar especificidade e estabilidade da cadeia. Nao inferir que `uint16` e `uint32` sejam campos distintos.
