#include "GameboyMEM.h"

#include "gameboy_defines.h"
#include "logger.h"
#include "format"
#include "GameboyGPU.h"
#include "GameboyCPU.h"

#include <iostream>

using namespace std;

/* ***********************************************************************************************************
    DEFINES
*********************************************************************************************************** */
#define LINE_NUM_HEX(x)     (((float)x / DEBUG_MEM_ELEM_PER_LINE) + ((float)(DEBUG_MEM_ELEM_PER_LINE - 1) / DEBUG_MEM_ELEM_PER_LINE))

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
void GameboyMEM::InitMemory(BaseCartridge* _cartridge) {
    const auto& vec_rom = _cartridge->GetRomVector();

    machine_ctx.isCgb = vec_rom[ROM_HEAD_CGBFLAG] & 0x80 ? true : false;
    machine_ctx.wram_bank_num = (machine_ctx.isCgb ? 8 : 2);
    machine_ctx.vram_bank_num = (machine_ctx.isCgb ? 2 : 1);

    if (!ReadRomHeaderInfo(vec_rom)) { 
        LOG_ERROR("Couldn't acquire memory information");
        return; 
    }

    AllocateMemory(_cartridge);

    if (!CopyRom(vec_rom)) {
        LOG_ERROR("Couldn't copy ROM data");
        return;
    } else {
        LOG_INFO("[emu] ROM copied");
    }
    InitMemoryState();
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
void GameboyMEM::AllocateMemory(BaseCartridge* _cartridge) {
    ROM_0 = vector<u8>(ROM_0_SIZE, 0);
    ROM_N = vector<vector<u8>>(machine_ctx.rom_bank_num - 1);
    for (int i = 0; i < machine_ctx.rom_bank_num - 1; i++) {
        ROM_N[i] = vector<u8>(ROM_N_SIZE, 0);
    }

    graphics_ctx.VRAM_N = vector<vector<u8>>(machine_ctx.vram_bank_num);
    for (int i = 0; i < machine_ctx.vram_bank_num; i++) {
        graphics_ctx.VRAM_N[i] = vector<u8>(VRAM_N_SIZE, 0);
    }

    if (machine_ctx.ram_present && machine_ctx.ram_bank_num > 0) {
        if (machine_ctx.battery_buffered) {
            auto file_name = split_string(_cartridge->fileName, ".");
            saveFile = SAVE_FOLDER;

            size_t j = file_name.size() - 1;
            for (size_t i = 0; i < j; i++) {
                saveFile += file_name[i];
                if (i < j - 1) { saveFile += "."; }
            }
            saveFile += SAVE_EXT;

            u8* data = (u8*)mapper.GetMappedFile(saveFile.c_str(), machine_ctx.ram_bank_num * RAM_N_SIZE);

            RAM_N.clear();
            for (int i = 0; i < machine_ctx.ram_bank_num; i++) {
                RAM_N.emplace_back(data + i * RAM_N_SIZE);

                /* only for testing the mapping
                for (int j = 0; j < RAM_N_SIZE / 0x10; j++) {
                    std::string s = "";
                    for (int k = 0; k < 0x10; k++) {
                        s += std::format("{:02x}", RAM_N[i][j * 0x10 + k]) + " ";
                    }
                    LOG_WARN(s);
                }
                */
            }
        } else {
            RAM_N = vector<u8*>(machine_ctx.ram_bank_num);
            for (int i = 0; i < machine_ctx.ram_bank_num; i++) {
                RAM_N[i] = new u8[RAM_N_SIZE];
            }
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
    case RAM_BANK_NUM_0_VAL:
        machine_ctx.ram_bank_num = 0;
        break;
    case RAM_BANK_NUM_1_VAL:
        machine_ctx.ram_bank_num = 1;
        break;
    case RAM_BANK_NUM_4_VAL:
        machine_ctx.ram_bank_num = 4;
        break;
        // not allowed
    case RAM_BANK_NUM_8_VAL:
        machine_ctx.ram_bank_num = 8;
        return false;
        break;
        // not allowed
    case RAM_BANK_NUM_16_VAL:
        machine_ctx.ram_bank_num = 16;
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
    if (graphics_ctx.ppu_enable && graphics_ctx.mode == PPU_MODE_3) {
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
    if (graphics_ctx.ppu_enable && (graphics_ctx.mode == PPU_MODE_3 || graphics_ctx.mode == PPU_MODE_2)) {
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
    if (graphics_ctx.ppu_enable && graphics_ctx.mode == PPU_MODE_3) {
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
    if (graphics_ctx.ppu_enable && (graphics_ctx.mode == PPU_MODE_3 || graphics_ctx.mode == PPU_MODE_2)) {
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
            return IO[_addr - IO_OFFSET];
        }
        else {
            switch (_addr) {
            case CGB_SPEED_SWITCH_ADDR:
            case CGB_VRAM_SELECT_ADDR:
            case CGB_HDMA5_ADDR:
            case CGB_IR_ADDR:
            case BCPS_BGPI_ADDR:
            case BCPD_BGPD_ADDR:
            case OCPS_OBPI_ADDR:
            case OCPD_OBPD_ADDR:
            case CGB_OBJ_PRIO_ADDR:
            case CGB_WRAM_SELECT_ADDR:
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
        OAM_DMA(_data);
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
        // DMG only
    case BGP_ADDR:
        IO[BGP_ADDR - IO_OFFSET] = _data;
        SetColorPaletteValues(_data, graphics_ctx.dmg_bgp_color_palette);
        break;
        // DMG only
    case OBP0_ADDR:
        IO[OBP0_ADDR - IO_OFFSET] = _data;
        SetColorPaletteValues(_data, graphics_ctx.dmg_obp0_color_palette);
        break;
    case OBP1_ADDR:
        // DMG only
        IO[OBP1_ADDR - IO_OFFSET] = _data;
        SetColorPaletteValues(_data, graphics_ctx.dmg_obp1_color_palette);
        break;
    case BCPD_BGPD_ADDR:
        SetBGWINPaletteValues(_data);
        break;
    case OCPD_OBPD_ADDR:
        SetOBJPaletteValues(_data);
        break;
    case BCPS_BGPI_ADDR:
        SetBCPS(_data);
        break;
    case OCPS_OBPI_ADDR:
        SetOCPS(_data);
        break;
    case LY_ADDR:
        break;
    case NR52_ADDR:
        SetAPUMasterControl(_data);
        break;
    case NR51_ADDR:
        SetAPUChannelPanning(_data);
        break;
    case NR50_ADDR:
        SetAPUMasterVolume(_data);
        break;
    case NR10_ADDR:
        SetAPUCh1Sweep(_data);
        break;
    case NR11_ADDR:
        SetAPUCh1TimerDutyCycle(_data);
        break;
    case NR12_ADDR:
        SetAPUCh1Envelope(_data);
        break;
    case NR13_ADDR:
        SetAPUCh1PeriodLow(_data);
        break;
    case NR14_ADDR:
        SetAPUCh1PeriodHighControl(_data);
        break;
    case NR21_ADDR:
        SetAPUCh2TimerDutyCycle(_data);
        break;
    case NR22_ADDR:
        SetAPUCh2Envelope(_data);
        break;
    case NR23_ADDR:
        SetAPUCh2PeriodLow(_data);
        break;
    case NR24_ADDR:
        SetAPUCh2PeriodHighControl(_data);
        break;
    case NR30_ADDR:
        SetAPUCh3DACEnable(_data);
        break;
    case NR31_ADDR:
        SetAPUCh3Timer(_data);
        break;
    case NR32_ADDR:
        SetAPUCh3Volume(_data);
        break;
    case NR33_ADDR:
        SetAPUCh3PeriodLow(_data);
        break;
    case NR34_ADDR:
        SetAPUCh3PeriodHighControl(_data);
        break;
    case NR41_ADDR:
        SetAPUCh4Timer(_data);
        break;
    case NR42_ADDR:
        SetAPUCh4Envelope(_data);
        break;
    case NR43_ADDR:
        SetAPUCh4FrequRandomness(_data);
        break;
    case NR44_ADDR:
        SetAPUCh4Control(_data);
        break;
    default:
        // Wave RAM
        if (WAVE_RAM_ADDR - 1 < _addr && _addr < WAVE_RAM_ADDR + WAVE_RAM_SIZE) {
            SetAPUCh3WaveRam(_addr, _data);
        } else {
            IO[_addr - IO_OFFSET] = _data;
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

const u8* GameboyMEM::GetBank(const MEM_TYPE& _type, const int& _bank) {
    switch (_type) {
    case MEM_TYPE::ROM0:
        return ROM_0.data();
        break;
    case MEM_TYPE::ROMn:
        return ROM_N[_bank].data();
        break;
    case MEM_TYPE::RAMn:
        return RAM_N[_bank];
        break;
    case MEM_TYPE::WRAM0:
        return WRAM_0.data();
        break;
    case MEM_TYPE::WRAMn:
        return WRAM_N[_bank].data();
        break;
    default:
        LOG_ERROR("[emu] GetBank: memory area access not implemented");
        return nullptr;
        break;
    }
}

/* ***********************************************************************************************************
    DMA
*********************************************************************************************************** */
void GameboyMEM::VRAM_DMA(const u8& _data) {
    if (machine_ctx.isCgb) {
        if (graphics_ctx.vram_dma) {
            //LOG_WARN("HDMA5 while DMA");
            graphics_ctx.vram_dma = false;
            if (!(_data & 0x80)) {
                IO[CGB_HDMA5_ADDR - IO_OFFSET] = 0x80 | _data;
                return;
            }
        }

        IO[CGB_HDMA5_ADDR - IO_OFFSET] = _data & 0x7F;

        u16 source_addr = ((u16)IO[CGB_HDMA1_ADDR - IO_OFFSET] << 8) | (IO[CGB_HDMA2_ADDR - IO_OFFSET] & 0xF0);
        u16 dest_addr = (((u16)(IO[CGB_HDMA3_ADDR - IO_OFFSET] & 0x1F)) << 8) | (IO[CGB_HDMA4_ADDR - IO_OFFSET] & 0xF0);

        graphics_ctx.vram_dma_ppu_en = graphics_ctx.ppu_enable;

        GameboyCPU* core_instance = (GameboyCPU*)BaseCPU::getInstance();

        if (_data & 0x80) {
            // HBLANK DMA
            core_instance->TickTimers();

            if (source_addr < ROM_N_OFFSET) {
                graphics_ctx.vram_dma_src_addr = source_addr;
                graphics_ctx.vram_dma_mem = MEM_TYPE::ROM0;
            } else if (source_addr < VRAM_N_OFFSET) {
                graphics_ctx.vram_dma_src_addr = source_addr - ROM_N_OFFSET;
                graphics_ctx.vram_dma_mem = MEM_TYPE::ROMn;
            } else if (source_addr < RAM_N_OFFSET) {
                LOG_ERROR("[emu] HDMA source address ", std::format("{:04x}", source_addr), " undefined copy");
                return;
            } else if (source_addr < WRAM_0_OFFSET) {
                graphics_ctx.vram_dma_src_addr = source_addr - RAM_N_OFFSET;
                graphics_ctx.vram_dma_mem = MEM_TYPE::RAMn;
            } else if (source_addr < WRAM_N_OFFSET) {
                graphics_ctx.vram_dma_src_addr = source_addr - WRAM_0_OFFSET;
                graphics_ctx.vram_dma_mem = MEM_TYPE::WRAM0;
            } else if (source_addr < MIRROR_WRAM_OFFSET) {
                graphics_ctx.vram_dma_src_addr = source_addr - WRAM_N_OFFSET;
                graphics_ctx.vram_dma_mem = MEM_TYPE::WRAMn;
            } else {
                graphics_ctx.vram_dma_src_addr = source_addr - (RAM_N_OFFSET + 0x4000);
                graphics_ctx.vram_dma_mem = MEM_TYPE::RAMn;
            }

            graphics_ctx.vram_dma_dst_addr = dest_addr;
            graphics_ctx.vram_dma = true;

            if (graphics_ctx.mode == PPU_MODE_0 || !graphics_ctx.ppu_enable) {
                ((GameboyGPU*)BaseGPU::getInstance())->VRAMDMANextBlock();
            }

            //LOG_WARN("VRAM HBLANK DMA: ", graphics_ctx.dma_length * 0x10);
        } else {
            // General purpose DMA
            u8 blocks = (IO[CGB_HDMA5_ADDR - IO_OFFSET] & 0x7F) + 1;
            u16 length = (u16)blocks * 0x10;

            int machine_cycles = ((int)blocks * VRAM_DMA_MC_PER_BLOCK * machine_ctx.currentSpeed) + 1;
            for (int i = 0; i < machine_cycles; i++) {
                core_instance->TickTimers();
            }

            if (source_addr < ROM_N_OFFSET) {
                memcpy(&graphics_ctx.VRAM_N[machine_ctx.vram_bank_selected][dest_addr], &ROM_0[source_addr], length);
            } else if (source_addr < VRAM_N_OFFSET) {
                memcpy(&graphics_ctx.VRAM_N[machine_ctx.vram_bank_selected][dest_addr], &ROM_N[machine_ctx.rom_bank_selected][source_addr - ROM_N_OFFSET], length);
            } else if (source_addr < RAM_N_OFFSET) {
                LOG_ERROR("[emu] HDMA source address ", std::format("{:04x}", source_addr), " undefined copy");
                return;
            } else if (source_addr < WRAM_0_OFFSET) {
                memcpy(&graphics_ctx.VRAM_N[machine_ctx.vram_bank_selected][dest_addr], &RAM_N[machine_ctx.ram_bank_selected][source_addr - RAM_N_OFFSET], length);
            } else if (source_addr < WRAM_N_OFFSET) {
                memcpy(&graphics_ctx.VRAM_N[machine_ctx.vram_bank_selected][dest_addr], &WRAM_0[source_addr - WRAM_0_OFFSET], length);
            } else if (source_addr < MIRROR_WRAM_OFFSET) {
                memcpy(&graphics_ctx.VRAM_N[machine_ctx.vram_bank_selected][dest_addr], &WRAM_N[machine_ctx.wram_bank_selected][source_addr - WRAM_N_OFFSET], length);
            } else {
                memcpy(&graphics_ctx.VRAM_N[machine_ctx.vram_bank_selected][dest_addr], &RAM_N[machine_ctx.ram_bank_selected][source_addr - (RAM_N_OFFSET + 0x4000)], length);
            }

            IO[CGB_HDMA5_ADDR - IO_OFFSET] = 0xFF;
        }
    }
}

void GameboyMEM::OAM_DMA(const u8& _data) {
    graphics_ctx.oam_dma = false;
    
    GameboyCPU* core_instance = (GameboyCPU*)BaseCPU::getInstance();
    core_instance->TickTimers();

    IO[OAM_DMA_ADDR - IO_OFFSET] = _data;
    u16 source_addr = (u16)IO[OAM_DMA_ADDR - IO_OFFSET] << 8;

    if (source_addr < ROM_N_OFFSET) {
        graphics_ctx.oam_dma_src_addr = source_addr;
        graphics_ctx.oam_dma_mem = MEM_TYPE::ROM0;
    } else if (source_addr < VRAM_N_OFFSET) {
        graphics_ctx.oam_dma_src_addr = source_addr - ROM_N_OFFSET;
        graphics_ctx.oam_dma_mem = MEM_TYPE::ROMn;
    } else if (source_addr < RAM_N_OFFSET) {
        LOG_ERROR("[emu] OAM DMA source address ", std::format("{:04x}", source_addr), " undefined copy");
        return;
    } else if (source_addr < WRAM_0_OFFSET) {
        graphics_ctx.oam_dma_src_addr = source_addr - RAM_N_OFFSET;
        graphics_ctx.oam_dma_mem = MEM_TYPE::RAMn;
    } else if (source_addr < WRAM_N_OFFSET) {
        graphics_ctx.oam_dma_src_addr = source_addr - WRAM_0_OFFSET;
        graphics_ctx.oam_dma_mem = MEM_TYPE::WRAM0;
    } else if (source_addr < MIRROR_WRAM_OFFSET) {
        graphics_ctx.oam_dma_src_addr = source_addr - WRAM_N_OFFSET;
        graphics_ctx.oam_dma_mem = MEM_TYPE::WRAMn;
    } else {
        LOG_ERROR("[emu] OAM DMA source address not implemented / allowed");
        return;
    }

    graphics_ctx.oam_dma = true;
    graphics_ctx.oam_dma_counter = 0;
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

        IO[JOYP_ADDR - IO_OFFSET] = data | 0xC0;
    } else {
        IO[JOYP_ADDR - IO_OFFSET] = ((_data & JOYP_SELECT_MASK) | JOYP_RESET_BUTTONS) | 0xC0;
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

        GameboyGPU* graphics_instance = (GameboyGPU*)BaseGPU::getInstance();

        graphics_instance->SetMode(PPU_MODE_2);

        if (graphics_ctx.vram_dma_ppu_en) {
            graphics_instance->VRAMDMANextBlock();
            graphics_ctx.vram_dma_ppu_en = false;             // needs to be investigated if this is correct, docs state that the block transfer happens when ppu was enabled when hdma started and ppu gets disabled afterwards
        }
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

    // TODO: CGB is able to set different color palettes for DMG games
    if (machine_ctx.isCgb) {
        for (int i = 0; i < 4; i++) {
            switch (colors & 0x03) {
            case 0x00:
                _color_palette[i] = CGB_DMG_COLOR_WHITE;
                break;
            case 0x01:
                _color_palette[i] = CGB_DMG_COLOR_LIGHTGREY;
                break;
            case 0x02:
                _color_palette[i] = CGB_DMG_COLOR_DARKGREY;
                break;
            case 0x03:
                _color_palette[i] = CGB_DMG_COLOR_BLACK;
                break;
            }

            colors >>= 2;
        }
    } else {
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
}

void GameboyMEM::SetBGWINPaletteValues(const u8& _data) {
    u8 addr = IO[BCPS_BGPI_ADDR - IO_OFFSET] & 0x3F;

    if (!graphics_ctx.ppu_enable || graphics_ctx.mode != PPU_MODE_3) {
        graphics_ctx.cgb_bgp_palette_ram[addr] = _data;

        // set new palette value
        int ram_index = addr & 0xFE;                    // get index in color ram, first bit irrelevant because colors aligned as 2 bytes
        int palette_index = addr >> 3;                  // get index of palette (divide by 8 (divide by 2^3 -> shift by 3), because each palette has 4 colors @ 2 bytes = 8 bytes each)
        int color_index = (addr & 0x07) >> 1;           // get index of color within palette (lower 3 bit -> can address 8 byte; shift by 1 because each color has 2 bytes (divide by 2^1))

        // NOTE: CGB has RGB555 in little endian -> reverse order
        u16 rgb555_color = graphics_ctx.cgb_bgp_palette_ram[ram_index] | ((u16)graphics_ctx.cgb_bgp_palette_ram[ram_index + 1] << 8);
        u32 rgba8888_color = ((u32)(rgb555_color & PPU_CGB_RED) << 27) | ((u32)(rgb555_color & PPU_CGB_GREEN) << 14) | ((u32)(rgb555_color & PPU_CGB_BLUE) << 1) | 0xFF;        // additionally leftshift each value by 3 -> 5 bit to 8 bit 'scaling'

        graphics_ctx.cgb_bgp_color_palettes[palette_index][color_index] = rgba8888_color;
        
        /*
        LOG_WARN("------------");
        LOG_WARN("RAM: ", ram_index, "; Palette: ", palette_index, "; Color: ", color_index, "; Data: ", format("{:02x}", _data));
        for (int i = 0; i < 8; i++) {
            LOG_INFO("Palette ", i, ": ", format("{:08x}", graphics_ctx.cgb_bgp_color_palettes[i][0]), format("{:08x}", graphics_ctx.cgb_bgp_color_palettes[i][1]), format("{:08x}", graphics_ctx.cgb_bgp_color_palettes[i][2]), format("{:08x}", graphics_ctx.cgb_bgp_color_palettes[i][3]));
        }
        */
    }

    if (graphics_ctx.bgp_increment) {
        addr++;
        IO[BCPS_BGPI_ADDR - IO_OFFSET] = (IO[BCPS_BGPI_ADDR - IO_OFFSET] & PPU_CGB_PALETTE_INDEX_INC) | (addr & 0x3F);
    }
}

void GameboyMEM::SetOBJPaletteValues(const u8& _data) {
    u8 addr = IO[OCPS_OBPI_ADDR - IO_OFFSET] & 0x3F;

    if (!graphics_ctx.ppu_enable || graphics_ctx.mode != PPU_MODE_3) {
        graphics_ctx.cgb_obp_palette_ram[addr] = _data;

        // set new palette value
        int ram_index = addr & 0xFE;
        int palette_index = addr >> 3;
        int color_index = (addr & 0x07) >> 1;

        u16 rgb555_color = graphics_ctx.cgb_obp_palette_ram[ram_index] | ((u16)graphics_ctx.cgb_obp_palette_ram[ram_index + 1] << 8);
        u32 rgba8888_color = ((u32)(rgb555_color & PPU_CGB_RED) << 27) | ((u32)(rgb555_color & PPU_CGB_GREEN) << 14) | ((u32)(rgb555_color & PPU_CGB_BLUE) << 1) | 0xFF;

        graphics_ctx.cgb_obp_color_palettes[palette_index][color_index] = rgba8888_color;
    }

    if (graphics_ctx.obp_increment) {
        addr++;
        IO[OCPS_OBPI_ADDR - IO_OFFSET] = (IO[OCPS_OBPI_ADDR - IO_OFFSET] & PPU_CGB_PALETTE_INDEX_INC) | (addr & 0x3F);
    }
}

void GameboyMEM::SetBCPS(const u8& _data) {
    IO[BCPS_BGPI_ADDR - IO_OFFSET] = _data;
    graphics_ctx.bgp_increment = (_data & PPU_CGB_PALETTE_INDEX_INC ? true : false);
}

void GameboyMEM::SetOCPS(const u8& _data) {
    IO[OCPS_OBPI_ADDR - IO_OFFSET] = _data;
    graphics_ctx.obp_increment = (_data & PPU_CGB_PALETTE_INDEX_INC ? true : false);
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
// TODO: reset registers
void GameboyMEM::SetAPUMasterControl(const u8& _data) {
    IO[NR52_ADDR - IO_OFFSET] = (IO[NR52_ADDR - IO_OFFSET] & ~APU_ENABLE_BIT) | (_data & APU_ENABLE_BIT);

    sound_ctx.apuEnable = _data & APU_ENABLE_BIT ? true : false;
}

void GameboyMEM::SetAPUChannelPanning(const u8& _data) {
    IO[NR51_ADDR - IO_OFFSET] = _data;

    sound_ctx.ch1Right.store(_data & 0x01 ? true : false);
    sound_ctx.ch2Right.store(_data & 0x02 ? true : false);
    sound_ctx.ch3Right.store(_data & 0x04 ? true : false);
    sound_ctx.ch4Right.store(_data & 0x08 ? true : false);
    sound_ctx.ch1Left.store(_data & 0x10 ? true : false);
    sound_ctx.ch2Left.store(_data & 0x20 ? true : false);
    sound_ctx.ch3Left.store(_data & 0x40 ? true : false);
    sound_ctx.ch4Left.store(_data & 0x80 ? true : false);
}

void GameboyMEM::SetAPUMasterVolume(const u8& _data) {
    IO[NR50_ADDR - IO_OFFSET] = _data;

    sound_ctx.masterVolumeRight.store((float)VOLUME_MAP.at(_data & MASTER_VOLUME_RIGHT));
    sound_ctx.masterVolumeLeft.store((float)VOLUME_MAP.at((_data & MASTER_VOLUME_LEFT) >> 4));
    sound_ctx.outRightEnabled.store(_data & 0x08 ? true : false);
    sound_ctx.outLeftEnabled.store(_data & 0x80 ? true : false);
}

// channel 1
void GameboyMEM::SetAPUCh1Sweep(const u8& _data) {
    IO[NR10_ADDR - IO_OFFSET] = _data;

    sound_ctx.ch1SweepPace = (_data & CH1_SWEEP_PACE) >> 4;
    sound_ctx.ch1SweepDirSubtract = (_data & CH1_SWEEP_DIR ? true : false);
    sound_ctx.ch1SweepPeriodStep = _data & CH1_SWEEP_STEP;
}

void GameboyMEM::SetAPUCh1TimerDutyCycle(const u8& _data) {
    IO[NR11_ADDR - IO_OFFSET] = _data;

    sound_ctx.ch1DutyCycleIndex.store((_data & CH_1_2_DUTY_CYCLE) >> 6);
    sound_ctx.ch1LengthTimer = _data & CH_1_2_4_LENGTH_TIMER;
    sound_ctx.ch1LengthAltered = true;
}

void GameboyMEM::SetAPUCh1Envelope(const u8& _data) {
    IO[NR12_ADDR - IO_OFFSET] = _data;

    sound_ctx.ch1EnvelopeVolume = (_data & CH_1_2_4_ENV_VOLUME) >> 4;
    sound_ctx.ch1EnvelopeIncrease = (_data & CH_1_2_4_ENV_DIR ? true : false);
    sound_ctx.ch1EnvelopePace = _data & CH_1_2_4_ENV_PACE;

    sound_ctx.ch1Volume.store((float)sound_ctx.ch1EnvelopeVolume / 0xF);

    sound_ctx.ch1DAC = _data & 0xF8 ? true : false;
    if (!sound_ctx.ch1DAC) {
        sound_ctx.ch1Enable.store(false);
        IO[NR52_ADDR - IO_OFFSET] &= ~CH_1_ENABLE;
    }
}

void GameboyMEM::SetAPUCh1PeriodLow(const u8& _data) {
    IO[NR13_ADDR - IO_OFFSET] = _data;

    sound_ctx.ch1Period = _data | (((u16)IO[NR14_ADDR - IO_OFFSET] & CH_1_2_3_PERIOD_HIGH) << 8);
    sound_ctx.ch1SamplingRate.store((float)(pow(2, 20) / (CH_1_2_3_PERIOD_FLIP - sound_ctx.ch1Period)));
    //LOG_INFO("f = ", sound_ctx.ch1SamplingRate.load() / pow(2, 3));
}

void GameboyMEM::SetAPUCh1PeriodHighControl(const u8& _data) {
    IO[NR14_ADDR - IO_OFFSET] = _data;

    if (_data & CH_1_2_3_4_CTRL_TRIGGER && sound_ctx.ch1DAC) {
        sound_ctx.ch1Enable.store(true);
        IO[NR52_ADDR - IO_OFFSET] |= CH_1_ENABLE;
    }
    sound_ctx.ch1LengthEnable = _data & CH_1_2_3_4_CTRL_LENGTH_EN ? true : false;

    sound_ctx.ch1Period = (((u16)_data & CH_1_2_3_PERIOD_HIGH) << 8) | IO[NR13_ADDR - IO_OFFSET];
    sound_ctx.ch1SamplingRate.store((float)(pow(2, 20) / (CH_1_2_3_PERIOD_FLIP - sound_ctx.ch1Period)));
    //LOG_INFO("f = ", sound_ctx.ch1SamplingRate.load() / pow(2, 3), "; length: ", sound_ctx.ch1LengthEnable ? "true" : "false");
}

// channel 2
void GameboyMEM::SetAPUCh2TimerDutyCycle(const u8& _data) {
    IO[NR21_ADDR - IO_OFFSET] = _data;

    sound_ctx.ch2DutyCycleIndex.store((_data & CH_1_2_DUTY_CYCLE) >> 6);
    sound_ctx.ch2LengthTimer = _data & CH_1_2_4_LENGTH_TIMER;
    sound_ctx.ch2LengthAltered = true;
}

void GameboyMEM::SetAPUCh2Envelope(const u8& _data) {
    IO[NR22_ADDR - IO_OFFSET] = _data;

    sound_ctx.ch2EnvelopeVolume = (_data & CH_1_2_4_ENV_VOLUME) >> 4;
    sound_ctx.ch2EnvelopeIncrease = (_data & CH_1_2_4_ENV_DIR ? true : false);
    sound_ctx.ch2EnvelopePace = _data & CH_1_2_4_ENV_PACE;

    sound_ctx.ch2Volume.store((float)sound_ctx.ch2EnvelopeVolume / 0xF);

    sound_ctx.ch2DAC = _data & 0xF8 ? true : false;
    if (!sound_ctx.ch2DAC) {
        sound_ctx.ch2Enable.store(false);
        IO[NR52_ADDR - IO_OFFSET] &= ~CH_2_ENABLE;
    }
}

void GameboyMEM::SetAPUCh2PeriodLow(const u8& _data) {
    IO[NR23_ADDR - IO_OFFSET] = _data;

    sound_ctx.ch2Period = _data | (((u16)IO[NR24_ADDR - IO_OFFSET] & CH_1_2_3_PERIOD_HIGH) << 8);
    sound_ctx.ch2SamplingRate.store((float)(pow(2, 20) / (CH_1_2_3_PERIOD_FLIP - sound_ctx.ch2Period)));
    //LOG_INFO("f = ", sound_ctx.ch1SamplingRate.load() / pow(2, 3));
}

void GameboyMEM::SetAPUCh2PeriodHighControl(const u8& _data) {
    IO[NR24_ADDR - IO_OFFSET] = _data;

    if (_data & CH_1_2_3_4_CTRL_TRIGGER && sound_ctx.ch2DAC) {
        sound_ctx.ch2Enable.store(true);
        IO[NR52_ADDR - IO_OFFSET] |= CH_2_ENABLE;
    }
    sound_ctx.ch2LengthEnable = _data & CH_1_2_3_4_CTRL_LENGTH_EN ? true : false;

    sound_ctx.ch2Period = (((u16)_data & CH_1_2_3_PERIOD_HIGH) << 8) | IO[NR23_ADDR - IO_OFFSET];
    sound_ctx.ch2SamplingRate.store((float)(pow(2, 20) / (CH_1_2_3_PERIOD_FLIP - sound_ctx.ch2Period)));
    //LOG_INFO("f = ", sound_ctx.ch1SamplingRate.load() / pow(2, 3), "; length: ", sound_ctx.ch1LengthEnable ? "true" : "false");
}

void GameboyMEM::SetAPUCh3DACEnable(const u8& _data) {
    IO[NR30_ADDR - IO_OFFSET] = _data;

    sound_ctx.ch3DAC = _data & CH_3_DAC ? true : false;
    if (!sound_ctx.ch3DAC) {
        sound_ctx.ch3Enable.store(false);
        IO[NR52_ADDR - IO_OFFSET] &= ~CH_3_ENABLE;
    }
}

void GameboyMEM::SetAPUCh3Timer(const u8& _data) {
    IO[NR31_ADDR - IO_OFFSET] = _data;

    sound_ctx.ch3LengthTimer = _data;
    sound_ctx.ch3LengthAltered = true;
}

void GameboyMEM::SetAPUCh3Volume(const u8& _data) {
    IO[NR32_ADDR - IO_OFFSET] = _data;

    switch (_data & CH_3_VOLUME) {
    case 0x00:
        sound_ctx.ch3Volume.store(VOLUME_MAP.at(8));
        break;
    case 0x20:
        sound_ctx.ch3Volume.store(VOLUME_MAP.at(7));
        break;
    case 0x40:
        sound_ctx.ch3Volume.store(VOLUME_MAP.at(3));
        break;
    case 0x60:
        sound_ctx.ch3Volume.store(VOLUME_MAP.at(1));
        break;
    }
}

void GameboyMEM::SetAPUCh3PeriodLow(const u8& _data) {
    IO[NR33_ADDR - IO_OFFSET] = _data;

    sound_ctx.ch3Period = _data | (((u16)IO[NR34_ADDR - IO_OFFSET] & CH_1_2_3_PERIOD_HIGH) << 8);
    sound_ctx.ch3SamplingRate.store((float)(pow(2, 21) / (CH_1_2_3_PERIOD_FLIP - sound_ctx.ch2Period)));
}

void GameboyMEM::SetAPUCh3PeriodHighControl(const u8& _data) {
    IO[NR34_ADDR - IO_OFFSET] = _data;

    if (_data & CH_1_2_3_4_CTRL_TRIGGER && sound_ctx.ch3DAC) {
        sound_ctx.ch3Enable.store(true);
        IO[NR52_ADDR - IO_OFFSET] |= CH_3_ENABLE;
    }
    sound_ctx.ch3LengthEnable = _data & CH_1_2_3_4_CTRL_LENGTH_EN ? true : false;

    sound_ctx.ch3Period = (((u16)_data & CH_1_2_3_PERIOD_HIGH) << 8) | IO[NR33_ADDR - IO_OFFSET];
    sound_ctx.ch3SamplingRate.store((float)(pow(2, 21) / (CH_1_2_3_PERIOD_FLIP - sound_ctx.ch3Period)));
}

void GameboyMEM::SetAPUCh3WaveRam(const u16& _addr, const u8& _data) {
    IO[_addr - IO_OFFSET] = _data;

    int index = (_addr - WAVE_RAM_ADDR) << 1;
    unique_lock<mutex> lock_wave_ram(sound_ctx.mutWaveRam);
    sound_ctx.waveRam[index] = ((float)(_data >> 4) / 0x7) - 1.f;
    sound_ctx.waveRam[index + 1] = ((float)(_data & 0xF) / 0x7) - 1.f;
}

void GameboyMEM::SetAPUCh4Timer(const u8& _data) {
    IO[NR41_ADDR - IO_OFFSET] = _data;

    sound_ctx.ch4LengthTimer = _data & CH_1_2_4_LENGTH_TIMER;
    sound_ctx.ch4LengthAltered = true;
}

void GameboyMEM::SetAPUCh4Envelope(const u8& _data) {
    IO[NR42_ADDR - IO_OFFSET] = _data;

    sound_ctx.ch4EnvelopeVolume = (_data & CH_1_2_4_ENV_VOLUME) >> 4;
    sound_ctx.ch4EnvelopeIncrease = (_data & CH_1_2_4_ENV_DIR ? true : false);
    sound_ctx.ch4EnvelopePace = _data & CH_1_2_4_ENV_PACE;

    sound_ctx.ch4Volume.store((float)sound_ctx.ch4EnvelopeVolume / 0xF);

    sound_ctx.ch4DAC = _data & 0xF8 ? true : false;
    if (!sound_ctx.ch4DAC) {
        sound_ctx.ch4Enable.store(false);
        IO[NR52_ADDR - IO_OFFSET] &= ~CH_4_ENABLE;
    }
}

void GameboyMEM::SetAPUCh4FrequRandomness(const u8& _data) {
    IO[NR43_ADDR - IO_OFFSET] = _data;

    sound_ctx.ch4LFSRWidth7Bit = _data & CH_4_LFSR_WIDTH ? true : false;

    int ch4_clock_shift = (_data & CH_4_CLOCK_SHIFT) >> 4;
    int t = _data & CH_4_CLOCK_DIVIDER;
    float ch4_clock_divider = t ? (float)t : .5f;
    sound_ctx.ch4SamplingRate.store((float)(pow(2, 18) / (ch4_clock_divider * pow(2, ch4_clock_shift))));

    sound_ctx.ch4LFSRStep = (float)(sound_ctx.ch4SamplingRate / BASE_CLOCK_CPU);
}

void GameboyMEM::SetAPUCh4Control(const u8& _data) {
    IO[NR44_ADDR - IO_OFFSET] = _data;

    if (_data & CH_1_2_3_4_CTRL_TRIGGER && sound_ctx.ch4DAC) {
        sound_ctx.ch4LFSR = CH_4_LFSR_INIT_VALUE;
        sound_ctx.ch4Enable.store(true);
        IO[NR52_ADDR - IO_OFFSET] |= CH_4_ENABLE;
    }
    sound_ctx.ch4LengthEnable = _data & CH_1_2_3_4_CTRL_LENGTH_EN ? true : false;
}

/* ***********************************************************************************************************
    MEMORY DEBUGGER
*********************************************************************************************************** */
void GameboyMEM::FillMemoryDebugTable(TableSection<memory_entry>& _table_section, u8* _bank_data, const int& _offset, const size_t& _size) {
    auto data = memory_entry();

    int size = (int)_size;
    int line_num = (int)LINE_NUM_HEX(size);
    int index;

    _table_section = TableSection<memory_entry>(line_num);

    for (int i = 0; i < line_num; i++) {
        auto table_entry = TableEntry<memory_entry>();

        index = i * DEBUG_MEM_ELEM_PER_LINE;
        get<ST_ENTRY_ADDRESS>(table_entry) = _offset + index;

        data = memory_entry();
        get<MEM_ENTRY_ADDR>(data) = format("{:04x}", _offset + index);
        get<MEM_ENTRY_LEN>(data) = size - index > DEBUG_MEM_ELEM_PER_LINE - 1 ? DEBUG_MEM_ELEM_PER_LINE : size % DEBUG_MEM_ELEM_PER_LINE;
        get<MEM_ENTRY_REF>(data) = &_bank_data[index];

        get<ST_ENTRY_DATA>(table_entry) = data;

        _table_section[i] = table_entry;
    }
}

void GameboyMEM::GetMemoryDebugTables(std::vector<Table<memory_entry>>& _tables) {
    // access for memory inspector
    _tables.clear();

    // ROM
    {
        // ROM 0
        auto table = Table<memory_entry>(DEBUG_MEM_LINES);
        table.name = "ROM";
        auto table_section = TableSection<memory_entry>();

        FillMemoryDebugTable(table_section, ROM_0.data(), ROM_0_OFFSET, ROM_0.size());

        table.AddTableSectionDisposable(table_section);

        // ROM n
        for (auto& n : ROM_N) {
            table_section = TableSection<memory_entry>();

            FillMemoryDebugTable(table_section, n.data(), ROM_0_OFFSET, n.size());

            table.AddTableSectionDisposable(table_section);
        }

        _tables.push_back(table);
    }

    // VRAM
    {
        auto table = Table<memory_entry>(DEBUG_MEM_LINES);
        table.name = "VRAM";
        auto table_section = TableSection<memory_entry>();

        for (auto& n : graphics_ctx.VRAM_N) {
            table_section = TableSection<memory_entry>();

            FillMemoryDebugTable(table_section, n.data(), VRAM_N_OFFSET, n.size());

            table.AddTableSectionDisposable(table_section);
        }

        _tables.push_back(table);
    }

    // RAM
    if (machine_ctx.ram_bank_num > 0) {
        auto table = Table<memory_entry>(DEBUG_MEM_LINES);
        table.name = "RAM";
        auto table_section = TableSection<memory_entry>();

        for (auto& n : RAM_N) {
            table_section = TableSection<memory_entry>();

            FillMemoryDebugTable(table_section, n, RAM_N_OFFSET, RAM_N_SIZE);

            table.AddTableSectionDisposable(table_section);
        }

        _tables.push_back(table);
    }

    // WRAM
    {
        auto table = Table<memory_entry>(DEBUG_MEM_LINES);
        table.name = "WRAM";
        auto table_section = TableSection<memory_entry>();

        FillMemoryDebugTable(table_section, WRAM_0.data(), WRAM_0_OFFSET, WRAM_0.size());

        table.AddTableSectionDisposable(table_section);

        // WRAM n
        for (auto& n : WRAM_N) {
            table_section = TableSection<memory_entry>();

            FillMemoryDebugTable(table_section, n.data(), WRAM_N_OFFSET, n.size());

            table.AddTableSectionDisposable(table_section);
        }

        _tables.push_back(table);
    }

    // OAM
    {
        auto table = Table<memory_entry>(DEBUG_MEM_LINES);
        table.name = "OAM";
        auto table_section = TableSection<memory_entry>();

        FillMemoryDebugTable(table_section, graphics_ctx.OAM.data(), OAM_OFFSET, graphics_ctx.OAM.size());

        table.AddTableSectionDisposable(table_section);

        _tables.push_back(table);
    }

    // IO
    {
        auto table = Table<memory_entry>(DEBUG_MEM_LINES);
        table.name = "IO";
        auto table_section = TableSection<memory_entry>();

        FillMemoryDebugTable(table_section, IO.data(), IO_OFFSET, IO.size());

        table.AddTableSectionDisposable(table_section);

        _tables.push_back(table);
    }

    // HRAM
    {
        auto table = Table<memory_entry>(DEBUG_MEM_LINES);
        table.name = "HRAM";
        auto table_section = TableSection<memory_entry>();

        FillMemoryDebugTable(table_section, HRAM.data(), HRAM_OFFSET, HRAM.size());

        table.AddTableSectionDisposable(table_section);

        _tables.push_back(table);
    }
}

vector<u8>* GameboyMEM::GetProgramData(const int& _bank) const {
    if (_bank == 0) {
        return (vector<u8>*)&ROM_0;
    } else if (_bank <= (int)ROM_N.size()) {
        return (vector<u8>*)&ROM_N[_bank - 1];
    } else {
        return nullptr;
    }
}