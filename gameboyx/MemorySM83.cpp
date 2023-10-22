#include "MemorySM83.h"

#include "gameboy_defines.h"
#include "logger.h"
#include "format"

#include <iostream>

using namespace std;

/* ***********************************************************************************************************
    DEFINES
*********************************************************************************************************** */
#define LINE_NUM(x)     (int)((float)x / DEBUG_MEM_ELEM_PER_LINE) + ((float)(DEBUG_MEM_ELEM_PER_LINE - 1) / DEBUG_MEM_ELEM_PER_LINE)

/* ***********************************************************************************************************
    CONSTRUCTOR AND (DE)INIT
*********************************************************************************************************** */
MemorySM83* MemorySM83::instance = nullptr;

MemorySM83* MemorySM83::getInstance(machine_information& _machine_info) {
    if (instance == nullptr) {
        instance = new MemorySM83(_machine_info);
        instance->InitMemory();
    }
    return instance;
}

MemorySM83* MemorySM83::getInstance() {
    if (instance != nullptr) { return instance; }
    else { LOG_ERROR("Couldn't access memory instance"); }
}

void MemorySM83::resetInstance() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

/* ***********************************************************************************************************
    HARDWARE ACCESS
*********************************************************************************************************** */
machine_state_context* MemorySM83::GetMachineContext() {
    return &machine_ctx;
}

graphics_context* MemorySM83::GetGraphicsContext() {
    return &graphics_ctx;
}

void MemorySM83::RequestInterrupts(const u8& isr_flags) {
    IO[IF_ADDR - IO_OFFSET] |= isr_flags;
}

/* ***********************************************************************************************************
    INITIALIZE MEMORY
*********************************************************************************************************** */
void MemorySM83::InitMemory() {
    Cartridge* cart_obj = Cartridge::getInstance();

    const auto& vec_rom = cart_obj->GetRomVector();

    isCgb = cart_obj->GetIsCgb();
    machine_ctx.isCgb = isCgb;
    machine_ctx.wram_bank_num = (isCgb ? 8 : 2);
    graphics_ctx.isCgb = isCgb;
    graphics_ctx.vram_bank_num = (isCgb ? 2 : 1);

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
    vector<u8>::const_iterator start = _vec_rom.begin() + ROM_0_OFFSET;
    vector<u8>::const_iterator end = _vec_rom.begin() + ROM_0_SIZE;
    ROM_0 = vector<u8>(start, end);

    for (int i = 0; i < machine_ctx.rom_bank_num - 1; i++) {
        start = _vec_rom.begin() + ROM_N_OFFSET + i * ROM_N_SIZE;
        end = _vec_rom.begin() + ROM_N_OFFSET + i * ROM_N_SIZE + ROM_N_SIZE;

        ROM_N[i] = vector<u8>(start, end);
    }

    return true;
}

inline void setup_bank_access(ScrollableTableBuffer<memory_data>& _table_buffer, vector<u8>& _bank, const int& _offset) {
    auto data = memory_data();

    int size = _bank.size();
    int line_num = LINE_NUM(size);
    int index;

    for (int i = 0; i < line_num; i++) {
        auto table_entry = ScrollableTableEntry<memory_data>();

        index = i * DEBUG_MEM_ELEM_PER_LINE;
        get<ST_ENTRY_ADDRESS>(table_entry) = _offset + index;

        data = memory_data();
        get<MEM_ENTRY_ADDR>(data) = format("{:x}", _offset + index);
        get<MEM_ENTRY_LEN>(data) = size - index > DEBUG_MEM_ELEM_PER_LINE - 1 ? DEBUG_MEM_ELEM_PER_LINE : size % DEBUG_MEM_ELEM_PER_LINE;
        get<MEM_ENTRY_REF>(data) = &_bank[index];

        get<ST_ENTRY_DATA>(table_entry) = data;

        _table_buffer.emplace_back(table_entry);
    }
}

void MemorySM83::SetupDebugMemoryAccess() {
    // access for memory inspector
    machineInfo.memory_buffer.clear();

    auto mem_type_table_buffer = MemoryBufferEntry<ScrollableTable<memory_data>>();
    auto table = ScrollableTable<memory_data>(DEBUG_MEM_LINES);
    auto table_buffer = ScrollableTableBuffer<memory_data>();
    
    // ROM
    {
        mem_type_table_buffer = MemoryBufferEntry<ScrollableTable<memory_data>>();
        mem_type_table_buffer.first = "ROM";
        
        table = ScrollableTable<memory_data>(DEBUG_MEM_LINES);
        table_buffer = ScrollableTableBuffer<memory_data>();
        
        setup_bank_access(table_buffer, ROM_0, ROM_0_OFFSET);

        table.AddMemoryArea(table_buffer);
        mem_type_table_buffer.second.emplace_back(table);

        for (auto& n : ROM_N) {
            table = ScrollableTable<memory_data>(DEBUG_MEM_LINES);
            table_buffer = ScrollableTableBuffer<memory_data>();

            setup_bank_access(table_buffer, n, ROM_N_OFFSET);

            table.AddMemoryArea(table_buffer);
            mem_type_table_buffer.second.emplace_back(table);
        }

        machineInfo.memory_buffer.emplace_back(mem_type_table_buffer);
    }
    
    // VRAM
    {
        mem_type_table_buffer = MemoryBufferEntry<ScrollableTable<memory_data>>();
        mem_type_table_buffer.first = "VRAM";

        for (auto& n : graphics_ctx.VRAM_N) {
            table = ScrollableTable<memory_data>(DEBUG_MEM_LINES);
            table_buffer = ScrollableTableBuffer<memory_data>();

            setup_bank_access(table_buffer, n, VRAM_N_OFFSET);

            table.AddMemoryArea(table_buffer);
            mem_type_table_buffer.second.emplace_back(table);
        }

        machineInfo.memory_buffer.emplace_back(mem_type_table_buffer);
    }

    // RAM
    if (machine_ctx.ram_bank_num > 0) {
        mem_type_table_buffer = MemoryBufferEntry<ScrollableTable<memory_data>>();
        mem_type_table_buffer.first = "RAM";

        for (auto& n : RAM_N) {
            table = ScrollableTable<memory_data>(DEBUG_MEM_LINES);
            table_buffer = ScrollableTableBuffer<memory_data>();

            setup_bank_access(table_buffer, n, RAM_N_OFFSET);

            table.AddMemoryArea(table_buffer);
            mem_type_table_buffer.second.emplace_back(table);
        }

        machineInfo.memory_buffer.emplace_back(mem_type_table_buffer);
    }

    // WRAM
    {
        mem_type_table_buffer = MemoryBufferEntry<ScrollableTable<memory_data>>();
        mem_type_table_buffer.first = "WRAM";

        table = ScrollableTable<memory_data>(DEBUG_MEM_LINES);
        table_buffer = ScrollableTableBuffer<memory_data>();

        setup_bank_access(table_buffer, WRAM_0, WRAM_0_OFFSET);

        table.AddMemoryArea(table_buffer);
        mem_type_table_buffer.second.emplace_back(table);

        for (auto& n : WRAM_N) {
            table = ScrollableTable<memory_data>(DEBUG_MEM_LINES);
            table_buffer = ScrollableTableBuffer<memory_data>();

            setup_bank_access(table_buffer, n, WRAM_N_OFFSET);

            table.AddMemoryArea(table_buffer);
            mem_type_table_buffer.second.emplace_back(table);
        }

        machineInfo.memory_buffer.emplace_back(mem_type_table_buffer);
    }

    // OAM
    {
        mem_type_table_buffer = MemoryBufferEntry<ScrollableTable<memory_data>>();
        mem_type_table_buffer.first = "OAM";

        table = ScrollableTable<memory_data>(DEBUG_MEM_LINES);
        table_buffer = ScrollableTableBuffer<memory_data>();

        setup_bank_access(table_buffer, graphics_ctx.OAM, OAM_OFFSET);

        table.AddMemoryArea(table_buffer);
        mem_type_table_buffer.second.emplace_back(table);

        machineInfo.memory_buffer.emplace_back(mem_type_table_buffer);
    }

    // IO
    {
        mem_type_table_buffer = MemoryBufferEntry<ScrollableTable<memory_data>>();
        mem_type_table_buffer.first = "IO";

        table = ScrollableTable<memory_data>(DEBUG_MEM_LINES);
        table_buffer = ScrollableTableBuffer<memory_data>();

        setup_bank_access(table_buffer, IO, IO_OFFSET);

        table.AddMemoryArea(table_buffer);
        mem_type_table_buffer.second.emplace_back(table);

        machineInfo.memory_buffer.emplace_back(mem_type_table_buffer);
    }

    // HRAM
    {
        mem_type_table_buffer = MemoryBufferEntry<ScrollableTable<memory_data>>();
        mem_type_table_buffer.first = "HRAM";

        table = ScrollableTable<memory_data>(DEBUG_MEM_LINES);
        table_buffer = ScrollableTableBuffer<memory_data>();

        setup_bank_access(table_buffer, HRAM, HRAM_OFFSET);

        table.AddMemoryArea(table_buffer);
        mem_type_table_buffer.second.emplace_back(table);

        machineInfo.memory_buffer.emplace_back(mem_type_table_buffer);
    }
}

std::vector<std::pair<int, std::vector<u8>>> MemorySM83::GetProgramData() const {
    auto rom_data = vector<pair<int, vector<u8>>>(machine_ctx.rom_bank_num);

    rom_data[0] = pair(ROM_0_OFFSET, ROM_0);
    for (int i = 1; const auto & bank : ROM_N) {
        rom_data[i++] = pair(ROM_N_OFFSET, bank);
    }

    return rom_data;
}

/* ***********************************************************************************************************
    MANAGE ALLOCATED MEMORY
*********************************************************************************************************** */
void MemorySM83::AllocateMemory() {
    ROM_0 = vector<u8>(ROM_0_SIZE, 0);
    ROM_N = vector<vector<u8>>(machine_ctx.rom_bank_num - 1);
    for (int i = 0; i < machine_ctx.rom_bank_num - 1; i++) {
        ROM_N[i] = vector<u8>(ROM_N_SIZE, 0);
    }

    graphics_ctx.VRAM_N = vector<vector<u8>>(graphics_ctx.vram_bank_num);
    for (int i = 0; i < graphics_ctx.vram_bank_num; i++) {
        graphics_ctx.VRAM_N[i] = vector<u8>(VRAM_N_SIZE, 0);
    }

    if (machine_ctx.ram_bank_num > 0) {
        RAM_N = vector<vector<u8>>(machine_ctx.ram_bank_num);
        for (int i = 0; i < machine_ctx.ram_bank_num; i++) {
            RAM_N[i] = vector<u8>(RAM_N_SIZE, 0);
        }
    }

    WRAM_0 = vector<u8>(WRAM_0_SIZE, 0);
    WRAM_N = vector<vector<u8>>(machine_ctx.wram_bank_num - 1);
    for (int i = 0; i < machine_ctx.wram_bank_num - 1; i++) {
        WRAM_N[i] = vector<u8>(WRAM_N_SIZE, 0);
    }

    graphics_ctx.OAM = vector<u8>(OAM_SIZE, 0);

    IO = vector<u8>(IO_SIZE, 0);

    HRAM = vector<u8>(HRAM_SIZE, 0);
}

/* ***********************************************************************************************************
    INIT MEMORY STATE
*********************************************************************************************************** */
void MemorySM83::InitMemoryState() {
    InitTimers();
    
    machine_ctx.IE = INIT_CGB_IE;
    IO[IF_ADDR - IO_OFFSET] = INIT_CGB_IF;

    IO[BGP_ADDR - IO_OFFSET] = INIT_BGP;
    IO[OBP0_ADDR - IO_OFFSET] = INIT_OBP0;
    IO[OBP1_ADDR - IO_OFFSET] = INIT_OBP1;

    SetVRAMBank(INIT_VRAM_BANK);
    SetWRAMBank(INIT_WRAM_BANK);

    SetLCDCValues(INIT_CGB_LCDC);
    IO[STAT_ADDR - IO_OFFSET] = INIT_CGB_STAT;
    IO[LY_ADDR - IO_OFFSET] = INIT_CGB_LY;
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
        machine_ctx.rom_bank_num = total_rom_size >> 14;
    }
    else {
        return false;
    }

    // get ram info
    value = _vec_rom[ROM_HEAD_RAMSIZE];

    switch (value) {
    case 0x00:
        machine_ctx.ram_bank_num = 0;
        break;
    case 0x02:
        machine_ctx.ram_bank_num = 1;
        break;
    case 0x03:
        machine_ctx.ram_bank_num = 4;
        break;
        // not allowed
    case 0x04:
        machine_ctx.ram_bank_num = 16;
        return false;
        break;
        // not allowed
    case 0x05:
        machine_ctx.ram_bank_num = 8;
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
    return ROM_N[machine_ctx.rom_bank_selected][_addr - ROM_N_OFFSET];
}

u8 MemorySM83::ReadVRAM_N(const u16& _addr) {
    return graphics_ctx.VRAM_N[IO[CGB_VRAM_SELECT_ADDR - IO_OFFSET]][_addr - VRAM_N_OFFSET];
}

u8 MemorySM83::ReadRAM_N(const u16& _addr) {
    return RAM_N[machine_ctx.ram_bank_selected][_addr - RAM_N_OFFSET];
}

u8 MemorySM83::ReadWRAM_0(const u16& _addr) {
    return WRAM_0[_addr - WRAM_0_OFFSET];
}

u8 MemorySM83::ReadWRAM_N(const u16& _addr) {
    return WRAM_N[machine_ctx.wram_bank_selected][_addr - WRAM_N_OFFSET];
}

u8 MemorySM83::ReadOAM(const u16& _addr) {
    return graphics_ctx.OAM[_addr - OAM_OFFSET];
}

u8 MemorySM83::ReadIO(const u16& _addr) {
    return GetIORegister(_addr);
}

u8 MemorySM83::ReadHRAM(const u16& _addr) {
    return HRAM[_addr - HRAM_OFFSET];
}

u8 MemorySM83::ReadIE() {
    return machine_ctx.IE;
}

// write *****
void MemorySM83::WriteVRAM_N(const u8& _data, const u16& _addr) {
    graphics_ctx.VRAM_N[IO[CGB_VRAM_SELECT_ADDR - IO_OFFSET]][_addr - VRAM_N_OFFSET] = _data;
}

void MemorySM83::WriteRAM_N(const u8& _data, const u16& _addr) {
    RAM_N[machine_ctx.ram_bank_selected][_addr - RAM_N_OFFSET] = _data;
}

void MemorySM83::WriteWRAM_0(const u8& _data, const u16& _addr) {
    WRAM_0[_addr - WRAM_0_OFFSET] = _data;
}

void MemorySM83::WriteWRAM_N(const u8& _data, const u16& _addr) {
    WRAM_N[machine_ctx.wram_bank_selected][_addr - WRAM_N_OFFSET] = _data;
}

void MemorySM83::WriteOAM(const u8& _data, const u16& _addr) {
    graphics_ctx.OAM[_addr - OAM_OFFSET] = _data;
}

void MemorySM83::WriteIO(const u8& _data, const u16& _addr) {
    SetIORegister(_data, _addr);
}

void MemorySM83::WriteHRAM(const u8& _data, const u16& _addr) {
    HRAM[_addr - HRAM_OFFSET] = _data;
}

void MemorySM83::WriteIE(const u8& _data) {
    machine_ctx.IE = _data;
}

/* ***********************************************************************************************************
    IO PROCESSING
*********************************************************************************************************** */
u8 MemorySM83::GetIORegister(const u16& _addr) {
    // TODO: take registers with mixed access into account and further reading on return values when reading from specific locations
    switch (_addr) {
    case 0xFF03:
    case 0xFF08:
    case 0xFF09:
    case 0xFF0A:
    case 0xFF0B:
    case 0xFF0C:
    case 0xFF0D:
    case 0xFF0E:
    case 0xFF15:
    case 0xFF1F:
    case 0xFF27:
    case 0xFF28:
    case 0xFF29:
    case 0xFF2A:
    case 0xFF2B:
    case 0xFF2C:
    case 0xFF2D:
    case 0xFF2E:
    case 0xFF2F:
    case 0xFF4C:
    case 0xFF4E:
    case 0xFF50:
    case 0xFF57:
    case 0xFF58:
    case 0xFF59:
    case 0xFF5A:
    case 0xFF5B:
    case 0xFF5C:
    case 0xFF5D:
    case 0xFF5E:
    case 0xFF5F:
    case 0xFF60:
    case 0xFF61:
    case 0xFF62:
    case 0xFF63:
    case 0xFF64:
    case 0xFF65:
    case 0xFF66:
    case 0xFF67:
    case 0xFF6D:
    case 0xFF6E:
    case 0xFF6F:
    case 0xFF71:
    case 0xFF72:
    case 0xFF73:
    case 0xFF74:
    case 0xFF75:
        return 0xFF;
        break;
    default:
        if (_addr > 0xFF77) {
            return 0xFF;
        }
        else if(isCgb){
            switch (_addr) {
            case 0xFF47:
            case 0xFF48:
            case 0xFF49:
                return 0xFF;
                break;
            default:
                return IO[_addr - IO_OFFSET];
                break;
            }
        }
        else {
            switch (_addr) {
            case CGB_SPEED_SWITCH_ADDR:
            case CGB_VRAM_SELECT_ADDR:
            case CGB_HDMA1_ADDR:
            case CGB_HDMA2_ADDR:
            case CGB_HDMA3_ADDR:
            case CGB_HDMA4_ADDR:
            case CGB_HDMA5_ADDR:
            case CGB_IR_ADDR:
            case BCPS_BGPI_ADDR:
            case BCPD_BGPD_ADDR:
            case OCPS_OBPI_ADDR:
            case OCPD_OBPD_ADDR:
            case CGB_OBJ_PRIO_ADDR:
            case CGB_WRAM_SELECT_ADDR:
            case PCM12_ADDR:
            case PCM34_ADDR:
                LOG_WARN("Read ", format("{:x}", _addr), " as FF");
                return 0xFF;
                break;
            default:
                return IO[_addr - IO_OFFSET];
                break;
            }
        }
    }
}

void MemorySM83::SetIORegister(const u8& _data, const u16& _addr) {
    switch (_addr) {
    case DIV_ADDR:
        IO[DIV_ADDR - IO_OFFSET] = 0x00;
        break;
    case TAC_ADDR:
        IO[TAC_ADDR - IO_OFFSET] = _data & 0x07;
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
        IO[STAT_ADDR - IO_OFFSET] = (IO[STAT_ADDR - IO_OFFSET] & (PPU_STAT_MODE | PPU_STAT_LYC_FLAG)) | (_data & ~(PPU_STAT_MODE | PPU_STAT_LYC_FLAG));
        break;
    case OAM_DMA_ADDR:
        IO[OAM_DMA_ADDR - IO_OFFSET] = _data;
        OAM_DMA();
        break;
    case CGB_HDMA2_ADDR:
        IO[CGB_HDMA2_ADDR - IO_OFFSET] = _data & 0xF0;
        break;
        // dest high
    case CGB_HDMA3_ADDR:
        IO[CGB_HDMA3_ADDR - IO_OFFSET] = _data & 0x1F;
        break;
        // dest low
    case CGB_HDMA4_ADDR:
        IO[CGB_HDMA4_ADDR - IO_OFFSET] = _data & 0xF0;
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
        IO[_addr - IO_OFFSET] = _data;
        // TODO: remove, only for testing with blargg's instruction test rom
        if (_addr == SERIAL_DATA) {
            printf("%c", (char)IO[_addr - IO_OFFSET]);
        }
        break;
    }
}

void MemorySM83::SetIOValue(const u8& _data, const u16& _addr) {
    IO[_addr - IO_OFFSET] = _data;
}

u8 MemorySM83::GetIOValue(const u16& _addr) {
    return IO[_addr - IO_OFFSET];
}

/* ***********************************************************************************************************
    DMA
*********************************************************************************************************** */
void MemorySM83::VRAM_DMA(const u8& _data) {
    // u8 mode = HDMA5 & DMA_MODE_BIT;
    IO[CGB_HDMA5_ADDR - IO_OFFSET] = _data;
    if (isCgb) {
        IO[CGB_HDMA5_ADDR - IO_OFFSET] |= 0x80;
        u16 length = ((IO[CGB_HDMA5_ADDR - IO_OFFSET] & 0x7F) + 1) * 0x10;

        u16 source_addr = ((u16)IO[CGB_HDMA1_ADDR - IO_OFFSET] << 8) | IO[CGB_HDMA2_ADDR - IO_OFFSET];
        u16 dest_addr = ((u16)IO[CGB_HDMA3_ADDR - IO_OFFSET] << 8) | IO[CGB_HDMA4_ADDR - IO_OFFSET];

        if (source_addr < ROM_N_OFFSET) {
            memcpy(&graphics_ctx.VRAM_N[IO[CGB_VRAM_SELECT_ADDR - IO_OFFSET]][dest_addr - VRAM_N_OFFSET], &ROM_0[source_addr], length);
        }
        else if (source_addr < VRAM_N_OFFSET) {
            memcpy(&graphics_ctx.VRAM_N[IO[CGB_VRAM_SELECT_ADDR - IO_OFFSET]][dest_addr - VRAM_N_OFFSET], &ROM_N[machine_ctx.rom_bank_selected][source_addr - ROM_N_OFFSET], length);
        }
        else if (source_addr < WRAM_0_OFFSET && source_addr >= RAM_N_OFFSET) {
            memcpy(&graphics_ctx.VRAM_N[IO[CGB_VRAM_SELECT_ADDR - IO_OFFSET]][dest_addr - VRAM_N_OFFSET], &RAM_N[machine_ctx.ram_bank_selected][source_addr - RAM_N_OFFSET], length);
        }
        else if (source_addr < WRAM_N_OFFSET) {
            memcpy(&graphics_ctx.VRAM_N[IO[CGB_VRAM_SELECT_ADDR - IO_OFFSET]][dest_addr - VRAM_N_OFFSET], &WRAM_0[source_addr - WRAM_0_OFFSET], length);
        }
        else if (source_addr < MIRROR_WRAM_OFFSET) {
            memcpy(&graphics_ctx.VRAM_N[IO[CGB_VRAM_SELECT_ADDR - IO_OFFSET]][dest_addr - VRAM_N_OFFSET], &WRAM_N[machine_ctx.wram_bank_selected][source_addr - WRAM_N_OFFSET], length);
        }
        else {
            return;
        }

        IO[CGB_HDMA5_ADDR - IO_OFFSET] = 0xFF;
    }
}

void MemorySM83::OAM_DMA() {
    u16 source_address;
    if (IO[OAM_DMA_ADDR - IO_OFFSET] > OAM_DMA_SOURCE_MAX) {
        source_address = OAM_DMA_SOURCE_MAX * 0x100;
    }
    else {
        source_address = IO[OAM_DMA_ADDR - IO_OFFSET] * 0x100;
    }

    if (source_address < ROM_N_OFFSET) {
        memcpy(&graphics_ctx.OAM[0], &ROM_0[source_address], OAM_DMA_LENGTH);
    }
    else if (source_address < VRAM_N_OFFSET) {
        memcpy(&graphics_ctx.OAM[0], &ROM_N[machine_ctx.rom_bank_selected][source_address], OAM_DMA_LENGTH);
    }
    else if (source_address < RAM_N_OFFSET) {
        memcpy(&graphics_ctx.OAM[0], &graphics_ctx.VRAM_N[IO[CGB_VRAM_SELECT_ADDR - IO_OFFSET]][source_address], OAM_DMA_LENGTH);
    }
    else if (source_address < WRAM_0_OFFSET) {
        memcpy(&graphics_ctx.OAM[0], &RAM_N[machine_ctx.ram_bank_selected][source_address], OAM_DMA_LENGTH);
    }
    else if (source_address < WRAM_N_OFFSET) {
        memcpy(&graphics_ctx.OAM[0], &WRAM_0[source_address], OAM_DMA_LENGTH);
    }
    else if (source_address < MIRROR_WRAM_OFFSET) {
        memcpy(&graphics_ctx.OAM[0], &WRAM_N[machine_ctx.wram_bank_selected][source_address], OAM_DMA_LENGTH);
    }
}

/* ***********************************************************************************************************
    TIMERS
*********************************************************************************************************** */
void MemorySM83::InitTimers() {
    machine_ctx.machineCyclesPerDIVIncrement = ((BASE_CLOCK_CPU / 4) * pow(10, 6)) / DIV_FREQUENCY;

    ProcessTAC();
}

void MemorySM83::ProcessTAC() {
    int divisor;
    switch (IO[TAC_ADDR - IO_OFFSET] & TAC_CLOCK_SELECT) {
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

    machine_ctx.machineCyclesPerTIMAIncrement = BASE_CLOCK_CPU * pow(10, 6) / divisor;
}

/* ***********************************************************************************************************
    SPEED SWITCH
*********************************************************************************************************** */
void MemorySM83::SwitchSpeed(const u8& _data) {
    if (isCgb) {
        if (_data & PREPARE_SPEED_SWITCH) {
            IO[CGB_SPEED_SWITCH_ADDR - IO_OFFSET] ^= SET_SPEED;
            machine_ctx.speed_switch_requested = true;
        }
    }
}

/* ***********************************************************************************************************
    OBJ PRIORITY MODE
*********************************************************************************************************** */
void MemorySM83::SetObjPrio(const u8& _data) {
    switch (_data & PRIO_MODE) {
    case PRIO_OAM:
        IO[CGB_OBJ_PRIO_ADDR - IO_OFFSET] &= ~PRIO_COORD;
        break;
    case PRIO_COORD:
        IO[CGB_OBJ_PRIO_ADDR - IO_OFFSET] |= PRIO_COORD;
        break;
    }
}

void MemorySM83::SetLCDCValues(const u8& _data) {
    switch (_data & PPU_LCDC_WINBG_EN_PRIO) {
    case 0x00:
        graphics_ctx.bg_win_enable = false;
        break;
    case PPU_LCDC_WINBG_EN_PRIO:
        graphics_ctx.bg_win_enable = true;
        break;
    }

    switch (_data & PPU_LCDC_OBJ_ENABLE) {
    case 0x00:
        graphics_ctx.obj_enable = false;
        break;
    case PPU_LCDC_OBJ_ENABLE:
        graphics_ctx.obj_enable = true;
        break;
    }

    switch (_data & PPU_LCDC_OBJ_SIZE) {
    case 0x00:
        graphics_ctx.obj_size_16 = false;
        break;
    case PPU_LCDC_OBJ_SIZE:
        graphics_ctx.obj_size_16 = true;
        break;
    }

    switch (_data & PPU_LCDC_BG_TILEMAP) {
    case 0x00:
        graphics_ctx.bg_tilemap_offset = PPU_TILE_MAP0 - VRAM_N_OFFSET;
        break;
    case PPU_LCDC_BG_TILEMAP:
        graphics_ctx.bg_tilemap_offset = PPU_TILE_MAP1 - VRAM_N_OFFSET;
        break;
    }

    switch (_data & PPU_LCDC_WINBG_TILEDATA) {
    case 0x00:
        graphics_ctx.bg_win_8800_addr_mode = true;
        break;
    case PPU_LCDC_WINBG_TILEDATA:
        graphics_ctx.bg_win_8800_addr_mode = false;
        break;
    }

    switch (_data & PPU_LCDC_WIN_ENABLE) {
    case 0x00:
        graphics_ctx.win_enable = false;
        break;
    case PPU_LCDC_WIN_TILEMAP:
        graphics_ctx.win_enable = true;
        break;
    }

    switch (_data & PPU_LCDC_WIN_TILEMAP) {
    case 0x00:
        graphics_ctx.win_tilemap_offset = PPU_TILE_MAP0 - VRAM_N_OFFSET;
        break;
    case PPU_LCDC_WIN_TILEMAP:
        graphics_ctx.win_tilemap_offset = PPU_TILE_MAP1 - VRAM_N_OFFSET;
        break;
    }

    switch (_data & PPU_LCDC_ENABLE) {
    case 0x00:
        graphics_ctx.ppu_enable = false;
        break;
    case PPU_LCDC_ENABLE:
        graphics_ctx.ppu_enable = true;
        break;
    }

    IO[LCDC_ADDR - IO_OFFSET] = _data;
}

/* ***********************************************************************************************************
    MISC BANK SELECT
*********************************************************************************************************** */
void MemorySM83::SetVRAMBank(const u8& _data) {
    if (graphics_ctx.isCgb) {
        IO[CGB_VRAM_SELECT_ADDR - IO_OFFSET] = _data & 0x01;
    }
    else {
        IO[CGB_VRAM_SELECT_ADDR - IO_OFFSET] = 0x00;
    }
}

void MemorySM83::SetWRAMBank(const u8& _data) {
    IO[CGB_WRAM_SELECT_ADDR - IO_OFFSET] = _data & 0x07;
    if (IO[CGB_WRAM_SELECT_ADDR - IO_OFFSET] == 0) IO[CGB_WRAM_SELECT_ADDR - IO_OFFSET] = 1;
    if (isCgb) {
        machine_ctx.wram_bank_selected = IO[CGB_WRAM_SELECT_ADDR - IO_OFFSET] - 1;
    }
}