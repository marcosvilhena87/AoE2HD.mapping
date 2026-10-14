# AoE2HDControl

Reconstrucao do zero do leitor de memoria do **Age of Empires II HD**, alvo **AoK HD.exe 5.8.INT (PE32/x86)**.

Este projeto NAO copia o codigo da versao historica [AoE2HDControl.v0](https://github.com/marcosvilhena87/AoE2HDControl.v0). Reaproveita exclusivamente os enderecos/offsets previamente investigados. Nenhum offset da Definitive Edition e assumido valido.

## Escopo da primeira versao

- Localiza o processo e a base do modulo `AoK HD.exe`;
- Le World, jogadores e recursos economicos em modo somente leitura;
- Valida vtables, limites do vetor e referencias de retorno;
- Exibe diagnostico, sem armazenar ponteiros para outras partidas.

**Nao enumera unidades ainda.** Esse sera o proximo marco, apos nova verificacao experimental das estruturas de colecao.

## Compilar no Windows (Visual Studio com C++ e CMake)

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A Win32
cmake --build build --config Release
.\build\Release\aoe2hd_probe.exe
```

Se estiver usando Visual Studio 2022, altere o gerador para `Visual Studio 17 2022`.

## Limites

Somente leitura: utiliza `OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION)` e `ReadProcessMemory`. Nenhuma escrita, injecao ou envio de comandos. O jogo pode nao estar em partida ou a memoria pode mudar entre leituras. Recursos sao marcados como indisponiveis caso os validadores falhem.

Veja [docs/memory-map.md](docs/memory-map.md) para fontes, layout e limites das evidencias.
