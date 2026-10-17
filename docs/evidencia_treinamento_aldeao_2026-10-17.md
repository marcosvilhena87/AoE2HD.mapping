# Evidência de progresso de treinamento de aldeão — 2026-10-17

Fonte: Cheat Engine, tabela XML e resultados de "Find out what writes/accesses this address" fornecidos pelo usuário. Alvo informado: AoK HD.exe (versão 5.8.INT no projeto; confirmar a versão da sessão).

## Endereço observado e tipo

- Endereço absoluto **da sessão**: `0x18769C5C`.
- Cheat Engine: `VariableType=4 Bytes`, `LastState Value=17`; isso **não comprova** armazenamento uint32 nem escala 0–100.
- As instruções observadas leem/escrevem **16 bits** (`ax`/`cx`), indicando um campo `uint16` candidato no endereço, possivelmente com outro campo de 16 bits adjacente.
- `EDI=0x18769C5A` e `ESI/EBX=0x18769C5C` nas operações de zeragem; candidato vizinho em `0x18769C5A`, sem semântica confirmada.

## Evidência estrutural importante

Na instrução `AoK HD.exe+0x318E3A`:

```asm
cmp ax, [ebx+0x00000C44]
EBX=0x18769018
```

A soma `0x18769018 + 0x0C44 = 0x18769C5C` corresponde **exatamente** ao endereço investigado. Portanto **CandidateTrainingBase + 0x0C44** é um offset estrutural candidato `uint16`, sujeito a confirmar que `0x18769018` é de fato a base da estrutura de treinamento e que se mantém estável entre sessões.

## Trechos de código observados (RVAs no executável)

- `+0x4CB0B3`: `mov [esi], ax` depois de `xor eax,eax`; ESI=`0x18769C5C` (zera campo de 16 bits). Na sequência `mov ecx,[ecx+0x150]`, com ECX=`0x18866938` nessa captura.
- `+0x4BE396`: `mov [ebx], ax` depois de `xor eax,eax`; EBX=`0x18769C5C` (zera o mesmo campo). Na sequência `mov ecx,[esi+0x220]`, ESI=`0x18866938`.
- `+0x0BF111`: `mov [eax],cx`; EAX=`0x18769C5C`, CX=`0x0011` (decimal 17), escreve 16 bits.
- `+0x318E3A`: `cmp ax,[ebx+0xC44]`; EBX=`0x18769018`. Compara 16 bits nesse endereço, **não** escreve nele.

Dois blocos de capturas ocorreram entre aproximadamente 18:21:56–18:23:17 no relógio apresentado pelo usuário; registros "First seen/Last seen" não indicam que a fila começou ou terminou nesses instantes.

## Relação com fila e população

O histórico já contém `Building+0x214/+0x218/+0x21C` para fila de produção, porém **não há evidência de que `0x18769018` seja o objeto `Building`**. `CandidateTrainingBase+0x0C44` e `Building+0x214` não devem ser unidos em uma mesma estrutura sem verificação.

Hipótese: campo guarda progresso ou contador relacionado ao treinamento. Alternativas: temporizador, etapa de animação, ID ou estado da interface. Testar leituras `uint16` de `+0x0C44` e `+0x0C42` antes/durante/após treinamento, cancelamento, seleção e reinício. Comparar com o percentual do HUD; uma observação igual a 17 não estabelece relação percentual nem unidade de tempo.

**Status:** offset `+0x0C44` como `hipotese`; endereços absolutos como `sessao_observada`. Código de instrução representa RVAs de funções, não endereços de dados de treinamento.
