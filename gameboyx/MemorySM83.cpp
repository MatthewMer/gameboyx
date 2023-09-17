#include "MemorySM83.h"

#include "gameboy_defines.h"
#include "logger.h"

using namespace std;

/* ***********************************************************************************************************
    CONSTRUCTOR AND (DE)INIT
*********************************************************************************************************** */
MemorySM83* MemorySM83::instance = nullptr;

MemorySM83* MemorySM83::getInstance(const Cartridge& _cart_obj) {
    if (instance == nullptr) {
        instance = new MemorySM83(_cart_obj);
    }
    return instance;
}

void MemorySM83::resetInstance() {
    if (instance != nullptr) {
        instance->CleanupMemory();
        delete instance;
        instance = nullptr;
    }
}

machine_state_context* MemorySM83::GetMachineContext() const {
    return machine_ctx;
}

/* ***********************************************************************************************************
    INITIALIZE MEMORY
*********************************************************************************************************** */
void MemorySM83::InitMemory(const Cartridge& _cart_obj) {
    const auto& vec_rom = _cart_obj.GetRomVector();

    machine_ctx->isCgb = _cart_obj.GetIsCgb();

    if (!ReadRomHeaderInfo(vec_rom)) { return; }

    AllocateMemory();
    if (!CopyRom(vec_rom)) {
        LOG_ERROR("Couldn't copy ROM");
        return;
    }

    InitTimers();
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

    VRAM_N = new u8 * [machine_ctx->isCgb ? 2 : 1];
    for (int i = 0; i < (machine_ctx->isCgb ? 2 : 1); i++) {
        VRAM_N[i] = new u8[VRAM_N_SIZE];
    }

    RAM_N = new u8 * [ramBankNum];
    for (int i = 0; i < ramBankNum; i++) {
        RAM_N[i] = new u8[RAM_BANK_N_SIZE];
    }

    WRAM_0 = new u8[WRAM_0_SIZE];
    WRAM_N = new u8 * [machine_ctx->isCgb ? 7 : 1];
    for (int i = 0; i < (machine_ctx->isCgb ? 7 : 1); i++) {
        WRAM_N[i] = new u8[WRAM_N_SIZE];
    }

    OAM = new u8[OAM_SIZE];

    HRAM = new u8[HRAM_SIZE];

    sound_ctx->WAVE_RAM = new u8[WAVE_RAM_SIZE];
}

void MemorySM83::CleanupMemory() {
    delete[] ROM_0;
    for (int i = 0; i < romBankNum; i++) {
        delete[] ROM_N[i];
    }
    delete[] ROM_N;

    for (int i = 0; i < (machine_ctx->isCgb ? 2 : 1); i++) {
        delete[] VRAM_N[i];
    }
    delete[] VRAM_N;

    for (int i = 0; i < ramBankNum; i++) {
        delete[] RAM_N[i];
    }
    delete[] RAM_N;

    delete[] WRAM_0;
    for (int i = 0; i < (machine_ctx->isCgb ? 7 : 1); i++) {
        delete[] WRAM_N[i];
    }
    delete[] WRAM_N;

    delete[] OAM;

    delete[] HRAM;

    delete[] IO;

    delete machine_ctx;
    delete graphics_ctx;
    delete[] sound_ctx->WAVE_RAM;
    delete sound_ctx;
    delete joyp_ctx;
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
    return ROM_N[machine_ctx->romBank][_addr - ROM_BANK_N_OFFSET];
}

u8 MemorySM83::ReadVRAM_N(const u16& _addr) {
    if (machine_ctx->isCgb) {
        return VRAM_N[VRAM_BANK][_addr - VRAM_N_OFFSET];
    }else{
        return VRAM_N[0][_addr - VRAM_N_OFFSET];
    }
}

u8 MemorySM83::ReadRAM_N(const u16& _addr) {
    return RAM_N[machine_ctx->ramBank][_addr - RAM_BANK_N_OFFSET];
}

u8 MemorySM83::ReadWRAM_0(const u16& _addr) {
    return WRAM_0[_addr - WRAM_0_OFFSET];
}

u8 MemorySM83::ReadWRAM_N(const u16& _addr) {
    if (machine_ctx->isCgb) {
        return WRAM_N[machine_ctx->wramBank][_addr - WRAM_N_OFFSET];
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
    return machine_ctx->IE;
}

// write *****
void MemorySM83::WriteVRAM_N(const u8& _data, const u16& _addr) {
    if (machine_ctx->isCgb) {
        VRAM_N[VRAM_BANK][_addr - VRAM_N_OFFSET] = _data;
    }
    else {
        VRAM_N[0][_addr - VRAM_N_OFFSET] = _data;
    }
}

void MemorySM83::WriteRAM_N(const u8& _data, const u16& _addr) {
    RAM_N[machine_ctx->ramBank][_addr - RAM_BANK_N_OFFSET] = _data;
}

void MemorySM83::WriteWRAM_0(const u8& _data, const u16& _addr) {
    WRAM_0[_addr - WRAM_0_OFFSET] = _data;
}

void MemorySM83::WriteWRAM_N(const u8& _data, const u16& _addr) {
    if (machine_ctx->isCgb) {
        WRAM_N[machine_ctx->wramBank][_addr - WRAM_N_OFFSET] = _data;
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
    machine_ctx->IE = _data;
}

/* ***********************************************************************************************************
    IO PROCESSING
*********************************************************************************************************** */
u8 MemorySM83::GetIOValue(const u16& _addr) {
    switch (_addr & 0x00F0) {
    case 0x00:
        // joypad, timers, IF ********************
        switch (_addr) {
        case JOYP_ADDR:
            return joyp_ctx->JOYP_P1;
            break;
        case DIV_ADDR:
            return machine_ctx->DIV;
            break;
        case TIMA_ADDR:
            return machine_ctx->TIMA;
            break;
        case TMA_ADDR:
            return machine_ctx->TMA;
            break;
        case TAC_ADDR:
            return machine_ctx->TAC;
            break;
        case IF_ADDR:
            return machine_ctx->IF;
            break;
        }
        break;
    case 0x10:
    case 0x20:
        // sound ********************
        switch (_addr) {
        case NR10_ADDR:
            return sound_ctx->NR10;
            break;
        case NR11_ADDR:
            return sound_ctx->NR11;
            break;
        case NR12_ADDR:
            return sound_ctx->NR12;
            break;
        case NR14_ADDR:
            return sound_ctx->NR14;
            break;
        case NR21_ADDR:
            return sound_ctx->NR21;
            break;
        case NR22_ADDR:
            return sound_ctx->NR22;
            break;
        case NR24_ADDR:
            return sound_ctx->NR24;
            break;
        case NR30_ADDR:
            return sound_ctx->NR30;
            break;
        case NR32_ADDR:
            return sound_ctx->NR32;
            break;
        case NR34_ADDR:
            return sound_ctx->NR34;
            break;
        case NR42_ADDR:
            return sound_ctx->NR42;
            break;
        case NR43_ADDR:
            return sound_ctx->NR43;
            break;
        case NR44_ADDR:
            return sound_ctx->NR44;
            break;
        case NR50_ADDR:
            return sound_ctx->NR50;
            break;
        case NR51_ADDR:
            return sound_ctx->NR51;
            break;
        case NR52_ADDR:
            return sound_ctx->NR52;
            break;
        }
        break;
    case 0x30:
        // WAVE RAM ********************
        return sound_ctx->WAVE_RAM[_addr - WAVE_RAM_ADDR];
        break;
    case 0x40:
        // vram, lcd ********************
        switch (_addr) {
        case CGB_VRAM_SELECT_ADDR:
            return VRAM_BANK;
            break;
        case CGB_SPEED_SWITCH_ADDR:
            return SPEEDSWITCH;
            break;
        case LCDC_ADDR:
            return graphics_ctx->LCDC;
            break;
        case STAT_ADDR:
            return graphics_ctx->STAT;
            break;
        case SCY_ADDR:
            return graphics_ctx->SCY;
            break;
        case SCX_ADDR:
            return graphics_ctx->SCX;
            break;
        case LY_ADDR:
            return graphics_ctx->LY;
            break;
        case LYC_ADDR:
            return graphics_ctx->LYC;
            break;
        case OAM_DMA_ADDR:
            return OAM_DMA_REG;
            break;
        }
        break;
    case 0x50:
        // DMA
        switch (_addr) {
        case CGB_HDMA5_ADDR:
            return HDMA5;
            break;
        }
        break;
    case 0x60:
        // color palettes, obj prio ********************
        switch (_addr) {
        case CGB_OBJ_PRIO_ADDR:
            return graphics_ctx->OBJ_PRIO;
            break;
        case BCPS_BGPI_ADDR:
            return graphics_ctx->BCPS_BGPI;
            break;
        case BCPD_BGPD_ADDR:
            return graphics_ctx->BCPD_BGPD;
            break;
        case OCPS_OBPI_ADDR:
            return graphics_ctx->OCPS_OBPI;
            break;
        case OCPD_OBPD_ADDR:
            return graphics_ctx->OCPD_OBPD;
            break;
        }
        break;
    case 0x70:
        // wram, audio out ********************
        switch (_addr) {
        case CGB_WRAM_SELECT_ADDR:
            return WRAM_BANK;
            break;
        }
        break;
    }
}

void MemorySM83::SetIOValue(const u8& _data, const u16& _addr) {
    switch (_addr & 0x00F0) {
    case 0x00:
        // joypad, timers, IF ********************
        switch (_addr) {
        case JOYP_ADDR:
            joyp_ctx->JOYP_P1 = _data;
            break;
        case DIV_ADDR:
            machine_ctx->DIV = 0x00;
            break;
        case TIMA_ADDR:
            machine_ctx->TIMA = _data;
            break;
        case TMA_ADDR:
            machine_ctx->TMA = _data;
            break;
        case TAC_ADDR:
            machine_ctx->TAC = _data & 0x07;
            ProcessTAC();
            break;
        case IF_ADDR:
            machine_ctx->IF = _data & 0x1F;
            break;
        }
        break;
    case 0x10:
    case 0x20:
        // sound ********************
        switch (_addr) {
        case NR10_ADDR:
            sound_ctx->NR10 = _data;
            break;
        case NR11_ADDR:
            sound_ctx->NR11 = _data;
            break;
        case NR12_ADDR:
            sound_ctx->NR12 = _data;
            break;
        case NR13_ADDR:
            sound_ctx->NR13 = _data;
            break;
        case NR14_ADDR:
            sound_ctx->NR14 = _data;
            break;
        case NR21_ADDR:
            sound_ctx->NR21 = _data;
            break;
        case NR22_ADDR:
            sound_ctx->NR22 = _data;
            break;
        case NR23_ADDR:
            sound_ctx->NR23 = _data;
            break;
        case NR24_ADDR:
            sound_ctx->NR24 = _data;
            break;
        case NR30_ADDR:
            sound_ctx->NR30 = _data;
            break;
        case NR31_ADDR:
            sound_ctx->NR31 = _data;
            break;
        case NR32_ADDR:
            sound_ctx->NR32 = _data;
            break;
        case NR33_ADDR:
            sound_ctx->NR33 = _data;
            break;
        case NR34_ADDR:
            sound_ctx->NR34 = _data;
            break;
        case NR41_ADDR:
            sound_ctx->NR41 = _data;
            break;
        case NR42_ADDR:
            sound_ctx->NR42 = _data;
            break;
        case NR43_ADDR:
            sound_ctx->NR43 = _data;
            break;
        case NR44_ADDR:
            sound_ctx->NR44 = _data;
            break;
        case NR50_ADDR:
            sound_ctx->NR50 = _data;
            break;
        case NR51_ADDR:
            sound_ctx->NR51 = _data;
            break;
        case NR52_ADDR:
            sound_ctx->NR52 = _data;
            break;
        }
        break;
    case 0x30:
        // WAVE RAM ********************
        sound_ctx->WAVE_RAM[_addr - WAVE_RAM_ADDR] = _data;
        break;
    case 0x40:
        // vram, lcd ********************
        switch (_addr) {
        case CGB_VRAM_SELECT_ADDR:
            VRAM_BANK = _data & 0x01;
            break;
        case CGB_SPEED_SWITCH_ADDR:
            if (machine_ctx->isCgb) {
                SwitchSpeed(_data);
            }
            break;
        case LCDC_ADDR:
            graphics_ctx->LCDC = _data;
            break;
        case STAT_ADDR:
            graphics_ctx->STAT = _data;
            break;
        case SCY_ADDR:
            graphics_ctx->SCY = _data;
            break;
        case SCX_ADDR:
            graphics_ctx->SCX = _data;
            break;
        case LYC_ADDR:
            graphics_ctx->LYC = _data;
            break;
        case OAM_DMA_ADDR:
            OAM_DMA_REG = _data;
            break;
        }
        break;
    case 0x50:
        // DMA ********************
        switch (_addr) {
            // (CGB) VRAM DMA TRANSFERS *****
            // source high
        case CGB_HDMA1_ADDR:
            HDMA1 = _data;
            break;
            // source low
        case CGB_HDMA2_ADDR:
            HDMA2 = _data & 0xF0;
            break;
            // dest high
        case CGB_HDMA3_ADDR:
            HDMA3 = _data & 0x1F;
            break;
            // dest low
        case CGB_HDMA4_ADDR:
            HDMA4 = _data & 0xF0;
            break;
            // mode
        case CGB_HDMA5_ADDR:
            HDMA5 = _data;
            if (machine_ctx->isCgb) {
                VRAM_DMA();
            }
            break;
        }
        break;
    case 0x60:
        // color palettes, obj prio ********************
        switch (_addr) {
        case CGB_OBJ_PRIO_ADDR:
            SetObjPrio(_data);
            break;
        case BCPS_BGPI_ADDR:
            graphics_ctx->BCPS_BGPI = _data;
            break;
        case BCPD_BGPD_ADDR:
            graphics_ctx->BCPD_BGPD = _data;
            break;
        case OCPS_OBPI_ADDR:
            graphics_ctx->OCPS_OBPI = _data;
            break;
        case OCPD_OBPD_ADDR:
            graphics_ctx->OCPD_OBPD = _data;
            break;
        }
        break;
    case 0x70:
        // wram, audio out ********************
        switch (_addr) {
        case CGB_WRAM_SELECT_ADDR:
            WRAM_BANK = _data & 0x07;
            if (WRAM_BANK == 0) WRAM_BANK = 1;
            machine_ctx->wramBank = WRAM_BANK - 1;
            break;
        }
        break;
    }
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
        memcpy(&VRAM_N[VRAM_BANK][dest_addr - VRAM_N_OFFSET], &ROM_N[machine_ctx->romBank][source_addr - ROM_BANK_N_OFFSET], length);
    }
    else if (source_addr < WRAM_0_OFFSET && source_addr >= RAM_BANK_N_OFFSET) {
        memcpy(&VRAM_N[VRAM_BANK][dest_addr - VRAM_N_OFFSET], &RAM_N[machine_ctx->ramBank][source_addr - RAM_BANK_N_OFFSET], length);
    }
    else if (source_addr < WRAM_N_OFFSET) {
        memcpy(&VRAM_N[VRAM_BANK][dest_addr - VRAM_N_OFFSET], &WRAM_0[source_addr - WRAM_0_OFFSET], length);
    }
    else if (source_addr < MIRROR_WRAM_OFFSET && source_addr >= WRAM_N_OFFSET) {
        memcpy(&VRAM_N[VRAM_BANK][dest_addr - VRAM_N_OFFSET], &WRAM_N[machine_ctx->wramBank][source_addr - WRAM_N_OFFSET], length);
    }
    else {
        return;
    }

    HDMA5 = 0xFF;
}

void MemorySM83::OAM_DMA() {

}

/* ***********************************************************************************************************
    TIMERS
*********************************************************************************************************** */
void MemorySM83::InitTimers() {
    machine_ctx->machineCyclesPerDIVIncrement = BASE_CLOCK_MACHINE_CYCLES * pow(10, 6) / DIV_FREQUENCY;

    ProcessTAC();
}

void MemorySM83::ProcessTAC() {
    int divisor;
    switch (machine_ctx->TAC & TAC_CLOCK_SELECT) {
    case 0x00:
        divisor = 1024;
        break;
    case 0x01:
        divisor = 16;
        break;
    case 0x02:
        divisor = 64;
        break;
    case 0x03:
        divisor = 256;
        break;
    }

    machine_ctx->machineCyclesPerTIMAIncrement = BASE_CLOCK_CPU * pow(10, 6) / divisor;
}

/* ***********************************************************************************************************
    SPEED SWITCH
*********************************************************************************************************** */
void MemorySM83::SwitchSpeed(const u8& _data) {
    if (_data & PREPARE_SPEED_SWITCH) {
        if ((_data & 0x80) ^ SPEEDSWITCH) {
            switch (_data & SPEED) {
            case NORMAL_SPEED:
                SPEEDSWITCH |= DOUBLE_SPEED;
                machine_ctx->currentSpeed = 2;
                break;
            case DOUBLE_SPEED:
                SPEEDSWITCH &= ~DOUBLE_SPEED;
                machine_ctx->currentSpeed = 1;
                break;
            }

            machine_ctx->IE = 0x00;
            // TODO: JOYP = 0x30 and STOP
        }
    }
}

/* ***********************************************************************************************************
    OBJ PRIORITY MODE
*********************************************************************************************************** */
void MemorySM83::SetObjPrio(const u8& _data) {
    switch (_data & PRIO_MODE) {
    case PRIO_OAM:
        graphics_ctx->OBJ_PRIO &= ~PRIO_COORD;
        break;
    case PRIO_COORD:
        graphics_ctx->OBJ_PRIO |= PRIO_COORD;
        break;
    }
}