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
