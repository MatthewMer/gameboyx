#include "GameboyMEM.h"

#include "gameboy_defines.h"
#include "logger.h"
#include "format"

#include <iostream>

using namespace std;

/* ***********************************************************************************************************
    DEFINES
*********************************************************************************************************** */
#define LINE_NUM(x)     (((float)x / DEBUG_MEM_ELEM_PER_LINE) + ((float)(DEBUG_MEM_ELEM_PER_LINE - 1) / DEBUG_MEM_ELEM_PER_LINE))

/* ***********************************************************************************************************
    CONSTRUCTOR AND (DE)INIT
*********************************************************************************************************** */
GameboyMEM* GameboyMEM::instance = nullptr;

GameboyMEM* GameboyMEM::getInstance(machine_information& _machine_info) {
    if (instance == nullptr) {
        instance = new GameboyMEM(_machine_info);
        instance->InitMemory();
    }
    return instance;
}

GameboyMEM* GameboyMEM::getInstance() {
    if (instance != nullptr) { return instance; }
    else { LOG_ERROR("Couldn't access memory instance"); }
}

void GameboyMEM::resetInstance() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

/* ***********************************************************************************************************
    HARDWARE ACCESS
*********************************************************************************************************** */
machine_context* GameboyMEM::GetMachineContext() {
    return &machine_ctx;
}

graphics_context* GameboyMEM::GetGraphicsContext() {
    return &graphics_ctx;
}

sound_context* GameboyMEM::GetSoundContext() {
    return &sound_ctx;
}

control_context* GameboyMEM::GetControlContext() {
    return &control_ctx;
}

void GameboyMEM::RequestInterrupts(const u8& _isr_flags) {
    IO[IF_ADDR - IO_OFFSET] |= _isr_flags;
}

/* ***********************************************************************************************************
    INITIALIZE MEMORY
*********************************************************************************************************** */
void GameboyMEM::InitMemory() {
    const auto& vec_rom = machineInfo.cartridge->GetRomVector();

    machine_ctx.isCgb = vec_rom[ROM_HEAD_CGBFLAG] & 0x80 ? true : false;
    machine_ctx.wram_bank_num = (machine_ctx.isCgb ? 8 : 2);
    machine_ctx.vram_bank_num = (machine_ctx.isCgb ? 2 : 1);

    if (!ReadRomHeaderInfo(vec_rom)) { 
        LOG_ERROR("Couldn't acquire memory information");
        return; 
    }

    AllocateMemory();

    if (!CopyRom(vec_rom)) {
        LOG_ERROR("Couldn't copy ROM data");
        return;
    } else {
        LOG_INFO("[emu] ROM copied");
    }
    InitMemoryState();

    SetupDebugMemoryAccess();
}

bool GameboyMEM::CopyRom(const vector<u8>& _vec_rom) {
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

/* ***********************************************************************************************************
    MANAGE ALLOCATED MEMORY
*********************************************************************************************************** */
void GameboyMEM::AllocateMemory() {
    ROM_0 = vector<u8>(ROM_0_SIZE, 0);
    ROM_N = vector<vector<u8>>(machine_ctx.rom_bank_num - 1);
    for (int i = 0; i < machine_ctx.rom_bank_num - 1; i++) {
        ROM_N[i] = vector<u8>(ROM_N_SIZE, 0);
    }

    graphics_ctx.VRAM_N = vector<vector<u8>>(machine_ctx.vram_bank_num);
    for (int i = 0; i < machine_ctx.vram_bank_num; i++) {
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
void GameboyMEM::InitMemoryState() {   
    machine_ctx.IE = INIT_CGB_IE;
    IO[IF_ADDR - IO_OFFSET] = INIT_CGB_IF;

    IO[BGP_ADDR - IO_OFFSET] = INIT_BGP;
    IO[OBP0_ADDR - IO_OFFSET] = INIT_OBP0;
    IO[OBP1_ADDR - IO_OFFSET] = INIT_OBP1;

    SetVRAMBank(INIT_VRAM_BANK);
    SetWRAMBank(INIT_WRAM_BANK);

    IO[STAT_ADDR - IO_OFFSET] = INIT_CGB_STAT;
    IO[LY_ADDR - IO_OFFSET] = INIT_CGB_LY;
    SetLCDCValues(INIT_CGB_LCDC);
    SetLCDSTATValues(INIT_CGB_STAT);    

    if (machine_ctx.isCgb) {
        IO[DIV_ADDR - IO_OFFSET] = INIT_CGB_DIV >> 8; 
        machine_ctx.div_low_byte = INIT_CGB_DIV & 0xFF;
    }
    else { 
        IO[DIV_ADDR - IO_OFFSET] = INIT_DIV >> 8; 
        machine_ctx.div_low_byte = INIT_DIV & 0xFF;
    }

    IO[TIMA_ADDR - IO_OFFSET] = INIT_TIMA;
    IO[TMA_ADDR - IO_OFFSET] = INIT_TMA;
    IO[TAC_ADDR - IO_OFFSET] = INIT_TAC;
    SetControlValues(0xFF);
    ProcessTAC();
}

/* ***********************************************************************************************************
    ROM HEADER DATA FOR MEMORY
*********************************************************************************************************** */
bool GameboyMEM::ReadRomHeaderInfo(const std::vector<u8>& _vec_rom) {
    if (_vec_rom.size() < ROM_HEAD_ADDR + ROM_HEAD_SIZE) { return false; }

    u8 value = _vec_rom[ROM_HEAD_ROMSIZE];

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

    return true;
}

/* ***********************************************************************************************************
    MEMORY ACCESS
*********************************************************************************************************** */
// read *****
u8 GameboyMEM::ReadROM_0(const u16& _addr) {
    return ROM_0[_addr];
}

// overload for MBC1
u8 GameboyMEM::ReadROM_0(const u16& _addr, const int& _bank) {
    if (_bank == 0) { return ROM_0[_addr]; }
    else { return ROM_N[_bank - 1][_addr]; }
}

u8 GameboyMEM::ReadROM_N(const u16& _addr) {
    return ROM_N[machine_ctx.rom_bank_selected][_addr - ROM_N_OFFSET];
}

u8 GameboyMEM::ReadVRAM_N(const u16& _addr) {
    if (graphics_ctx.mode == PPU_MODE_3) {
        return 0xFF;
    } else {
        return graphics_ctx.VRAM_N[IO[CGB_VRAM_SELECT_ADDR - IO_OFFSET]][_addr - VRAM_N_OFFSET];
    }
}

u8 GameboyMEM::ReadRAM_N(const u16& _addr) {
    return RAM_N[machine_ctx.ram_bank_selected][_addr - RAM_N_OFFSET];
}

u8 GameboyMEM::ReadWRAM_0(const u16& _addr) {
    return WRAM_0[_addr - WRAM_0_OFFSET];
}

u8 GameboyMEM::ReadWRAM_N(const u16& _addr) {
    return WRAM_N[machine_ctx.wram_bank_selected][_addr - WRAM_N_OFFSET];
}

u8 GameboyMEM::ReadOAM(const u16& _addr) {
    if (graphics_ctx.mode == PPU_MODE_3 || graphics_ctx.mode == PPU_MODE_2) {
        return 0xFF;
    } else {
        return graphics_ctx.OAM[_addr - OAM_OFFSET];
    }
}

u8 GameboyMEM::ReadIO(const u16& _addr) {
    return ReadIORegister(_addr);
}

u8 GameboyMEM::ReadHRAM(const u16& _addr) {
    return HRAM[_addr - HRAM_OFFSET];
}

u8 GameboyMEM::ReadIE() {
    return machine_ctx.IE;
}

// write *****
void GameboyMEM::WriteVRAM_N(const u8& _data, const u16& _addr) {
    if (graphics_ctx.mode == PPU_MODE_3) {
        return;
    } else {
        graphics_ctx.VRAM_N[machine_ctx.vram_bank_selected][_addr - VRAM_N_OFFSET] = _data;
    }
}

void GameboyMEM::WriteRAM_N(const u8& _data, const u16& _addr) {
    RAM_N[machine_ctx.ram_bank_selected][_addr - RAM_N_OFFSET] = _data;
}

void GameboyMEM::WriteWRAM_0(const u8& _data, const u16& _addr) {
    WRAM_0[_addr - WRAM_0_OFFSET] = _data;
}

void GameboyMEM::WriteWRAM_N(const u8& _data, const u16& _addr) {
    WRAM_N[machine_ctx.wram_bank_selected][_addr - WRAM_N_OFFSET] = _data;
}

void GameboyMEM::WriteOAM(const u8& _data, const u16& _addr) {
    if (graphics_ctx.mode == PPU_MODE_3 || graphics_ctx.mode == PPU_MODE_2) {
        return;
    } else {
        graphics_ctx.OAM[_addr - OAM_OFFSET] = _data;
    }
}

void GameboyMEM::WriteIO(const u8& _data, const u16& _addr) {
    WriteIORegister(_data, _addr);
}

void GameboyMEM::WriteHRAM(const u8& _data, const u16& _addr) {
    HRAM[_addr - HRAM_OFFSET] = _data;
}

void GameboyMEM::WriteIE(const u8& _data) {
    machine_ctx.IE = _data;
}

/* ***********************************************************************************************************
    IO PROCESSING
*********************************************************************************************************** */
u8 GameboyMEM::ReadIORegister(const u16& _addr) {
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
    case CGB_HDMA1_ADDR:
    case CGB_HDMA2_ADDR:
    case CGB_HDMA3_ADDR:
    case CGB_HDMA4_ADDR:
        return 0xFF;
        break;
    default:
        if (_addr > 0xFF77) {
            return 0xFF;
        }
        else if(machine_ctx.isCgb){
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
                return 0xFF;
                break;
            default:
                return IO[_addr - IO_OFFSET];
                break;
            }
        }
    }
}

void GameboyMEM::WriteIORegister(const u8& _data, const u16& _addr) {
    switch (_addr) {
    case JOYP_ADDR:
        SetControlValues(_data);
        break;
    case DIV_ADDR:
        IO[DIV_ADDR - IO_OFFSET] = 0x00;
        machine_ctx.div_low_byte = 0x00;
        break;
    case TAC_ADDR:
        IO[TAC_ADDR - IO_OFFSET] = _data;
        ProcessTAC();
        break;
    case TIMA_ADDR:
        if (!machine_ctx.tima_reload_cycle) {
            IO[TIMA_ADDR - IO_OFFSET] = _data;
            machine_ctx.tima_overflow_cycle = false;
        }
        break;
    case IF_ADDR:
        machine_ctx.tima_reload_if_write = machine_ctx.tima_reload_cycle;
        graphics_ctx.vblank_if_write = true;
        IO[IF_ADDR - IO_OFFSET] = _data;
        break;
    case CGB_VRAM_SELECT_ADDR:
        SetVRAMBank(_data);
        break;
    case CGB_SPEED_SWITCH_ADDR:
        SwitchSpeed(_data);
        break;
    case LCDC_ADDR:
        SetLCDCValues(_data);
        break;
    case STAT_ADDR:
        SetLCDSTATValues(_data);
        break;
    case OAM_DMA_ADDR:
        IO[OAM_DMA_ADDR - IO_OFFSET] = _data;
        OAM_DMA();
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
    case BGP_ADDR:
        IO[BGP_ADDR - IO_OFFSET] = _data;
        SetColorPaletteValues(_data, graphics_ctx.dmg_bgp_color_palette);
        break;
    case OBP0_ADDR:
        IO[OBP0_ADDR - IO_OFFSET] = _data;
        SetColorPaletteValues(_data, graphics_ctx.dmg_obp0_color_palette);
        break;
    case OBP1_ADDR:
        IO[OBP1_ADDR - IO_OFFSET] = _data;
        SetColorPaletteValues(_data, graphics_ctx.dmg_obp1_color_palette);
        break;
    case LY_ADDR:
        if (!graphics_ctx.ppu_enable) {
            IO[LY_ADDR - IO_OFFSET] = _data;
        }
        break;
    case NR52_ADDR:
        SetAPUMasterControl(_data);
        break;
    case NR51_ADDR:
        SetAPUChannelPanning(_data);
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

u8& GameboyMEM::GetIO(const u16& _addr) {
    return IO[_addr - IO_OFFSET];
}

void GameboyMEM::SetIO(const u16& _addr, const u8& _data) {
    switch (_addr) {
    case IF_ADDR:
        graphics_ctx.vblank_if_write = true;
        break;
    }

    IO[_addr - IO_OFFSET] = _data;
}

u8* GameboyMEM::GetIOPtr(const u16& _addr) {
    return IO.data() + (_addr - IO_OFFSET);
}

void GameboyMEM::CopyDataToRAM(const vector<char>& _data) {
    for (int i = 0; i < RAM_N.size(); i++) {
        for (int j = 0; j < RAM_N_SIZE; j++) {
            RAM_N[i][j] = _data[i * RAM_N_SIZE + j];
        }
    }
}

void GameboyMEM::CopyDataFromRAM(vector<char>& _data) const {
    _data = vector<char>(RAM_N.size() * RAM_N_SIZE);

    for (int i = 0; i < RAM_N.size(); i++) {
        for (int j = 0; j < RAM_N_SIZE; j++) {
            _data[i * RAM_N_SIZE + j] = RAM_N[i][j];
        }
    }
}

/* ***********************************************************************************************************
    DMA
*********************************************************************************************************** */
void GameboyMEM::VRAM_DMA(const u8& _data) {
    // u8 mode = HDMA5 & DMA_MODE_BIT;
    IO[CGB_HDMA5_ADDR - IO_OFFSET] = _data;
    if (machine_ctx.isCgb) {
        IO[CGB_HDMA5_ADDR - IO_OFFSET] |= 0x80;
        u16 length = ((IO[CGB_HDMA5_ADDR - IO_OFFSET] & 0x7F) + 1) * 0x10;

        u16 source_addr = ((u16)IO[CGB_HDMA1_ADDR - IO_OFFSET] << 8) | (IO[CGB_HDMA2_ADDR - IO_OFFSET] & 0xF0);
        u16 dest_addr = (((u16)(IO[CGB_HDMA3_ADDR - IO_OFFSET] & 0x1F)) << 8) | (IO[CGB_HDMA4_ADDR - IO_OFFSET] & 0xF0);

        if (source_addr < ROM_N_OFFSET) {
            memcpy(&graphics_ctx.VRAM_N[machine_ctx.vram_bank_selected][dest_addr - VRAM_N_OFFSET], &ROM_0[source_addr], length);
        }
        else if (source_addr < VRAM_N_OFFSET) {
            memcpy(&graphics_ctx.VRAM_N[machine_ctx.vram_bank_selected][dest_addr - VRAM_N_OFFSET], &ROM_N[machine_ctx.rom_bank_selected][source_addr - ROM_N_OFFSET], length);
        }
        else if (source_addr < WRAM_0_OFFSET && source_addr >= RAM_N_OFFSET) {
            memcpy(&graphics_ctx.VRAM_N[machine_ctx.vram_bank_selected][dest_addr - VRAM_N_OFFSET], &RAM_N[machine_ctx.ram_bank_selected][source_addr - RAM_N_OFFSET], length);
        }
        else if (source_addr < WRAM_N_OFFSET) {
            memcpy(&graphics_ctx.VRAM_N[machine_ctx.vram_bank_selected][dest_addr - VRAM_N_OFFSET], &WRAM_0[source_addr - WRAM_0_OFFSET], length);
        }
        else if (source_addr < MIRROR_WRAM_OFFSET) {
            memcpy(&graphics_ctx.VRAM_N[machine_ctx.vram_bank_selected][dest_addr - VRAM_N_OFFSET], &WRAM_N[machine_ctx.wram_bank_selected][source_addr - WRAM_N_OFFSET], length);
        }
        else {
            return;
        }

        IO[CGB_HDMA5_ADDR - IO_OFFSET] = 0xFF;
    }
}

void GameboyMEM::OAM_DMA() {
    u16 source_address = IO[OAM_DMA_ADDR - IO_OFFSET] * 0x100;

    if (source_address < ROM_N_OFFSET) {
        memcpy(graphics_ctx.OAM.data(), &ROM_0[source_address], OAM_DMA_LENGTH);
    }
    else if (source_address < VRAM_N_OFFSET) {
        memcpy(graphics_ctx.OAM.data(), &ROM_N[machine_ctx.rom_bank_selected][source_address - ROM_N_OFFSET], OAM_DMA_LENGTH);
    }
    else if (source_address < RAM_N_OFFSET) {
        memcpy(graphics_ctx.OAM.data(), &graphics_ctx.VRAM_N[machine_ctx.vram_bank_selected][source_address - VRAM_N_OFFSET], OAM_DMA_LENGTH);
    }
    else if (source_address < WRAM_0_OFFSET) {
        memcpy(graphics_ctx.OAM.data(), &RAM_N[machine_ctx.ram_bank_selected][source_address - RAM_N_OFFSET], OAM_DMA_LENGTH);
    }
    else if (source_address < WRAM_N_OFFSET) {
        memcpy(graphics_ctx.OAM.data(), &WRAM_0[source_address - WRAM_0_OFFSET], OAM_DMA_LENGTH);
    }
    else if (source_address < MIRROR_WRAM_OFFSET) {
        memcpy(graphics_ctx.OAM.data(), &WRAM_N[machine_ctx.wram_bank_selected][source_address - WRAM_N_OFFSET], OAM_DMA_LENGTH);
    } else if (source_address < IE_OFFSET) {
        memcpy(graphics_ctx.OAM.data(), &HRAM[source_address - HRAM_OFFSET] , OAM_DMA_LENGTH);
    }
}

/* ***********************************************************************************************************
    TIMA CONTROL
*********************************************************************************************************** */
void GameboyMEM::ProcessTAC() {
    switch (IO[TAC_ADDR - IO_OFFSET] & TAC_CLOCK_SELECT) {
    case 0x00:
        machine_ctx.timaDivMask = TIMA_DIV_BIT_9;
        break;
    case 0x01:
        machine_ctx.timaDivMask = TIMA_DIV_BIT_3;
        break;
    case 0x02:
        machine_ctx.timaDivMask = TIMA_DIV_BIT_5;
        break;
    case 0x03:
        machine_ctx.timaDivMask = TIMA_DIV_BIT_7;

        break;
    }
}

/* ***********************************************************************************************************
    SPEED SWITCH
*********************************************************************************************************** */
void GameboyMEM::SwitchSpeed(const u8& _data) {
    if (machine_ctx.isCgb) {
        if (_data & PREPARE_SPEED_SWITCH) {
            IO[CGB_SPEED_SWITCH_ADDR - IO_OFFSET] ^= SET_SPEED;
            machine_ctx.speed_switch_requested = true;
        }
    }
}

/* ***********************************************************************************************************
    CONTROLLER INPUT
*********************************************************************************************************** */
void GameboyMEM::SetControlValues(const u8& _data) {
    control_ctx.buttons_selected = (!(_data & JOYP_SELECT_BUTTONS)) ? true : false;
    control_ctx.dpad_selected = (!(_data & JOYP_SELECT_DPAD)) ? true : false;

    if (control_ctx.buttons_selected || control_ctx.dpad_selected) {
        u8 data = _data & JOYP_SELECT_MASK;
        if (control_ctx.buttons_selected) {
            if (!control_ctx.start_pressed) {
                data |= JOYP_START_DOWN;
            }
            if (!control_ctx.select_pressed) {
                data |= JOYP_SELECT_UP;
            }
            if (!control_ctx.b_pressed) {
                data |= JOYP_B_LEFT;
            }
            if (!control_ctx.a_pressed) {
                data |= JOYP_A_RIGHT;
            }
        }
        if (control_ctx.dpad_selected) {
            if (!control_ctx.down_pressed) {
                data |= JOYP_START_DOWN;
            }
            if (!control_ctx.up_pressed) {
                data |= JOYP_SELECT_UP;
            }
            if (!control_ctx.left_pressed) {
                data |= JOYP_B_LEFT;
            }
            if (!control_ctx.right_pressed) {
                data |= JOYP_A_RIGHT;
            }
        }

        IO[JOYP_ADDR - IO_OFFSET] = data;
    } else {
        IO[JOYP_ADDR - IO_OFFSET] = (_data & JOYP_SELECT_MASK) | JOYP_RESET_BUTTONS;
    }
}

void GameboyMEM::SetButton(const u8& _bit, const bool& _is_button) {
    if ((control_ctx.buttons_selected && _is_button) || (control_ctx.dpad_selected && !_is_button)) {
        IO[JOYP_ADDR - IO_OFFSET] &= ~_bit;
        RequestInterrupts(IRQ_JOYPAD);
    }
}

void GameboyMEM::UnsetButton(const u8& _bit, const bool& _is_button) {
    if ((control_ctx.buttons_selected && _is_button) || (control_ctx.dpad_selected && !_is_button)) {
        IO[JOYP_ADDR - IO_OFFSET] |= _bit;
    }
}

/* ***********************************************************************************************************
    OBJ PRIORITY MODE
*********************************************************************************************************** */
// not relevant, gets set by boot rom but we do the check for DMG or CGB on our own
void GameboyMEM::SetObjPrio(const u8& _data) {
    switch (_data & PRIO_MODE) {
    case DMG_OBJ_PRIO_MODE:
        IO[CGB_OBJ_PRIO_ADDR - IO_OFFSET] |= DMG_OBJ_PRIO_MODE;
        break;
    case CGB_OBJ_PRIO_MODE:
        IO[CGB_OBJ_PRIO_ADDR - IO_OFFSET] &= ~DMG_OBJ_PRIO_MODE;
        break;
    }
}

/* ***********************************************************************************************************
    LCD AND PPU FUNCTIONALITY
*********************************************************************************************************** */
void GameboyMEM::SetLCDCValues(const u8& _data) {
    IO[LCDC_ADDR - IO_OFFSET] = _data;

    // bit 0
    if (_data & PPU_LCDC_WINBG_EN_PRIO) {
        graphics_ctx.bg_win_enable = true;              // DMG
        graphics_ctx.obj_prio = true;                   // CGB
    } else {
        graphics_ctx.bg_win_enable = false;
        graphics_ctx.obj_prio = false;
    }

    // bit 1
    if(_data & PPU_LCDC_OBJ_ENABLE) {
        graphics_ctx.obj_enable = true;
    } else {
        graphics_ctx.obj_enable = false;
    }

    // bit 2
    if (_data & PPU_LCDC_OBJ_SIZE) {
        graphics_ctx.obj_size_16 = true;
    } else {
        graphics_ctx.obj_size_16 = false;
    }

    // bit 3
    if (_data & PPU_LCDC_BG_TILEMAP) {
        graphics_ctx.bg_tilemap_offset = PPU_TILE_MAP1 - VRAM_N_OFFSET;
    } else {
        graphics_ctx.bg_tilemap_offset = PPU_TILE_MAP0 - VRAM_N_OFFSET;
    }

    // bit 4
    if (_data & PPU_LCDC_WINBG_TILEDATA) {
        graphics_ctx.bg_win_addr_mode_8000 = true;
    } else {
        graphics_ctx.bg_win_addr_mode_8000 = false;
    }

    // bit 5
    if (_data & PPU_LCDC_WIN_ENABLE) {
        graphics_ctx.win_enable = true;
    } else {
        graphics_ctx.win_enable = false;
    }

    // bit 6
    if (_data & PPU_LCDC_WIN_TILEMAP) {
        graphics_ctx.win_tilemap_offset = PPU_TILE_MAP1 - VRAM_N_OFFSET;
    } else {
        graphics_ctx.win_tilemap_offset = PPU_TILE_MAP0 - VRAM_N_OFFSET;
    }

    // bit 7
    if (_data & PPU_LCDC_ENABLE) {
        graphics_ctx.ppu_enable = true;
    } else {
        graphics_ctx.ppu_enable = false;
        IO[LY_ADDR - IO_OFFSET] = 0x00;
        graphics_ctx.mode = PPU_MODE_2;
    }
}

void GameboyMEM::SetLCDSTATValues(const u8& _data) {
    u8 data = (IO[STAT_ADDR - IO_OFFSET] & ~PPU_STAT_WRITEABLE_BITS) | (_data & PPU_STAT_WRITEABLE_BITS);
    IO[STAT_ADDR - IO_OFFSET] = data;

    if (data & PPU_STAT_MODE0_EN) {
        graphics_ctx.mode_0_int_sel = true;
    } else {
        graphics_ctx.mode_0_int_sel = false;
    }

    if (data & PPU_STAT_MODE1_EN) {
        graphics_ctx.mode_1_int_sel = true;
    } else {
        graphics_ctx.mode_1_int_sel = false;
    }

    if (data & PPU_STAT_MODE2_EN) {
        graphics_ctx.mode_2_int_sel = true;
    } else {
        graphics_ctx.mode_2_int_sel = false;
    }

    if (data & PPU_STAT_LYC_SOURCE) {
        graphics_ctx.lyc_ly_int_sel = true;
    } else {
        graphics_ctx.lyc_ly_int_sel = false;
    }
}

void GameboyMEM::SetColorPaletteValues(const u8& _data, u32* _color_palette) {
    u8 colors = _data;
    for (int i = 0; i < 4; i++) {
        switch (colors & 0x03) {
        case 0x00:
            _color_palette[i] = DMG_COLOR_WHITE_ALT;
            break;
        case 0x01:
            _color_palette[i] = DMG_COLOR_LIGHTGREY_ALT;
            break;
        case 0x02:
            _color_palette[i] = DMG_COLOR_DARKGREY_ALT;
            break;
        case 0x03:
            _color_palette[i] = DMG_COLOR_BLACK_ALT;
            break;
        }

        colors >>= 2;
    }
}

/* ***********************************************************************************************************
    MISC BANK SELECT
*********************************************************************************************************** */
void GameboyMEM::SetVRAMBank(const u8& _data) {
    if (machine_ctx.isCgb) {
        IO[CGB_VRAM_SELECT_ADDR - IO_OFFSET] = _data & 0x01;
        machine_ctx.vram_bank_selected = IO[CGB_VRAM_SELECT_ADDR - IO_OFFSET];
    }
}

void GameboyMEM::SetWRAMBank(const u8& _data) {
    if (machine_ctx.isCgb) {
        IO[CGB_WRAM_SELECT_ADDR - IO_OFFSET] = _data & 0x07;
        if (IO[CGB_WRAM_SELECT_ADDR - IO_OFFSET] == 0) IO[CGB_WRAM_SELECT_ADDR - IO_OFFSET] = 1;

        machine_ctx.wram_bank_selected = IO[CGB_WRAM_SELECT_ADDR - IO_OFFSET] - 1;
    }
}

/* ***********************************************************************************************************
    APU CONTROL
*********************************************************************************************************** */
void GameboyMEM::SetAPUMasterControl(const u8& _data) {
    IO[NR52_ADDR - IO_OFFSET] = (IO[NR52_ADDR - IO_OFFSET] & ~APU_ENABLE_BIT) | (_data & APU_ENABLE_BIT);
    sound_ctx.apuEnable = _data & APU_ENABLE_BIT ? true : false;
}

void GameboyMEM::SetAPUChannelPanning(const u8& _data) {
    IO[NR51_ADDR - IO_OFFSET] = _data;

    u8 data = _data;
    for (int i = 0; i < 8; i++) {
        sound_ctx.channelPanning[i] = (data & 0x01 ? true : false);
        data >>= 1;
    }
}

void GameboyMEM::SetAPUMasterVolume(const u8& _data) {
    IO[NR50_ADDR - IO_OFFSET] = _data;

    sound_ctx.masterVolumeRight = (float)VOLUME_MAP.at(_data & MASTER_VOLUME_RIGHT);
    sound_ctx.masterVolumeLeft = (float)VOLUME_MAP.at((_data & MASTER_VOLUME_LEFT) >> 4);
}

// channel 1


/* ***********************************************************************************************************
    MEMORY DEBUGGER
*********************************************************************************************************** */
inline void setup_bank_access(ScrollableTableBuffer<memory_data>& _table_buffer, vector<u8>& _bank, const int& _offset) {
    auto data = memory_data();

    int size = (int)_bank.size();
    int line_num = (int)LINE_NUM(size);
    int index;

    _table_buffer = ScrollableTableBuffer<memory_data>(line_num);

    for (int i = 0; i < line_num; i++) {
        auto table_entry = ScrollableTableEntry<memory_data>();

        index = i * DEBUG_MEM_ELEM_PER_LINE;
        get<ST_ENTRY_ADDRESS>(table_entry) = _offset + index;

        data = memory_data();
        get<MEM_ENTRY_ADDR>(data) = format("{:04x}", _offset + index);
        get<MEM_ENTRY_LEN>(data) = size - index > DEBUG_MEM_ELEM_PER_LINE - 1 ? DEBUG_MEM_ELEM_PER_LINE : size % DEBUG_MEM_ELEM_PER_LINE;
        get<MEM_ENTRY_REF>(data) = &_bank[index];

        get<ST_ENTRY_DATA>(table_entry) = data;

        _table_buffer[i] = table_entry;
    }
}

void GameboyMEM::SetupDebugMemoryAccess() {
    // access for memory inspector
    machineInfo.memory_buffer.clear();

    auto mem_type_table_buffer = MemoryBufferEntry<GuiScrollableTable<memory_data>>();
    auto table = GuiScrollableTable<memory_data>(DEBUG_MEM_LINES);
    auto table_buffer = ScrollableTableBuffer<memory_data>();

    // ROM
    {
        mem_type_table_buffer = MemoryBufferEntry<GuiScrollableTable<memory_data>>();
        mem_type_table_buffer.first = "ROM";

        table = GuiScrollableTable<memory_data>(DEBUG_MEM_LINES);
        table_buffer = ScrollableTableBuffer<memory_data>();

        setup_bank_access(table_buffer, ROM_0, ROM_0_OFFSET);

        table.AddMemoryArea(table_buffer);
        mem_type_table_buffer.second.emplace_back(table);

        for (auto& n : ROM_N) {
            table = GuiScrollableTable<memory_data>(DEBUG_MEM_LINES);
            table_buffer = ScrollableTableBuffer<memory_data>();

            setup_bank_access(table_buffer, n, ROM_N_OFFSET);

            table.AddMemoryArea(table_buffer);
            mem_type_table_buffer.second.emplace_back(table);
        }

        machineInfo.memory_buffer.emplace_back(mem_type_table_buffer);
    }

    // VRAM
    {
        mem_type_table_buffer = MemoryBufferEntry<GuiScrollableTable<memory_data>>();
        mem_type_table_buffer.first = "VRAM";

        for (auto& n : graphics_ctx.VRAM_N) {
            table = GuiScrollableTable<memory_data>(DEBUG_MEM_LINES);
            table_buffer = ScrollableTableBuffer<memory_data>();

            setup_bank_access(table_buffer, n, VRAM_N_OFFSET);

            table.AddMemoryArea(table_buffer);
            mem_type_table_buffer.second.emplace_back(table);
        }

        machineInfo.memory_buffer.emplace_back(mem_type_table_buffer);
    }

    // RAM
    if (machine_ctx.ram_bank_num > 0) {
        mem_type_table_buffer = MemoryBufferEntry<GuiScrollableTable<memory_data>>();
        mem_type_table_buffer.first = "RAM";

        for (auto& n : RAM_N) {
            table = GuiScrollableTable<memory_data>(DEBUG_MEM_LINES);
            table_buffer = ScrollableTableBuffer<memory_data>();

            setup_bank_access(table_buffer, n, RAM_N_OFFSET);

            table.AddMemoryArea(table_buffer);
            mem_type_table_buffer.second.emplace_back(table);
        }

        machineInfo.memory_buffer.emplace_back(mem_type_table_buffer);
    }

    // WRAM
    {
        mem_type_table_buffer = MemoryBufferEntry<GuiScrollableTable<memory_data>>();
        mem_type_table_buffer.first = "WRAM";

        table = GuiScrollableTable<memory_data>(DEBUG_MEM_LINES);
        table_buffer = ScrollableTableBuffer<memory_data>();

        setup_bank_access(table_buffer, WRAM_0, WRAM_0_OFFSET);

        table.AddMemoryArea(table_buffer);
        mem_type_table_buffer.second.emplace_back(table);

        for (auto& n : WRAM_N) {
            table = GuiScrollableTable<memory_data>(DEBUG_MEM_LINES);
            table_buffer = ScrollableTableBuffer<memory_data>();

            setup_bank_access(table_buffer, n, WRAM_N_OFFSET);

            table.AddMemoryArea(table_buffer);
            mem_type_table_buffer.second.emplace_back(table);
        }

        machineInfo.memory_buffer.emplace_back(mem_type_table_buffer);
    }

    // OAM
    {
        mem_type_table_buffer = MemoryBufferEntry<GuiScrollableTable<memory_data>>();
        mem_type_table_buffer.first = "OAM";

        table = GuiScrollableTable<memory_data>(DEBUG_MEM_LINES);
        table_buffer = ScrollableTableBuffer<memory_data>();

        setup_bank_access(table_buffer, graphics_ctx.OAM, OAM_OFFSET);

        table.AddMemoryArea(table_buffer);
        mem_type_table_buffer.second.emplace_back(table);

        machineInfo.memory_buffer.emplace_back(mem_type_table_buffer);
    }

    // IO
    {
        mem_type_table_buffer = MemoryBufferEntry<GuiScrollableTable<memory_data>>();
        mem_type_table_buffer.first = "IO";

        table = GuiScrollableTable<memory_data>(DEBUG_MEM_LINES);
        table_buffer = ScrollableTableBuffer<memory_data>();

        setup_bank_access(table_buffer, IO, IO_OFFSET);

        table.AddMemoryArea(table_buffer);
        mem_type_table_buffer.second.emplace_back(table);

        machineInfo.memory_buffer.emplace_back(mem_type_table_buffer);
    }

    // HRAM
    {
        mem_type_table_buffer = MemoryBufferEntry<GuiScrollableTable<memory_data>>();
        mem_type_table_buffer.first = "HRAM";

        table = GuiScrollableTable<memory_data>(DEBUG_MEM_LINES);
        table_buffer = ScrollableTableBuffer<memory_data>();

        setup_bank_access(table_buffer, HRAM, HRAM_OFFSET);

        table.AddMemoryArea(table_buffer);
        mem_type_table_buffer.second.emplace_back(table);

        machineInfo.memory_buffer.emplace_back(mem_type_table_buffer);
    }
}

std::vector<std::pair<int, std::vector<u8>>> GameboyMEM::GetProgramData() const {
    auto rom_data = vector<pair<int, vector<u8>>>(machine_ctx.rom_bank_num);

    rom_data[0] = pair(ROM_0_OFFSET, ROM_0);
    for (int i = 1; const auto & bank : ROM_N) {
        rom_data[i++] = pair(ROM_N_OFFSET, bank);
    }

    return rom_data;
}