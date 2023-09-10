#include "Memory.h"

#include "gameboy_config.h"

using namespace std;

/* ***********************************************************************************************************
    CONSTRUCTOR AND (DE)INIT
*********************************************************************************************************** */
Memory* Memory::instance = nullptr;

Memory* Memory::getInstance(const Cartridge& _cart_obj) {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }

    instance = new Memory(_cart_obj);
    return instance;
}

void Memory::resetInstance() {
    if (instance != nullptr) {
        instance->CleanupMemory();

        delete instance;
        instance = nullptr;
    }
}


Memory::Memory(const Cartridge& _cart_obj) {
    this->isCgb = _cart_obj.GetIsCgb();

    InitMemory(_cart_obj);
}

/* ***********************************************************************************************************
    INITIALIZE MEMORY
*********************************************************************************************************** */
void Memory::InitMemory(const Cartridge& _cart_obj) {
    const auto& vec_rom = _cart_obj.GetRomVector();

    if (!ReadRomHeaderInfo(vec_rom)) { return; }

    AllocateMemory();
    if (!CopyRom(vec_rom)) {
        return;
    }
}

bool Memory::CopyRom(const vector<u8>& _vec_rom) {
    if (ROM_0 != nullptr) {
        for (int i = 0; i < ROM_BANK_0_SIZE; i++) {
            ROM_0[i] = _vec_rom[i];
        }
    }
    else {
        return false;
    }

    if (ROM_N != nullptr) {
        for (int i = 0; i < romBankNum; i++) {
            if (ROM_N[i] != nullptr) {
                for (int j = 0; j < ROM_BANK_N_SIZE; j++) {
                    ROM_N[i][j] = _vec_rom[i * ROM_BANK_0_SIZE + j];
                }
            }
            else {
                return false;
            }
        }
    }
    else {
        return false;
    }
}

/* ***********************************************************************************************************
    MANAGE ALLOCATED MEMORY
*********************************************************************************************************** */
void Memory::AllocateMemory() {
    ROM_0 = new u8[ROM_BANK_0_SIZE];
    ROM_N = new u8 * [romBankNum];
    for (int i = 0; i < romBankNum; i++) {
        ROM_N[i] = new u8[ROM_BANK_N_SIZE];
    }

    VRAM_N = new u8 * [isCgb ? 2 : 1];
    for (int i = 0; i < (isCgb ? 2 : 1); i++) {
        VRAM_N[i] = new u8[VRAM_N_SIZE];
    }

    RAM_N = new u8 * [ramBankNum];
    for (int i = 0; i < ramBankNum; i++) {
        RAM_N[i] = new u8[RAM_BANK_N_SIZE];
    }

    WRAM_0 = new u8[WRAM_0_SIZE];
    WRAM_N = new u8 * [isCgb ? 7 : 1];
    for (int i = 0; i < (isCgb ? 7 : 1); i++) {
        WRAM_N[i] = new u8[WRAM_N_SIZE];
    }

    OAM = new u8[OAM_SIZE];

    IO = new u8[IO_REGISTERS_SIZE];

    HRAM = new u8[HRAM_SIZE];
}

void Memory::CleanupMemory() {
    delete[] ROM_0;
    for (int i = 0; i < romBankNum; i++) {
        delete[] ROM_N[i];
    }
    delete[] ROM_N;

    for (int i = 0; i < (isCgb ? 2 : 1); i++) {
        delete[] VRAM_N[i];
    }
    delete[] VRAM_N;

    for (int i = 0; i < ramBankNum; i++) {
        delete[] RAM_N[i];
    }
    delete[] RAM_N;

    delete[] WRAM_0;
    for (int i = 0; i < (isCgb ? 7 : 1); i++) {
        delete[] WRAM_N[i];
    }
    delete[] WRAM_N;

    delete[] OAM;

    delete[] IO;

    delete[] HRAM;
}

/* ***********************************************************************************************************
    ROM HEADER DATA FOR MEMORY
*********************************************************************************************************** */
bool Memory::ReadRomHeaderInfo(const std::vector<u8>& _vec_rom) {
    if (_vec_rom.size() < ROM_HEAD_ADDR + ROM_HEAD_SIZE) { return false; }

    u8 value;

    // get rom info
    value = _vec_rom[ROM_HEAD_ROMSIZE];

    if (value != 0x52 && value != 0x53 && value != 0x54) {                          // TODO: these values are used for special variations
        int total_rom_size = ROM_BASE_SIZE * (1 << _vec_rom[ROM_HEAD_ROMSIZE]);
        romBankNum = total_rom_size >> 14;
    }
    else {
        return false;
    }

    // get ram info
    value = _vec_rom[ROM_HEAD_RAMSIZE];

    switch (value) {
    case 0x00:
        ramBankNum = 0;
        break;
    case 0x02:
        ramBankNum = 1;
        break;
    case 0x03:
        ramBankNum = 4;
        break;
    // not allowed
    case 0x04:
        ramBankNum = 16;
        return false;
        break;
    // not allowed
    case 0x05:
        ramBankNum = 8;
        return false;
        break;
    default:
        return false;
        break;
    }
}

/* ***********************************************************************************************************
    MEMORY ACCESS
*********************************************************************************************************** */
// read *****
u8 Memory::ReadROM_0(const u16& _addr) {
    return ROM_0[_addr];
}

u8 Memory::ReadROM_N(const u16& _addr, const int& _bank) {
    return ROM_N[_bank][_addr - ROM_BANK_N_OFFSET];
}

u8 Memory::ReadVRAM_N(const u16& _addr, const int& _bank) {
    return VRAM_N[_bank][_addr - VRAM_N_OFFSET];
}

u8 Memory::ReadRAM_N(const u16& _addr, const int& _bank) {
    return RAM_N[_bank][_addr - RAM_BANK_N_OFFSET];
}

u8 Memory::ReadWRAM_0(const u16& _addr) {
    return WRAM_0[_addr - WRAM_0_OFFSET];
}

u8 Memory::ReadWRAM_N(const u16& _addr, const int& _bank) {
    return WRAM_N[_bank][_addr - WRAM_N_OFFSET];
}

u8 Memory::ReadOAM(const u16& _addr) {
    return OAM[_addr - OAM_OFFSET];
}

u8 Memory::ReadIO(const u16& _addr) {
    return IO[_addr - IO_REGISTERS_OFFSET];
}

u8 Memory::ReadHRAM(const u16& _addr) {
    return HRAM[_addr - HRAM_OFFSET];
}

u8 Memory::ReadIE() {
    return IE;
}

// write *****
void Memory::WriteVRAM_N(const u8& _data, const u16& _addr, const int& _bank) {
    VRAM_N[_bank][_addr - VRAM_N_OFFSET] = _data;
}

void Memory::WriteRAM_N(const u8& _data, const u16& _addr, const int& _bank) {
    RAM_N[_bank][_addr - RAM_BANK_N_OFFSET] = _data;
}

void Memory::WriteWRAM_0(const u8& _data, const u16& _addr) {
    WRAM_0[_addr - WRAM_0_OFFSET] = _data;
}

void Memory::WriteWRAM_N(const u8& _data, const u16& _addr, const int& _bank) {
    WRAM_N[_bank][_addr - WRAM_N_OFFSET] = _data;
}

void Memory::WriteOAM(const u8& _data, const u16& _addr) {
    OAM[_addr - OAM_OFFSET] = _data;
}

void Memory::WriteIO(const u8& _data, const u16& _addr) {
    IO[_addr - IO_REGISTERS_OFFSET] = _data;
}

void Memory::WriteHRAM(const u8& _data, const u16& _addr) {
    HRAM[_addr - HRAM_OFFSET] = _data;
}

void Memory::WriteIE(const u8& _data) {
    IE = _data;
}