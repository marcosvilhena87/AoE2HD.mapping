# Investigacao do contador de milicias

O controlador AoE2HD.control disponibiliza `--discover-militia`, uma busca somente leitura de campos uint16, uint32 e float32 nas estruturas conhecidas e ponteiros filhos limitados. Protocolo guiado: baseline, milicia +1, morte da milicia, producao de duas milicias, perda de uma, producao de aldeao com milicia restante. Assinatura relativa esperada para milicias vivas: `0,+1,0,+2,+1,+1`. Todos os candidatos permanecem **nao validados** ate captura real e controles adicionais. A descoberta nunca utiliza enderecos absolutos de sessoes antigas. Nenhum offset de milicia foi identificado ate o momento, portanto nao ha nova linha no catalogo de enderecos.

## Resultado real da Discovery de milicias (seis fases)

O arquivo `militia_discovery.csv` gerado no jogo mostrou 43 regioes monitoradas, 1730 registros variaveis e tres interpretacoes classificadas como compativeis, correspondentes a **dois enderecos fisicos distintos**:

- `WorldPlayer local+0x36C`, absoluto apenas nesta sessao `0x1440E388`: tanto uint16 como uint32 observaram `0,1,0,2,1,1` (mesmos bytes; tipo ainda nao diferenciado).
- `ResourceTable+0xA0`, absoluto apenas nesta sessao `0x102BAC70`: float32 observou `1,2,1,3,2,2`. Tem deslocamento inicial +1, sem semantica exclusiva demonstrada.

As fases sao baseline, conclusao de 1 milicia, morte dessa milicia, conclusao de 2 milicias, morte de 1 dentre as duas, conclusao de aldeao. O offset `WorldPlayer+0x36C` e o melhor candidato a milicias vivas e responde corretamente ao controle de aldeao, mas **nao foi validado apos reinicio completo**. `ResourceTable+0xA0` permanece candidato de significado desconhecido, embora tenha transicoes correlatas. Devem ser realizados controles com outras unidades militares e um novo processo para confirmar especificidade e estabilidade da cadeia. Nao inferir que `uint16` e `uint32` sejam campos distintos.

## Novo teste apos reinicio — implementado, execucao pendente

Foi incluido no AoE2HD.control o comando `--verify-militia-restart`, com sete fases: baseline, milicia concluida, milicia perdida, lanceiro concluido, lanceiro perdido, aldeao concluido, aldeao perdido. A verificacao resolve WorldPlayer e ResourceTable automaticamente; registra `WorldPlayer+0x36C` nos formatos uint16/uint32 e os controles float32 `ResourceTable+0x94` (aldeoes) e `+0x2C` (populacao). As assinaturas relativas esperadas sao milicias `0,1,0,0,0,0,0`, aldeoes `0,0,0,0,0,1,0` e populacao `0,1,0,1,0,1,0`. Confirma somente compatibilidade experimental; offsets de milicias continuam hipotese ate observacao real em outra sessao.

## Resultado da validacao apos reinicio (CSV real)

Arquivo `militia_validation_restart.csv`: sete etapas, `status=ok` e `stage_pass=1` em todas. A cadeia dinamica resolveu `WorldPlayer local=0x18B4440C` e `ResourceTable=0x13F4B148` nesta sessao; estes enderecos absolutos nao devem ser reaproveitados. As fases baseline, milicia concluida, milicia perdida, lanceiro concluido, lanceiro perdido, aldeao concluido, aldeao perdido produziram: `WorldPlayer+0x36C` uint16 e uint32 `0,1,0,0,0,0,0`; `ResourceTable+0x94` float32 `3,3,3,3,3,4,3`; `ResourceTable+0x2C` float32 `4,5,4,5,4,5,4`. Isso confirma a especificidade do campo para milicias em relacao a lanceiros e aldeoes nas condicoes experimentadas, com reprodutibilidade apos reinicio. Status atualizado para `validado` **funcionalmente**; extensao a diferentes tecnologias, civilizacoes e upgrades de unidades ainda nao comprovada. O campo `ResourceTable+0xA0` segue hipotese separada.
