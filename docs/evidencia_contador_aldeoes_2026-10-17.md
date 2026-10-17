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
