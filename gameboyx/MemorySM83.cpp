#include "MemorySM83.h"

#include "gameboy_defines.h"
#include "logger.h"
#include "format"

#include <iostream>

using namespace std;

/* ***********************************************************************************************************
    DEFINES
*********************************************************************************************************** */
#define LINE_NUM(x)     ((float)x / DEBUG_MEM_ELEM_PER_LINE) + ((float)(DEBUG_MEM_ELEM_PER_LINE - 1) / DEBUG_MEM_ELEM_PER_LINE)

/* ***********************************************************************************************************
    MISC LOOKUPS
*********************************************************************************************************** */
const vector<pair<memory_areas, string>> memory_area_map{
    {ENUM_ROM_N, "ROM"},
    {ENUM_VRAM_N, "VRAM"},
    {ENUM_RAM_N, "RAM"},
    {ENUM_WRAM_N, "WRAM"},
    {ENUM_OAM, "OAM"},
    {ENUM_IO, "IO"},
    {ENUM_HRAM, "HRAM"},
    {ENUM_IE, "IE"}
};

inline string get_memory_name(const memory_areas& _type) {
    for (const auto& [type,name] : memory_area_map) {
        if (type == _type) {
            return name;
        }
    }

    return "";
}

/* ***********************************************************************************************************
    CONSTRUCTOR AND (DE)INIT
*********************************************************************************************************** */
MemorySM83* MemorySM83::instance = nullptr;

MemorySM83* MemorySM83::getInstance(machine_information& _machine_info) {
    if (instance == nullptr) {
        instance = new MemorySM83(_machine_info);
    }
    return instance;
}

MemorySM83* MemorySM83::getInstance() {
    if (instance != nullptr) { return instance; }
}

void MemorySM83::resetInstance() {
    if (instance != nullptr) {
        instance->CleanupMemory();
        delete instance;
        instance = nullptr;
    }
}

/* ***********************************************************************************************************
    HARDWARE ACCESS
*********************************************************************************************************** */
machine_state_context* MemorySM83::GetMachineContext() const {
    return machine_ctx;
}

graphics_context* MemorySM83::GetGraphicsContext() const {
    return graphics_ctx;
}

sound_context* MemorySM83::GetSoundContext() const {
    return sound_ctx;
}

void MemorySM83::RequestInterrupts(const u8& isr_flags) {
    *machine_ctx->IF |= isr_flags;
}

/* ***********************************************************************************************************
    INITIALIZE MEMORY
*********************************************************************************************************** */
void MemorySM83::InitMemory() {
    Cartridge* cart_obj = Cartridge::getInstance();

    const auto& vec_rom = cart_obj->GetRomVector();

    machine_ctx->isCgb = cart_obj->GetIsCgb();
    machine_ctx->wram_bank_num = (machine_ctx->isCgb ? 8 : 2);
    graphics_ctx->isCgb = cart_obj->GetIsCgb();
    graphics_ctx->vram_bank_num = (graphics_ctx->isCgb ? 2 : 1);

    if (!ReadRomHeaderInfo(vec_rom)) { 
        LOG_ERROR("Couldn't acquire memory information");
        return; 
    }

    AllocateMemory();
    if (!CopyRom(vec_rom)) {
        LOG_ERROR("Couldn't copy ROM data");
        return;
    }

    InitMemoryState();

    SetupDebugMemoryAccess();
}

bool MemorySM83::CopyRom(const vector<u8>& _vec_rom) {
    if (ROM_0 != nullptr) {
        for (int i = 0; i < ROM_0_SIZE; i++) {
            ROM_0[i] = _vec_rom[i];
        }
    }
    else {
        LOG_ERROR("ROM 0 is nullptr");
        return false;
    }

    if (ROM_N != nullptr) {
        for (int i = 0; i < machine_ctx->rom_bank_num - 1; i++) {
            if (ROM_N[i] != nullptr) {
                for (int j = 0; j < ROM_N_SIZE; j++) {
                    ROM_N[i][j] = _vec_rom[ROM_N_OFFSET + i * ROM_N_SIZE + j];
                }
            }
            else {
                LOG_ERROR("ROM ", i + 1, " is nullptr");
                return false;
            }
        }

        return true;
    }
    else {
        return false;
    }
}

void MemorySM83::SetupDebugMemoryAccess() {
    // references to memory areas
    machineInfo.memory_access.clear();

    machineInfo.memory_access.emplace_back(ENUM_ROM_N, 1, ROM_0_SIZE, ROM_0_OFFSET, &ROM_0);
    machineInfo.memory_access.emplace_back(ENUM_ROM_N, machine_ctx->rom_bank_num - 1, ROM_N_SIZE, ROM_N_OFFSET, ROM_N);

    machineInfo.memory_access.emplace_back(ENUM_VRAM_N, graphics_ctx->vram_bank_num, VRAM_N_SIZE, VRAM_N_OFFSET, graphics_ctx->VRAM_N);

    if (machine_ctx->ram_bank_num > 0) {
        machineInfo.memory_access.emplace_back(ENUM_RAM_N, machine_ctx->ram_bank_num, RAM_N_SIZE, RAM_N_OFFSET, RAM_N);
    }

    machineInfo.memory_access.emplace_back(ENUM_WRAM_N, 1, WRAM_0_SIZE, WRAM_0_OFFSET, &WRAM_0);
    machineInfo.memory_access.emplace_back(ENUM_WRAM_N, machine_ctx->wram_bank_num - 1, WRAM_N_SIZE, WRAM_N_OFFSET, WRAM_N);

    machineInfo.memory_access.emplace_back(ENUM_OAM, 1, OAM_SIZE, OAM_OFFSET, &graphics_ctx->OAM);

    machineInfo.memory_access.emplace_back(ENUM_IO, 1, IO_REGISTERS_SIZE, IO_REGISTERS_OFFSET, &IO);

    machineInfo.memory_access.emplace_back(ENUM_HRAM, 1, HRAM_SIZE, HRAM_OFFSET, &HRAM);

    // access for memory inspector
    machineInfo.memory_buffer.clear();

    int line_num;
    bool entry_found;
    for (int i = 0, mem_type = 0; mem_type < machineInfo.memory_access.size(); mem_type++) {
        MemoryBufferEntry<ScrollableTable<memory_data>> entry = MemoryBufferEntry<ScrollableTable<memory_data>>();
        entry.first = get_memory_name(static_cast<memory_areas>(mem_type));

        entry_found = false;
        for (auto& [type, num, size, offset, ref] : machineInfo.memory_access) {
            if (mem_type == type) {
                for (int j = 0; j < num; j++) {
                    line_num = LINE_NUM(size);
                    ScrollableTable<memory_data> table = ScrollableTable<memory_data>(line_num < DEBUG_MEM_LINES ? line_num : DEBUG_MEM_LINES);
                    auto table_buffer = ScrollableTableBuffer<memory_data>();
                    get<ST_BUF_SIZE>(table_buffer) = line_num;

                    int index;
                    for (int k = 0; k < line_num; k++) {
                        ScrollableTableEntry<memory_data> buffer_entry = ScrollableTableEntry<memory_data>();

                        index = k * DEBUG_MEM_ELEM_PER_LINE;
                        get<ST_ENTRY_ADDRESS>(buffer_entry) = offset + index;

                        get<MEM_ENTRY_ADDR>(get<ST_ENTRY_DATA>(buffer_entry)) = format("{:x}", offset + index);
                        get<MEM_ENTRY_LEN>(get<ST_ENTRY_DATA>(buffer_entry)) = 
                            size - index > DEBUG_MEM_ELEM_PER_LINE - 1 ? DEBUG_MEM_ELEM_PER_LINE : size % DEBUG_MEM_ELEM_PER_LINE;
                        get<MEM_ENTRY_REF>(get<ST_ENTRY_DATA>(buffer_entry)) = &ref[j][index];

                        get<ST_BUF_BUFFER>(table_buffer).emplace_back(buffer_entry);
                    }

                    table.AddMemoryArea(table_buffer);
                    entry.second.emplace_back(table);
                }

                entry_found = true;
                i++;
            }
        }

        if (entry_found) { machineInfo.memory_buffer.emplace_back(entry); }
    }
}



/* ***********************************************************************************************************
    MANAGE ALLOCATED MEMORY
*********************************************************************************************************** */
void MemorySM83::AllocateMemory() {
    ROM_0 = new u8[ROM_0_SIZE];
    ROM_N = new u8 * [machine_ctx->rom_bank_num - 1];
    for (int i = 0; i < machine_ctx->rom_bank_num - 1; i++) {

        ROM_N[i] = new u8[ROM_N_SIZE];
    }

    graphics_ctx->VRAM_N = new u8 * [graphics_ctx->vram_bank_num];
    for (int i = 0; i < graphics_ctx->vram_bank_num; i++) {
        graphics_ctx->VRAM_N[i] = new u8[VRAM_N_SIZE];
    }

    RAM_N = new u8 * [machine_ctx->ram_bank_num];
    for (int i = 0; i < machine_ctx->ram_bank_num; i++) {
        RAM_N[i] = new u8[RAM_N_SIZE];
    }

    WRAM_0 = new u8[WRAM_0_SIZE];
    WRAM_N = new u8 * [machine_ctx->wram_bank_num - 1];
    for (int i = 0; i < machine_ctx->wram_bank_num - 1; i++) {
        WRAM_N[i] = new u8[WRAM_N_SIZE];
    }

    graphics_ctx->OAM = new u8[OAM_SIZE];

    HRAM = new u8[HRAM_SIZE];

    SetIOReferences();
}

void MemorySM83::SetIOReferences() {
    IO = new u8[IO_REGISTERS_SIZE];

    // joypad
    joyp_ctx->JOYP_P1 = &IO[JOYP_ADDR - IO_REGISTERS_OFFSET];

    // serial IO
    serial_ctx->SB = &IO[SERIAL_DATA - IO_REGISTERS_OFFSET];
    serial_ctx->SC = &IO[SERIAL_CTRL - IO_REGISTERS_OFFSET];

    // timer/divider
    machine_ctx->DIV = &IO[DIV_ADDR - IO_REGISTERS_OFFSET];
    machine_ctx->TIMA = &IO[TIMA_ADDR - IO_REGISTERS_OFFSET];
    machine_ctx->TMA = &IO[TMA_ADDR - IO_REGISTERS_OFFSET];
    machine_ctx->TAC = &IO[TAC_ADDR - IO_REGISTERS_OFFSET];

    // interrupt request
    machine_ctx->IF = &IO[IF_ADDR - IO_REGISTERS_OFFSET];

    // sound
    sound_ctx->NR10 = &IO[NR10_ADDR - IO_REGISTERS_OFFSET];
    sound_ctx->NR11 = &IO[NR11_ADDR - IO_REGISTERS_OFFSET];
    sound_ctx->NR12 = &IO[NR12_ADDR - IO_REGISTERS_OFFSET];
    sound_ctx->NR13 = &IO[NR13_ADDR - IO_REGISTERS_OFFSET];
    sound_ctx->NR14 = &IO[NR14_ADDR - IO_REGISTERS_OFFSET];

    sound_ctx->NR21 = &IO[NR21_ADDR - IO_REGISTERS_OFFSET];
    sound_ctx->NR22 = &IO[NR22_ADDR - IO_REGISTERS_OFFSET];
    sound_ctx->NR23 = &IO[NR23_ADDR - IO_REGISTERS_OFFSET];
    sound_ctx->NR24 = &IO[NR24_ADDR - IO_REGISTERS_OFFSET];

    sound_ctx->NR30 = &IO[NR30_ADDR - IO_REGISTERS_OFFSET];
    sound_ctx->NR31 = &IO[NR31_ADDR - IO_REGISTERS_OFFSET];
    sound_ctx->NR32 = &IO[NR32_ADDR - IO_REGISTERS_OFFSET];
    sound_ctx->NR33 = &IO[NR33_ADDR - IO_REGISTERS_OFFSET];
    sound_ctx->NR34 = &IO[NR34_ADDR - IO_REGISTERS_OFFSET];

    sound_ctx->NR41 = &IO[NR41_ADDR - IO_REGISTERS_OFFSET];
    sound_ctx->NR42 = &IO[NR42_ADDR - IO_REGISTERS_OFFSET];
    sound_ctx->NR43 = &IO[NR43_ADDR - IO_REGISTERS_OFFSET];
    sound_ctx->NR44 = &IO[NR44_ADDR - IO_REGISTERS_OFFSET];

    sound_ctx->NR50 = &IO[NR50_ADDR - IO_REGISTERS_OFFSET];
    sound_ctx->NR51 = &IO[NR51_ADDR - IO_REGISTERS_OFFSET];
    sound_ctx->NR52 = &IO[NR52_ADDR - IO_REGISTERS_OFFSET];

    sound_ctx->WAVE_RAM = &IO[WAVE_RAM_ADDR - IO_REGISTERS_OFFSET];

    // graphics
    graphics_ctx->LCDC = &IO[LCDC_ADDR - IO_REGISTERS_OFFSET];
    graphics_ctx->STAT = &IO[STAT_ADDR - IO_REGISTERS_OFFSET];
    graphics_ctx->SCY = &IO[SCY_ADDR - IO_REGISTERS_OFFSET];
    graphics_ctx->SCX = &IO[SCX_ADDR - IO_REGISTERS_OFFSET];
    graphics_ctx->LY = &IO[LY_ADDR - IO_REGISTERS_OFFSET];
    graphics_ctx->LYC = &IO[LYC_ADDR - IO_REGISTERS_OFFSET];

    OAM_DMA_REG = &IO[OAM_DMA_ADDR - IO_REGISTERS_OFFSET];

    graphics_ctx->BGP = &IO[BGP_ADDR - IO_REGISTERS_OFFSET];
    graphics_ctx->OBP0 = &IO[OBP0_ADDR - IO_REGISTERS_OFFSET];
    graphics_ctx->OBP1 = &IO[OBP1_ADDR - IO_REGISTERS_OFFSET];

    graphics_ctx->WY = &IO[WY_ADDR - IO_REGISTERS_OFFSET];
    graphics_ctx->WX = &IO[WX_ADDR - IO_REGISTERS_OFFSET];

    // speed switch
    SPEEDSWITCH = &IO[CGB_SPEED_SWITCH_ADDR - IO_REGISTERS_OFFSET];

    // vram
    graphics_ctx->VRAM_BANK = &IO[CGB_VRAM_SELECT_ADDR - IO_REGISTERS_OFFSET];
    HDMA1 = &IO[CGB_HDMA1_ADDR - IO_REGISTERS_OFFSET];
    HDMA2 = &IO[CGB_HDMA2_ADDR - IO_REGISTERS_OFFSET];
    HDMA3 = &IO[CGB_HDMA3_ADDR - IO_REGISTERS_OFFSET];
    HDMA4 = &IO[CGB_HDMA4_ADDR - IO_REGISTERS_OFFSET];
    HDMA5 = &IO[CGB_HDMA5_ADDR - IO_REGISTERS_OFFSET];

    graphics_ctx->BCPS_BGPI = &IO[BCPS_BGPI_ADDR - IO_REGISTERS_OFFSET];
    graphics_ctx->BCPD_BGPD = &IO[BCPD_BGPD_ADDR - IO_REGISTERS_OFFSET];
    graphics_ctx->OCPS_OBPI = &IO[OCPS_OBPI_ADDR - IO_REGISTERS_OFFSET];
    graphics_ctx->OCPD_OBPD = &IO[OCPD_OBPD_ADDR - IO_REGISTERS_OFFSET];
    graphics_ctx->OBJ_PRIO = &IO[CGB_OBJ_PRIO_ADDR - IO_REGISTERS_OFFSET];

    WRAM_BANK = &IO[CGB_WRAM_SELECT_ADDR - IO_REGISTERS_OFFSET];
}

void MemorySM83::CleanupMemory() {
    // delete ROM
    delete[] ROM_0;
    for (int i = 0; i < machine_ctx->rom_bank_num - 1; i++) {
        delete[] ROM_N[i];
    }
    delete[] ROM_N;

    // delete VRAM
    for (int i = 0; i < graphics_ctx->vram_bank_num; i++) {
        delete[] graphics_ctx->VRAM_N[i];
    }
    delete[] graphics_ctx->VRAM_N;

    // delete SRAM
    for (int i = 0; i < machine_ctx->ram_bank_num; i++) {
        delete[] RAM_N[i];
    }
    delete[] RAM_N;

    // delete WRAM
    delete[] WRAM_0;
    for (int i = 0; i < machine_ctx->wram_bank_num - 1; i++) {
        delete[] WRAM_N[i];
    }
    delete[] WRAM_N;

    // delete OAM
    delete[] graphics_ctx->OAM;

    // delete HRAM
    delete[] HRAM;

    // delete context for virtual hardware (mapped IO Registers)
    delete machine_ctx;
    delete graphics_ctx;
    delete sound_ctx;
    delete joyp_ctx;
    delete serial_ctx;

    delete[] IO;
}

/* ***********************************************************************************************************
    INIT MEMORY STATE
*********************************************************************************************************** */
void MemorySM83::InitMemoryState() {
    InitTimers();
    
    machine_ctx->IE = INIT_CGB_IE;
    *machine_ctx->IF = INIT_CGB_IF;

    *graphics_ctx->BGP = INIT_BGP;
    *graphics_ctx->OBP0 = INIT_OBP0;
    *graphics_ctx->OBP1 = INIT_OBP1;

    SetVRAMBank(INIT_VRAM_BANK);
    SetWRAMBank(INIT_WRAM_BANK);

    SetLCDCValues(INIT_CGB_LCDC);
    *graphics_ctx->STAT = INIT_CGB_STAT;
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
        machine_ctx->rom_bank_num = total_rom_size >> 14;
    }
    else {
        return false;
    }

    // get ram info
    value = _vec_rom[ROM_HEAD_RAMSIZE];

    switch (value) {
    case 0x00:
        machine_ctx->ram_bank_num = 0;
        break;
    case 0x02:
        machine_ctx->ram_bank_num = 1;
        break;
    case 0x03:
        machine_ctx->ram_bank_num = 4;
        break;
        // not allowed
    case 0x04:
        machine_ctx->ram_bank_num = 16;
        return false;
        break;
        // not allowed
    case 0x05:
        machine_ctx->ram_bank_num = 8;
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
    return ROM_N[machine_ctx->rom_bank_selected][_addr - ROM_N_OFFSET];
}

u8 MemorySM83::ReadVRAM_N(const u16& _addr) {
    return graphics_ctx->VRAM_N[*graphics_ctx->VRAM_BANK][_addr - VRAM_N_OFFSET];
}

u8 MemorySM83::ReadRAM_N(const u16& _addr) {
    return RAM_N[machine_ctx->ram_bank_selected][_addr - RAM_N_OFFSET];
}

u8 MemorySM83::ReadWRAM_0(const u16& _addr) {
    return WRAM_0[_addr - WRAM_0_OFFSET];
}

u8 MemorySM83::ReadWRAM_N(const u16& _addr) {
    return WRAM_N[machine_ctx->wram_bank_selected][_addr - WRAM_N_OFFSET];
}

u8 MemorySM83::ReadOAM(const u16& _addr) {
    return graphics_ctx->OAM[_addr - OAM_OFFSET];
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
    graphics_ctx->VRAM_N[*graphics_ctx->VRAM_BANK][_addr - VRAM_N_OFFSET] = _data;
}

void MemorySM83::WriteRAM_N(const u8& _data, const u16& _addr) {
    RAM_N[machine_ctx->ram_bank_selected][_addr - RAM_N_OFFSET] = _data;
}

void MemorySM83::WriteWRAM_0(const u8& _data, const u16& _addr) {
    WRAM_0[_addr - WRAM_0_OFFSET] = _data;
}

void MemorySM83::WriteWRAM_N(const u8& _data, const u16& _addr) {
    WRAM_N[machine_ctx->wram_bank_selected][_addr - WRAM_N_OFFSET] = _data;
}

void MemorySM83::WriteOAM(const u8& _data, const u16& _addr) {
    graphics_ctx->OAM[_addr - OAM_OFFSET] = _data;
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
    switch (_addr) {
    default:
        return IO[_addr - IO_REGISTERS_OFFSET];
        break;
    }
}

void MemorySM83::SetIOValue(const u8& _data, const u16& _addr) {
    switch (_addr) {
    case DIV_ADDR:
        *machine_ctx->DIV = 0x00;
        break;
    case TAC_ADDR:
        *machine_ctx->TAC = _data & 0x07;
        break;
    case IF_ADDR:
        *machine_ctx->IF = _data & 0x1F;
        break;
    case CGB_VRAM_SELECT_ADDR:
        SetVRAMBank(_data);
        break;
    case CGB_SPEED_SWITCH_ADDR:
        SwitchSpeed(_data);
    case LCDC_ADDR:
        SetLCDCValues(_data);
        break;
    case STAT_ADDR:
        *graphics_ctx->STAT = (*graphics_ctx->STAT & (PPU_STAT_MODE | PPU_STAT_LYC_FLAG)) | (_data & ~(PPU_STAT_MODE | PPU_STAT_LYC_FLAG));
        break;
    case OAM_DMA_ADDR:
        *OAM_DMA_REG = _data;
        OAM_DMA();
        break;
    case CGB_HDMA2_ADDR:
        *HDMA2 = _data & 0xF0;
        break;
        // dest high
    case CGB_HDMA3_ADDR:
        *HDMA3 = _data & 0x1F;
        break;
        // dest low
    case CGB_HDMA4_ADDR:
        *HDMA4 = _data & 0xF0;
        break;
        // mode
    case CGB_HDMA5_ADDR:
        VRAM_DMA(_data);
        break;
    case CGB_OBJ_PRIO_ADDR:
        SetObjPrio(_data);
        break;
    case CGB_WRAM_SELECT_ADDR:
        SetWRAMBank(_data);
        break;
    default:
        IO[_addr - IO_REGISTERS_OFFSET] = _data;
        break;
    }
}

/* ***********************************************************************************************************
    DMA
*********************************************************************************************************** */
void MemorySM83::VRAM_DMA(const u8& _data) {
    // u8 mode = HDMA5 & DMA_MODE_BIT;
    *HDMA5 = _data;
    if (machine_ctx->isCgb) {
        *HDMA5 |= 0x80;
        u16 length = ((*HDMA5 & 0x7F) + 1) * 0x10;

        u16 source_addr = ((u16)*HDMA1 << 8) | *HDMA2;
        u16 dest_addr = ((u16)*HDMA3 << 8) | *HDMA4;

        if (source_addr < ROM_N_OFFSET) {
            memcpy(&graphics_ctx->VRAM_N[*graphics_ctx->VRAM_BANK][dest_addr - VRAM_N_OFFSET], &ROM_0[source_addr], length);
        }
        else if (source_addr < VRAM_N_OFFSET) {
            memcpy(&graphics_ctx->VRAM_N[*graphics_ctx->VRAM_BANK][dest_addr - VRAM_N_OFFSET], &ROM_N[machine_ctx->rom_bank_selected][source_addr - ROM_N_OFFSET], length);
        }
        else if (source_addr < WRAM_0_OFFSET && source_addr >= RAM_N_OFFSET) {
            memcpy(&graphics_ctx->VRAM_N[*graphics_ctx->VRAM_BANK][dest_addr - VRAM_N_OFFSET], &RAM_N[machine_ctx->ram_bank_selected][source_addr - RAM_N_OFFSET], length);
        }
        else if (source_addr < WRAM_N_OFFSET) {
            memcpy(&graphics_ctx->VRAM_N[*graphics_ctx->VRAM_BANK][dest_addr - VRAM_N_OFFSET], &WRAM_0[source_addr - WRAM_0_OFFSET], length);
        }
        else if (source_addr < MIRROR_WRAM_OFFSET) {
            memcpy(&graphics_ctx->VRAM_N[*graphics_ctx->VRAM_BANK][dest_addr - VRAM_N_OFFSET], &WRAM_N[machine_ctx->wram_bank_selected][source_addr - WRAM_N_OFFSET], length);
        }
        else {
            return;
        }

        *HDMA5 = 0xFF;
    }
}

void MemorySM83::OAM_DMA() {
    u16 source_address;
    if (*OAM_DMA_REG > OAM_DMA_SOURCE_MAX) {
        source_address = OAM_DMA_SOURCE_MAX * 0x100;
    }
    else {
        source_address = *OAM_DMA_REG * 0x100;
    }

    if (source_address < ROM_N_OFFSET) {
        memcpy(graphics_ctx->OAM, &ROM_0[source_address], OAM_DMA_LENGTH);
    }
    else if (source_address < VRAM_N_OFFSET) {
        memcpy(graphics_ctx->OAM, &ROM_N[machine_ctx->rom_bank_selected][source_address], OAM_DMA_LENGTH);
    }
    else if (source_address < RAM_N_OFFSET) {
        memcpy(graphics_ctx->OAM, &graphics_ctx->VRAM_N[*graphics_ctx->VRAM_BANK][source_address], OAM_DMA_LENGTH);
    }
    else if (source_address < WRAM_0_OFFSET) {
        memcpy(graphics_ctx->OAM, &RAM_N[machine_ctx->ram_bank_selected][source_address], OAM_DMA_LENGTH);
    }
    else if (source_address < WRAM_N_OFFSET) {
        memcpy(graphics_ctx->OAM, &WRAM_0[source_address], OAM_DMA_LENGTH);
    }
    else if (source_address < MIRROR_WRAM_OFFSET) {
        memcpy(graphics_ctx->OAM, &WRAM_N[machine_ctx->wram_bank_selected][source_address], OAM_DMA_LENGTH);
    }
}

/* ***********************************************************************************************************
    TIMERS
*********************************************************************************************************** */
void MemorySM83::InitTimers() {
    machine_ctx->machineCyclesPerDIVIncrement = ((BASE_CLOCK_CPU / 4) * pow(10, 6)) / DIV_FREQUENCY;

    ProcessTAC();
}

void MemorySM83::ProcessTAC() {
    int divisor;
    switch (*machine_ctx->TAC & TAC_CLOCK_SELECT) {
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
    if (machine_ctx->isCgb) {
        if (_data & PREPARE_SPEED_SWITCH) {
            if ((_data & 0x80) ^ *SPEEDSWITCH) {
                switch (_data & SPEED) {
                case NORMAL_SPEED:
                    *SPEEDSWITCH |= DOUBLE_SPEED;
                    machine_ctx->currentSpeed = 2;
                    break;
                case DOUBLE_SPEED:
                    *SPEEDSWITCH &= ~DOUBLE_SPEED;
                    machine_ctx->currentSpeed = 1;
                    break;
                }

                machine_ctx->IE = 0x00;
                *joyp_ctx->JOYP_P1 = 0x30;
                // TODO: STOP
            }
        }
    }
}

/* ***********************************************************************************************************
    OBJ PRIORITY MODE
*********************************************************************************************************** */
void MemorySM83::SetObjPrio(const u8& _data) {
    switch (_data & PRIO_MODE) {
    case PRIO_OAM:
        *graphics_ctx->OBJ_PRIO &= ~PRIO_COORD;
        break;
    case PRIO_COORD:
        *graphics_ctx->OBJ_PRIO |= PRIO_COORD;
        break;
    }
}

void MemorySM83::SetLCDCValues(const u8& _data) {
    switch (_data & PPU_LCDC_WINBG_EN_PRIO) {
    case 0x00:
        graphics_ctx->bg_win_enable = false;
        break;
    case PPU_LCDC_WINBG_EN_PRIO:
        graphics_ctx->bg_win_enable = true;
        break;
    }

    switch (_data & PPU_LCDC_OBJ_ENABLE) {
    case 0x00:
        graphics_ctx->obj_enable = false;
        break;
    case PPU_LCDC_OBJ_ENABLE:
        graphics_ctx->obj_enable = true;
        break;
    }

    switch (_data & PPU_LCDC_OBJ_SIZE) {
    case 0x00:
        graphics_ctx->obj_size_16 = false;
        break;
    case PPU_LCDC_OBJ_SIZE:
        graphics_ctx->obj_size_16 = true;
        break;
    }

    switch (_data & PPU_LCDC_BG_TILEMAP) {
    case 0x00:
        graphics_ctx->bg_tilemap_offset = PPU_TILE_MAP0 - VRAM_N_OFFSET;
        break;
    case PPU_LCDC_BG_TILEMAP:
        graphics_ctx->bg_tilemap_offset = PPU_TILE_MAP1 - VRAM_N_OFFSET;
        break;
    }

    switch (_data & PPU_LCDC_WINBG_TILEDATA) {
    case 0x00:
        graphics_ctx->bg_win_8800_addr_mode = true;
        break;
    case PPU_LCDC_WINBG_TILEDATA:
        graphics_ctx->bg_win_8800_addr_mode = false;
        break;
    }

    switch (_data & PPU_LCDC_WIN_ENABLE) {
    case 0x00:
        graphics_ctx->win_enable = false;
        break;
    case PPU_LCDC_WIN_TILEMAP:
        graphics_ctx->win_enable = true;
        break;
    }

    switch (_data & PPU_LCDC_WIN_TILEMAP) {
    case 0x00:
        graphics_ctx->win_tilemap_offset = PPU_TILE_MAP0 - VRAM_N_OFFSET;
        break;
    case PPU_LCDC_WIN_TILEMAP:
        graphics_ctx->win_tilemap_offset = PPU_TILE_MAP1 - VRAM_N_OFFSET;
        break;
    }

    switch (_data & PPU_LCDC_ENABLE) {
    case 0x00:
        graphics_ctx->ppu_enable = false;
        break;
    case PPU_LCDC_ENABLE:
        graphics_ctx->ppu_enable = true;
        break;
    }

    *graphics_ctx->LCDC = _data;
}

/* ***********************************************************************************************************
    MISC BANK SELECT
*********************************************************************************************************** */
void MemorySM83::SetVRAMBank(const u8& _data) {
    if (graphics_ctx->isCgb) {
        *graphics_ctx->VRAM_BANK = _data & 0x01;
    }
    else {
        *graphics_ctx->VRAM_BANK = 0x00;
    }
}

void MemorySM83::SetWRAMBank(const u8& _data) {
    *WRAM_BANK = _data & 0x07;
    if (*WRAM_BANK == 0) *WRAM_BANK = 1;
    if (machine_ctx->isCgb) {
        machine_ctx->wram_bank_selected = *WRAM_BANK - 1;
    }
}