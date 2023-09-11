#include "MemorySM83.h"

#include "gameboy_config.h"
#include "logger.h"

using namespace std;

/* ***********************************************************************************************************
    REGISTER DEFINES
*********************************************************************************************************** */
// timer/divider
#define DIV                         0xFF04
#define TIMA                        0xFF05
#define TMA                         0xFF06
#define TAC                         0xFF07

/* ***********************************************************************************************************
    (CGB) REGISTER DEFINES
*********************************************************************************************************** */
// SPEED SWITCH
#define CGB_SPEED_SWITCH            0xFF4D
    
// VRAM BANK SELECT
#define CGB_VRAM_SELECT             0xFF4F

// VRAM DMA
#define CGB_HDMA1                   0xFF51
#define CGB_HDMA2                   0xFF52
#define CGB_HDMA3                   0xFF53
#define CGB_HDMA4                   0xFF54
#define CGB_HDMA5                   0xFF55
#define DMA_MODE_BIT                0x80
#define DMA_MODE_GENERAL            0x00
#define DMA_MODE_HBLANK             0x80

// OBJECT PRIORITY MODE
#define CGB_OBJ_PRIO_MODE           0xFF6C

// WRAM BANK SELECT
#define CGB_WRAM_SELECT             0xFF70


/* ***********************************************************************************************************
    CONSTRUCTOR AND (DE)INIT
*********************************************************************************************************** */
MemorySM83* MemorySM83::instance = nullptr;

MemorySM83* MemorySM83::getInstance(const Cartridge& _cart_obj) {
    if (instance != nullptr) {
        resetInstance();
    }

    instance = new MemorySM83(_cart_obj);
    return instance;
}

void MemorySM83::resetInstance() {
    if (instance != nullptr) {
        instance->CleanupMemory();

        delete instance;
        instance = nullptr;
    }
}


MemorySM83::MemorySM83(const Cartridge& _cart_obj) {
    this->isCgb = _cart_obj.GetIsCgb();

    InitMemory(_cart_obj);
}

/* ***********************************************************************************************************
    INITIALIZE MEMORY
*********************************************************************************************************** */
void MemorySM83::InitMemory(const Cartridge& _cart_obj) {
    const auto& vec_rom = _cart_obj.GetRomVector();

    if (!ReadRomHeaderInfo(vec_rom)) { return; }

    AllocateMemory();
    if (!CopyRom(vec_rom)) {
        return;
    }
}

bool MemorySM83::CopyRom(const vector<u8>& _vec_rom) {
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
void MemorySM83::AllocateMemory() {
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

    HRAM = new u8[HRAM_SIZE];
}

void MemorySM83::CleanupMemory() {
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

    delete[] HRAM;
}

/* ***********************************************************************************************************
    ROM HEADER DATA FOR MEMORY
*********************************************************************************************************** */
bool MemorySM83::ReadRomHeaderInfo(const std::vector<u8>& _vec_rom) {
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
u8 MemorySM83::ReadROM_0(const u16& _addr) {
    return ROM_0[_addr];
}

u8 MemorySM83::ReadROM_N(const u16& _addr) {
    return ROM_N[romBank][_addr - ROM_BANK_N_OFFSET];
}

u8 MemorySM83::ReadVRAM_N(const u16& _addr) {
    if (isCgb) {
        return VRAM_N[VRAM_BANK][_addr - VRAM_N_OFFSET];
    }else{
        return VRAM_N[0][_addr - VRAM_N_OFFSET];
    }
}

u8 MemorySM83::ReadRAM_N(const u16& _addr) {
    return RAM_N[ramBank][_addr - RAM_BANK_N_OFFSET];
}

u8 MemorySM83::ReadWRAM_0(const u16& _addr) {
    return WRAM_0[_addr - WRAM_0_OFFSET];
}

u8 MemorySM83::ReadWRAM_N(const u16& _addr) {
    if (isCgb) {
        return WRAM_N[WRAM_BANK][_addr - WRAM_N_OFFSET];
    }
    else {
        return WRAM_N[0][_addr - WRAM_N_OFFSET];
    }
}

u8 MemorySM83::ReadOAM(const u16& _addr) {
    return OAM[_addr - OAM_OFFSET];
}

u8 MemorySM83::ReadIO(const u16& _addr) {
    return GetIOValue(_addr);
}

u8 MemorySM83::ReadHRAM(const u16& _addr) {
    return HRAM[_addr - HRAM_OFFSET];
}

u8 MemorySM83::ReadIE() {
    return IE;
}

// write *****
void MemorySM83::WriteVRAM_N(const u8& _data, const u16& _addr) {
    if (isCgb) {
        VRAM_N[VRAM_BANK][_addr - VRAM_N_OFFSET] = _data;
    }
    else {
        VRAM_N[0][_addr - VRAM_N_OFFSET] = _data;
    }
}

void MemorySM83::WriteRAM_N(const u8& _data, const u16& _addr) {
    RAM_N[ramBank][_addr - RAM_BANK_N_OFFSET] = _data;
}

void MemorySM83::WriteWRAM_0(const u8& _data, const u16& _addr) {
    WRAM_0[_addr - WRAM_0_OFFSET] = _data;
}

void MemorySM83::WriteWRAM_N(const u8& _data, const u16& _addr) {
    if (isCgb) {
        WRAM_N[WRAM_BANK][_addr - WRAM_N_OFFSET] = _data;
    }
    else {
        WRAM_N[0][_addr - WRAM_N_OFFSET] = _data;
    }
}

void MemorySM83::WriteOAM(const u8& _data, const u16& _addr) {
    OAM[_addr - OAM_OFFSET] = _data;
}

void MemorySM83::WriteIO(const u8& _data, const u16& _addr) {
    SetIOValue(_data, _addr);
}

void MemorySM83::WriteHRAM(const u8& _data, const u16& _addr) {
    HRAM[_addr - HRAM_OFFSET] = _data;
}

void MemorySM83::WriteIE(const u8& _data) {
    IE = _data;
}

/* ***********************************************************************************************************
    IO PROCESSING
*********************************************************************************************************** */
u8 MemorySM83::GetIOValue(const u16& _addr) {
    switch (_addr) {
    case CGB_VRAM_SELECT:
        return VRAM_BANK;
        break;
    case CGB_WRAM_SELECT:
        return WRAM_BANK + 1;
        break;
    case CGB_HDMA1:
        return HDMA1;
        break;
    case CGB_HDMA2:
        return HDMA2;
        break;
    case CGB_HDMA3:
        return HDMA3;
        break;
    case CGB_HDMA4:
        return HDMA4;
        break;
    case CGB_HDMA5:
        return HDMA5;
        break;
    }
}

void MemorySM83::SetIOValue(const u8& _data, const u16& _addr) {
    switch (_addr) {
    // BANK SELECTS *****
    case CGB_VRAM_SELECT:
        VRAM_BANK = _data & 0x01;
        break;
    case CGB_WRAM_SELECT:
        WRAM_BANK = _data & 0x07;
        if (WRAM_BANK == 0) WRAM_BANK = 1;
        WRAM_BANK -= 1;
        break;
    // (CGB) VRAM DMA TRANSFERS *****
    // source high
    case CGB_HDMA1:
        HDMA1 = _data;
        break;
    // source low
    case CGB_HDMA2:
        HDMA2 = _data & 0xF0;
        break;
    // dest high
    case CGB_HDMA3:
        HDMA3 = _data & 0x1F;
        break;
    // dest low
    case CGB_HDMA4:
        HDMA4 = _data & 0xF0;
        break;
    // mode
    case CGB_HDMA5:
        HDMA5 = _data;
        if (isCgb) {
            VRAM_DMA();
        }
        break;
    }
}

/* ***********************************************************************************************************
    ROM RAM BANK NUMBER GET SET
*********************************************************************************************************** */
void MemorySM83::SetRomBank(const u8& _bank) {
    romBank = _bank - 1;
}

void MemorySM83::SetRamBank(const u8& _bank) {
    ramBank = _bank;
}

u8 MemorySM83::GetRomBank() {
    return ramBank + 1;
}

u8 MemorySM83::GetRamBank() {
    return ramBank;
}

/* ***********************************************************************************************************
    DMA
*********************************************************************************************************** */
void MemorySM83::VRAM_DMA() {
    // u8 mode = HDMA5 & DMA_MODE_BIT;
    HDMA5 |= 0x80;
    u16 length = ((HDMA5 & 0x7F) + 1) * 0x10;

    u16 source_addr = ((u16)HDMA1 << 8) | HDMA2;
    u16 dest_addr = ((u16)HDMA3 << 8) | HDMA4;

    if (source_addr < ROM_BANK_N_OFFSET) {
        memcpy(&VRAM_N[VRAM_BANK][dest_addr - VRAM_N_OFFSET], &ROM_0[source_addr], length);
    }
    else if (source_addr < VRAM_N_OFFSET) {
        memcpy(&VRAM_N[VRAM_BANK][dest_addr - VRAM_N_OFFSET], &ROM_N[romBank][source_addr - ROM_BANK_N_OFFSET], length);
    }
    else if (source_addr < WRAM_0_OFFSET && source_addr >= RAM_BANK_N_OFFSET) {
        memcpy(&VRAM_N[VRAM_BANK][dest_addr - VRAM_N_OFFSET], &RAM_N[ramBank][source_addr - RAM_BANK_N_OFFSET], length);
    }
    else if (source_addr < WRAM_N_OFFSET) {
        memcpy(&VRAM_N[VRAM_BANK][dest_addr - VRAM_N_OFFSET], &WRAM_0[source_addr - WRAM_0_OFFSET], length);
    }
    else if (source_addr < MIRROR_WRAM_OFFSET && source_addr >= WRAM_N_OFFSET) {
        memcpy(&VRAM_N[VRAM_BANK][dest_addr - VRAM_N_OFFSET], &WRAM_N[WRAM_BANK][source_addr - WRAM_N_OFFSET], length);
    }
    else {
        return;
    }

    HDMA5 = 0xFF;
}

void MemorySM83::OAM_DMA() {

}