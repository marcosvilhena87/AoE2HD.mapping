#include <windows.h>
#include <tlhelp32.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {
constexpr uint32_t kOwnerRva = 0x006F2C58;
constexpr uint32_t kWorldVtable = 0x005D329C;
constexpr uint32_t kGaiaVtable = 0x005DC8A0;
constexpr uint32_t kHumanVtable = 0x005DCAE4;
constexpr uint32_t kComputerVtable = 0x005DC618;
constexpr uint32_t kMaxEntries = 9;

struct Handle {
    HANDLE value = nullptr;
    explicit Handle(HANDLE h = nullptr) : value(h) {}
    ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
};

bool plus(uint32_t base, uint32_t offset, uint32_t& result) {
    if (base > UINT32_MAX - offset) return false;
    result = base + offset;
    return true;
}
bool read(HANDLE process, uint32_t address, void* output, SIZE_T bytes) {
    SIZE_T obtained = 0;
    return ReadProcessMemory(process, reinterpret_cast<LPCVOID>(static_cast<uintptr_t>(address)),
                             output, bytes, &obtained) != FALSE && obtained == bytes;
}
bool read32(HANDLE process, uint32_t base, uint32_t offset, uint32_t& result) {
    uint32_t address = 0;
    return plus(base, offset, address) && read(process, address, &result, sizeof(result));
}
bool matchesRva(uint32_t base, uint32_t rva, uint32_t value) {
    uint32_t expected = 0;
    return plus(base, rva, expected) && expected == value;
}
uint32_t findProcess() {
    Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (snapshot.value == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (!Process32FirstW(snapshot.value, &entry)) return 0;
    do {
        if (_wcsicmp(entry.szExeFile, L"AoK HD.exe") == 0) return entry.th32ProcessID;
    } while (Process32NextW(snapshot.value, &entry));
    return 0;
}
bool moduleBase(uint32_t pid, uint32_t& base) {
    Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid));
    if (snapshot.value == INVALID_HANDLE_VALUE) return false;
    MODULEENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (!Module32FirstW(snapshot.value, &entry)) return false;
    do {
        if (_wcsicmp(entry.szModule, L"AoK HD.exe") == 0) {
            const uintptr_t address = reinterpret_cast<uintptr_t>(entry.modBaseAddr);
            if (address > UINT32_MAX) return false;
            base = static_cast<uint32_t>(address);
            return true;
        }
    } while (Module32NextW(snapshot.value, &entry));
    return false;
}
void hexField(const char* label, uint32_t value) {
    std::cout << label << "0x" << std::hex << std::uppercase << value << std::dec << "\n";
}
bool probe(HANDLE process, uint32_t base) {
    uint32_t owner = 0, ownerAddress = 0, world = 0, vt = 0;
    if (!plus(base, kOwnerRva, ownerAddress) || !read32(process, ownerAddress, 0, owner))
        { std::cerr << "Nao foi possivel ler Owner.\n"; return false; }
    if (!owner || !read32(process, owner, 0xC8, world) || !world)
        { std::cerr << "World indisponivel (fora de partida?).\n"; return false; }
    if (!read32(process, world, 0, vt) || !matchesRva(base, kWorldVtable, vt))
        { std::cerr << "Vtable de World divergente.\n"; return false; }

    uint32_t local = 0, begin = 0, end = 0, cap = 0;
    if (!read32(process, world, 0x174, local) ||
        !read32(process, world, 0x184, begin) ||
        !read32(process, world, 0x188, end) ||
        !read32(process, world, 0x18C, cap))
        { std::cerr << "Falha na leitura do vetor de jogadores.\n"; return false; }
    if (!begin || begin % 4 || end < begin || cap < end ||
        (end - begin) % 8 || (cap - begin) % 8 ||
        (end - begin) / 8 < 2 || (cap - begin) / 8 > kMaxEntries)
        { std::cerr << "Vetor de jogadores invalido ou em construcao.\n"; return false; }
    const uint32_t count = (end - begin) / 8;
    if (local == 0 || local >= count)
        { std::cerr << "Indice local fora dos limites.\n"; return false; }
    std::vector<uint32_t> players;
    players.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t addr = 0, object = 0, control = 0, playerVt = 0, back = 0, kind = 0;
        if (!plus(begin, i * 8, addr) ||
            !read32(process, addr, 0, object) || !read32(process, addr, 4, control) ||
            !object || !control || control > UINT32_MAX - 12 || object != control + 12 ||
            !read32(process, object, 0, playerVt) ||
            !read32(process, object, 4, back) ||
            !read32(process, object, 8, kind))
            { std::cerr << "Entrada de jogador incompleta/invalida.\n"; return false; }
        const uint32_t rva = i == 0 ? kGaiaVtable : (i == local ? kHumanVtable : kComputerVtable);
        const uint32_t expectedKind = i == 0 ? 2u : (i == local ? 1u : 3u);
        if (!matchesRva(base, rva, playerVt) || back != world || kind != expectedKind)
            { std::cerr << "Identidade de jogador divergente.\n"; return false; }
        players.push_back(object);
    }
    uint32_t owner2 = 0, world2 = 0, begin2 = 0, end2 = 0, cap2 = 0, local2 = 0;
    if (!read32(process, ownerAddress, 0, owner2) ||
        !read32(process, owner, 0xC8, world2) ||
        !read32(process, world, 0x184, begin2) ||
        !read32(process, world, 0x188, end2) ||
        !read32(process, world, 0x18C, cap2) ||
        !read32(process, world, 0x174, local2) ||
        owner2 != owner || world2 != world ||
        begin2 != begin || end2 != end || cap2 != cap || local2 != local)
        { std::cerr << "Partida mudou durante a leitura.\n"; return false; }

    hexField("World: ", world);
    std::cout << "Jogadores (incluindo Gaia): " << count << "\n";
    std::cout << "Indice local: " << local << "\n";
    hexField("Jogador local: ", players[local]);

    uint32_t table = 0;
    if (!read32(process, players[local], 0x3C, table) || !table)
        { std::cout << "Recursos indisponiveis.\n"; return true; }
    std::array<float, 4> resources{};
    if (!read(process, table, resources.data(), sizeof(resources)))
        { std::cout << "Tabela de recursos ilegivel.\n"; return true; }
    for (float v : resources)
        if (!std::isfinite(v) || v < 0.0f)
            { std::cout << "Recursos invalidos.\n"; return true; }
    uint32_t table2 = 0, back2 = 0;
    if (!read32(process, players[local], 0x3C, table2) ||
        !read32(process, players[local], 4, back2) ||
        table2 != table || back2 != world)
        { std::cout << "Recursos alterados durante a leitura.\n"; return true; }
    std::cout << std::fixed << std::setprecision(1)
              << "Comida: " << resources[0] << "\n"
              << "Madeira: " << resources[1] << "\n"
              << "Pedra: " << resources[2] << "\n"
              << "Ouro: " << resources[3] << "\n";
    return true;
}
} // namespace

int main() {
    const uint32_t pid = findProcess();
    if (!pid) { std::cerr << "AoK HD.exe nao encontrado.\n"; return 1; }
    uint32_t base = 0;
    if (!moduleBase(pid, base)) { std::cerr << "Modulo do jogo nao encontrado.\n"; return 1; }
    Handle process(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid));
    if (!process.value) { std::cerr << "Sem acesso somente leitura ao processo.\n"; return 1; }
    std::cout << "AoE2HDControl - leitura experimental (5.8.INT)\n";
    hexField("Module base: ", base);
    return probe(process.value, base) ? 0 : 2;
}
