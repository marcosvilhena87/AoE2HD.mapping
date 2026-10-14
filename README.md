# AoE2HDControl

Projeto de engenharia reversa e reconstrução de uma **API de controle para Age of Empires II HD (AoK HD.exe 5.8.INT, x86)**.

O **AoE2Control para Definitive Edition** é usado **apenas como referência conceitual** para definir funcionalidades e alvos semânticos. **Offsets, layouts e estruturas da Definitive Edition não são considerados válidos no HD.** Toda estrutura usada aqui deve ser investigada e validada especificamente na versão HD.

Esta é uma **reimplementação do zero**. O repositório histórico [AoE2HDControl.v0](https://github.com/marcosvilhena87/AoE2HDControl.v0) preserva descobertas de engenharia reversa, endereços, offsets e experimentos anteriores; não é necessário transportar sua arquitetura nem aceitar hipóteses antigas sem validação.

## Objetivo de longo prazo

Construir gradualmente uma API para observar o estado do jogo e, em fases posteriores, avaliar mecanismos de controle. Prioridades:

1. Leitura confiável de `World`, jogadores e recursos;
2. Enumeração das unidades de cada jogador **sem necessidade de seleção**;
3. Identificação de aldeões, tipos, posições, saúde e estados;
4. Telemetria de economia, produção e demais eventos;
5. Investigação posterior de comandos de controle, somente com fundamentos específicos do HD.

## Estado atual — primeira implementação

- Localiza o processo e a base do módulo `AoK HD.exe`;
- Lê `World`, jogadores e recursos econômicos em modo somente leitura;
- Valida vtables, limites do vetor e referências de retorno;
- Exibe diagnóstico, sem reutilizar ponteiros entre partidas.

**Teste real informado:** base de módulo `0x00B90000`, `World=0x1929E848`, três jogadores (incluindo Gaia), jogador local no índice 1 e leitura de comida, madeira, pedra e ouro. Isso confirma o caminho básico no cenário testado, não todas as configurações do jogo.

**Ainda não implementado:** enumeração automática de unidades, identificação de aldeões e comandos de controle.

## Compilar no Windows (Visual Studio com C++ e CMake)

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A Win32
cmake --build build --config Release
.\build\Release\aoe2hd_probe.exe
```

Se estiver usando Visual Studio 2022, altere o gerador para `Visual Studio 17 2022`.

## Segurança e limites atuais

O protótipo usa `OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION)` e `ReadProcessMemory`. Não realiza escrita, injeção nem envio de comandos. Durante mudanças de partida, a memória pode variar; valores são rejeitados quando as validações falham. A prontidão estrutural não prova, por si só, prontidão total de gameplay.

Veja [docs/memory-map.md](docs/memory-map.md) para os endereços reutilizados, a origem das evidências e os limites da validação.
