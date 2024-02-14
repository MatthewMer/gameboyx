#include "GameboyCPU.h"

/* ***********************************************************************************************************
    INCLUDES
*********************************************************************************************************** */
#include "gameboy_defines.h"
#include <format>
#include "logger.h"
#include "GameboyMEM.h"
#include "GuiTable.h"
#include <unordered_map>

#include <iostream>

using namespace std;

/* ***********************************************************************************************************
    MISC LOOKUPS
*********************************************************************************************************** */
enum instr_lookup_enums {
    INSTR_OPCODE,
    INSTR_FUNC,
    INSTR_MNEMONIC,
    INSTR_ARG_1,
    INSTR_ARG_2
};

const unordered_map<cgb_flag_types, string> FLAG_NAMES{
    {FLAG_Z, "ZERO"},
    {FLAG_N, "SUB"},
    {FLAG_H, "HALFCARRY"},
    {FLAG_C, "CARRY"},
    {FLAG_IME, "IME"},
    {INT_VBLANK, "VBLANK"},
    {INT_STAT, "STAT"},
    {INT_TIMER, "TIMER"},
    {INT_SERIAL, "SERIAL"},
    {INT_JOYPAD, "JOYPAD"}
};

const unordered_map<cgb_data_types, string> REGISTER_NAMES{
    {AF, "AF"},
    {A, "A"},
    {F, "F"},
    {BC, "BC"},
    {B, "B"},
    {C, "C"},
    {DE, "DE"},
    {D, "D"},
    {E, "E"},
    {HL, "HL"},
    {H, "H"},
    {L, "L"},
    {SP, "SP"},
    {PC, "PC"},
    {IE, "IE"},
    {IF, "IF"}
};

const unordered_map<cgb_data_types, string> DATA_NAMES{
    {d8, "d8"},
    {d16, "d16"},
    {a8, "a8"},
    {a16, "a16"},
    {r8, "r8"},
    {a8_ref, "(a8)"},
    {a16_ref, "(a16)"},
    {BC_ref, "(BC)"},
    {DE_ref, "(DE)"},
    {HL_ref, "(HL)"},
    {HL_INC_ref, "(HL+)"},
    {HL_DEC_ref, "(HL-)"},
    {SP_r8, "SP r8"},
    {C_ref, "(FF00+C)"},
};

/* ***********************************************************************************************************
    ACCESS CPU STATUS
*********************************************************************************************************** */
// return clock cycles per second
u32 GameboyCPU::GetCurrentClockCycles() {
    u32 result = tickCounter;
    tickCounter = 0;
    return result;
}

void GameboyCPU::GetBankAndPC(int& _bank, u32& _pc) const {
    _pc = (u32)Regs.PC;

    if (_pc < ROM_N_OFFSET) {
        _bank = 0;
    } else if (_pc < VRAM_N_OFFSET) {
        _bank = machine_ctx->rom_bank_selected + 1;
    } else {
        _bank = -1;
    }
}

/* ***********************************************************************************************************
    CONSTRUCTOR
*********************************************************************************************************** */
GameboyCPU::GameboyCPU(BaseCartridge* _cartridge) : BaseCPU(_cartridge) {
    mem_instance = (GameboyMEM*)BaseMEM::getInstance();
    machine_ctx = mem_instance->GetMachineContext();
    graphics_ctx = mem_instance->GetGraphicsContext();
    sound_ctx = mem_instance->GetSoundContext();

    InitRegisterStates();

    setupLookupTable();
    setupLookupTableCB();
}

void GameboyCPU::SetInstances() {
    graphics_instance = BaseGPU::getInstance();
    sound_instance = BaseAPU::getInstance();
    ticksPerFrame = graphics_instance->GetTicksPerFrame((float)(BASE_CLOCK_CPU * pow(10, 6)));
}

/* ***********************************************************************************************************
    INIT CPU
*********************************************************************************************************** */
// initial register states
void GameboyCPU::InitRegisterStates() {
    Regs = registers();

    Regs.SP = INIT_SP;
    Regs.PC = INIT_PC;

    if (machine_ctx->isCgb) {
        Regs.A = (INIT_CGB_AF & 0xFF00) >> 8;
        Regs.F = INIT_CGB_AF & 0xFF;

        Regs.BC = INIT_CGB_BC;
        Regs.DE = INIT_CGB_DE;
        Regs.HL = INIT_CGB_HL;
    }
    else {
        Regs.A = (INIT_DMG_AF & 0xFF00) >> 8;
        Regs.F = INIT_DMG_AF & 0xFF;

        Regs.BC = INIT_DMG_BC;
        Regs.DE = INIT_DMG_DE;
        Regs.HL = INIT_DMG_HL;
    }
}

/* ***********************************************************************************************************
    RUN CPU
*********************************************************************************************************** */
void GameboyCPU::RunCycles() {
    currentTicks = 0;

    while ((currentTicks < (ticksPerFrame * machine_ctx->currentSpeed))) {
        if (stopped) {
            // check button press
            if (mem_instance->GetIO(IF_ADDR) & IRQ_JOYPAD) {
                stopped = false;
            } else {
                return;
            }
            
        } else if (halted) {
            TickTimers();

            // check pending and enabled interrupts
            if (machine_ctx->IE & mem_instance->GetIO(IF_ADDR)) {
                halted = false;
            }
        } else {
            CheckInterrupts();
            ExecuteInstruction();
        }
    }

    tickCounter += currentTicks;
}

void GameboyCPU::RunCycle() {
    currentTicks = 0;

    if (stopped) {
        // check button press
        if (mem_instance->GetIO(IF_ADDR) & IRQ_JOYPAD) {
            stopped = false;
        } else {
            return;
        }

    } else if (halted) {
        while (halted) {
            // exit loop after ticks per frame have passed, otherwise application would just stall forever in case no interrupts have been enabled
            if (currentTicks >= ticksPerFrame) { break; }

            TickTimers();

            // check pending and enabled interrupts
            if (machine_ctx->IE & mem_instance->GetIO(IF_ADDR)) {
                halted = false;
            }
        }
    } else {
        if (!CheckInterrupts()) {
            ExecuteInstruction();
        }
    }

    tickCounter += currentTicks;
}

void GameboyCPU::ExecuteInstruction() {
    curPC = Regs.PC;
    FetchOpCode();

    if (opcode == 0xCB) {
        FetchOpCode();
        instrPtr = &instrMapCB[opcode];
    }
    else {
        instrPtr = &instrMap[opcode];
    }

    functionPtr = get<INSTR_FUNC>(*instrPtr);
    (this->*functionPtr)();
}

bool GameboyCPU::CheckInterrupts() {
    if (ime) {
        u8& isr_requested = mem_instance->GetIO(IF_ADDR);
        if ((isr_requested & IRQ_VBLANK) && (machine_ctx->IE & IRQ_VBLANK)) {
            ime = false;

            isr_push(ISR_VBLANK_HANDLER_ADDR);
            isr_requested &= ~IRQ_VBLANK;
            return true;
        }
        else if ((isr_requested & IRQ_LCD_STAT) && (machine_ctx->IE & IRQ_LCD_STAT)) {
            ime = false;

            isr_push(ISR_LCD_STAT_HANDLER_ADDR);
            isr_requested &= ~IRQ_LCD_STAT;
            return true;
        }
        else if ((isr_requested & IRQ_TIMER) && (machine_ctx->IE & IRQ_TIMER)) {
            ime = false;

            isr_push(ISR_TIMER_HANDLER_ADDR);
            isr_requested &= ~IRQ_TIMER;
            return true;
        }
        /*if (machine_ctx->IF & IRQ_SERIAL) {
            // not implemented
        }*/
        else if ((isr_requested & IRQ_JOYPAD) && (machine_ctx->IE & IRQ_JOYPAD)) {
            ime = false;

            isr_push(ISR_JOYPAD_HANDLER_ADDR);
            isr_requested &= ~IRQ_JOYPAD;
            return true;
        }
    }
    return false;
}

void GameboyCPU::TickTimers() {
    bool div_low_byte_selected = machine_ctx->timaDivMask < 0x100;
    u8& div = mem_instance->GetIO(DIV_ADDR);
    bool tima_enabled = mem_instance->GetIO(TAC_ADDR) & TAC_CLOCK_ENABLE;

    if (machine_ctx->tima_reload_cycle) {
        div = mem_instance->GetIO(TMA_ADDR);
        if (!machine_ctx->tima_reload_if_write) {
            mem_instance->RequestInterrupts(IRQ_TIMER);
        } else {
            machine_ctx->tima_reload_if_write = false;
        }
        machine_ctx->tima_reload_cycle = false;
    } else if (machine_ctx->tima_overflow_cycle) {
        machine_ctx->tima_reload_cycle = true;
        machine_ctx->tima_overflow_cycle = false;
    }

    for (int i = 0; i < 4; i++) {
        if (machine_ctx->div_low_byte == 0xFF) {
            machine_ctx->div_low_byte = 0x00;

            if (div == 0xFF) {
                div = 0x00;
            } else {
                div++;
            }

            //sound_ctx->divApuBitHigh = (div & sound_ctx->divApuBitMask ? true : false);
        } else {
            machine_ctx->div_low_byte++;
        }

        if (div_low_byte_selected) {
            timaEnAndDivOverflowCur = tima_enabled && (machine_ctx->div_low_byte & machine_ctx->timaDivMask ? true : false);
        } else {
            timaEnAndDivOverflowCur = tima_enabled && (div & (machine_ctx->timaDivMask >> 8) ? true : false);
        }

        if (!timaEnAndDivOverflowCur && timaEnAndDivOverflowPrev) {
            IncrementTIMA();
        }
        timaEnAndDivOverflowPrev = timaEnAndDivOverflowCur;

        /*  Would be more precise, but not necessary
        if (sound_ctx->divApuBitWasHigh && !sound_ctx->divApuBitHigh) {
            sound_instance->ProcessAPU(1);
        }
        */
    }

    currentTicks += TICKS_PER_MC;

    // peripherals directly bound to CPUs timer system (master clock) (PPU not affected by speed mode)
    int ticks = TICKS_PER_MC / machine_ctx->currentSpeed;
    graphics_instance->ProcessGPU(ticks);
    sound_instance->ProcessAPU(ticks);
}

void GameboyCPU::IncrementTIMA() {
    u8& tima = mem_instance->GetIO(TIMA_ADDR);
    if (tima == 0xFF) {
        tima = 0x00;
        machine_ctx->tima_overflow_cycle = true;
    }
    else {
        tima++;
    }
}

void GameboyCPU::FetchOpCode() {
    opcode = mmu_instance->Read8Bit(Regs.PC);
    Regs.PC++;
    TickTimers();
}

void GameboyCPU::Fetch8Bit() {
    data = mmu_instance->Read8Bit(Regs.PC);
    Regs.PC++;
    TickTimers();
}

void GameboyCPU::Fetch16Bit() {
    data = mmu_instance->Read8Bit(Regs.PC);
    Regs.PC++;
    TickTimers();

    data |= (((u16)mmu_instance->Read8Bit(Regs.PC)) << 8);
    Regs.PC++;
    TickTimers();
}

void GameboyCPU::Write8Bit(const u8& _data, const u16& _addr) {
    mmu_instance->Write8Bit(_data, _addr);
    TickTimers();
}

void GameboyCPU::Write16Bit(const u16& _data, const u16& _addr) {
    mmu_instance->Write8Bit((_data >> 8) & 0xFF, _addr + 1);
    TickTimers();
    mmu_instance->Write8Bit(_data & 0xFF, _addr);
    TickTimers();
}

u8 GameboyCPU::Read8Bit(const u16& _addr) {
    u8 data = mmu_instance->Read8Bit(_addr);
    TickTimers();
    return data;
}

u16 GameboyCPU::Read16Bit(const u16& _addr) {
    u16 data = mmu_instance->Read8Bit(_addr);
    TickTimers();
    data |= (((u16)mmu_instance->Read8Bit(_addr + 1)) << 8);
    TickTimers();
    return data;
}

/* ***********************************************************************************************************
    FLAG/BIT DEFINES
*********************************************************************************************************** */
#define FLAG_CARRY			0x10
#define FLAG_HCARRY			0x20
#define FLAG_SUB			0x40
#define FLAG_ZERO			0x80

#define MSB                 0x80
#define LSB                 0x01

/* ***********************************************************************************************************
    FLAG SET DEFINES
*********************************************************************************************************** */
#define RESET_FLAGS(flags, f) {f &= ~(flags);}
#define SET_FLAGS(flags, f) {f |= (flags);}
#define RESET_ALL_FLAGS(f) {f = 0x00;}

#define ADD_8_FLAGS(d, s, f) {f &= ~(FLAG_CARRY | FLAG_HCARRY); f |= (((u16)(d & 0xFF) + (s & 0xFF)) > 0xFF ? FLAG_CARRY : 0); f |= (((d & 0xF) + (s & 0xF)) > 0xF ? FLAG_HCARRY : 0);}
#define ADD_8_C(d, s, f) {f &= ~FLAG_CARRY; f |= (((u16)(d & 0xFF) + (s & 0xFF)) > 0xFF ? FLAG_CARRY : 0);}
#define ADD_8_HC(d, s, f) {f &= ~FLAG_HCARRY; f |= (((d & 0xF) + (s & 0xF)) > 0xF ? FLAG_HCARRY : 0);}

#define ADC_8_C(d, s, c, f) {f &= ~FLAG_CARRY; f |= ((((u16)(d & 0xFF)) + ((u16)(s & 0xFF)) + c) > 0xFF ? FLAG_CARRY : 0);}
#define ADC_8_HC(d, s, c, f) {f &= ~FLAG_HCARRY; f |= (((d & 0xF) + (s & 0xF) + c) > 0xF ? FLAG_HCARRY : 0);}

#define ADD_16_FLAGS(d, s, f) {f &= ~(FLAG_CARRY | FLAG_HCARRY); f |= ((((u32)(d & 0xFFFF)) + (s & 0xFFFF)) > 0xFFFF ? FLAG_CARRY : 0); f |= ((((u16)(d & 0xFFF)) + (s & 0xFFF)) > 0xFFF ? FLAG_HCARRY : 0);}

#define SUB_8_FLAGS(d, s, f) {f &= ~(FLAG_CARRY | FLAG_HCARRY); f |= (((d & 0xFF) < (s & 0xFF)) ? FLAG_CARRY : 0); f |= (((d & 0xF) < (s & 0xF)) ? FLAG_HCARRY : 0);}
#define SUB_8_C(d, s, f) {f &= ~FLAG_CARRY; f |= (((d & 0xFF) < (s & 0xFF)) ? FLAG_CARRY : 0);}
#define SUB_8_HC(d, s, f) {f &= ~FLAG_HCARRY; f|= (((d & 0xF) < (s & 0xF)) ? FLAG_HCARRY : 0);}

#define SBC_8_C(d, s, c, f) {f &= ~FLAG_CARRY; f |= ((((u16)(d & 0xFF)) < (((u16)s & 0xFF) + c)) ? FLAG_CARRY : 0);}
#define SBC_8_HC(d, s, c, f) {f &= ~FLAG_HCARRY; f |= (((d & 0xF) < ((s & 0xF) + c)) ? FLAG_HCARRY : 0);}

#define ZERO_FLAG(x, f) {f &= ~FLAG_ZERO; f |= (((x) == 0) ? FLAG_ZERO : 0x00);}

/* ***********************************************************************************************************
*
*   GAMEBOY (COLOR) GameboyCPU INSTRUCTION LOOKUP TABLE
*
*********************************************************************************************************** */
void GameboyCPU::setupLookupTable() {
    instrMap.clear();

    // Elements: opcode, instruction function, machine cycles, bytes (without opcode), instr_info

    // 0x00
    instrMap.emplace_back(0x00, &GameboyCPU::NOP, "NOP", NO_DATA, NO_DATA);
    instrMap.emplace_back(0x01, &GameboyCPU::LDd16, "LD", BC, d16);
    instrMap.emplace_back(0x02, &GameboyCPU::LDfromAtoRef, "LD", BC_ref, A);
    instrMap.emplace_back(0x03, &GameboyCPU::INC16, "INC", BC, NO_DATA);
    instrMap.emplace_back(0x04, &GameboyCPU::INC8, "INC", B, NO_DATA);
    instrMap.emplace_back(0x05, &GameboyCPU::DEC8, "DEC", B, NO_DATA);
    instrMap.emplace_back(0x06, &GameboyCPU::LDd8, "LD", B, d8);
    instrMap.emplace_back(0x07, &GameboyCPU::RLCA, "RLCA", A, NO_DATA);
    instrMap.emplace_back(0x08, &GameboyCPU::LDSPa16, "LD", a16_ref, SP);
    instrMap.emplace_back(0x09, &GameboyCPU::ADDHL, "ADD", HL, BC);
    instrMap.emplace_back(0x0a, &GameboyCPU::LDtoAfromRef, "LD", A, BC_ref);
    instrMap.emplace_back(0x0b, &GameboyCPU::DEC16, "DEC", BC, NO_DATA);
    instrMap.emplace_back(0x0c, &GameboyCPU::INC8, "INC", C, NO_DATA);
    instrMap.emplace_back(0x0d, &GameboyCPU::DEC8, "DEC", C, NO_DATA);
    instrMap.emplace_back(0x0e, &GameboyCPU::LDd8, "LD", C, d8);
    instrMap.emplace_back(0x0f, &GameboyCPU::RRCA, "RRCA", A, NO_DATA);

    // 0x10
    instrMap.emplace_back(0x10, &GameboyCPU::STOP, "STOP", d8, NO_DATA);
    instrMap.emplace_back(0x11, &GameboyCPU::LDd16, "LD", DE, d16);
    instrMap.emplace_back(0x12, &GameboyCPU::LDfromAtoRef, "LD", DE_ref, A);
    instrMap.emplace_back(0x13, &GameboyCPU::INC16, "INC", DE, NO_DATA);
    instrMap.emplace_back(0x14, &GameboyCPU::INC8, "INC", D, NO_DATA);
    instrMap.emplace_back(0x15, &GameboyCPU::DEC8, "DEC", D, NO_DATA);
    instrMap.emplace_back(0x16, &GameboyCPU::LDd8, "LD", D, d8);
    instrMap.emplace_back(0x17, &GameboyCPU::RLA, "RLA", A, NO_DATA);
    instrMap.emplace_back(0x18, &GameboyCPU::JR, "JR", r8, NO_DATA);
    instrMap.emplace_back(0x19, &GameboyCPU::ADDHL, "ADD", HL, DE);
    instrMap.emplace_back(0x1a, &GameboyCPU::LDtoAfromRef, "LD", A, DE_ref);
    instrMap.emplace_back(0x1b, &GameboyCPU::DEC16, "DEC", DE, NO_DATA);
    instrMap.emplace_back(0x1c, &GameboyCPU::INC8, "INC", E, NO_DATA);
    instrMap.emplace_back(0x1d, &GameboyCPU::DEC8, "DEC", E, NO_DATA);
    instrMap.emplace_back(0x1e, &GameboyCPU::LDd8, "LD", E, d8);
    instrMap.emplace_back(0x1f, &GameboyCPU::RRA, "RRA", A, NO_DATA);

    // 0x20
    instrMap.emplace_back(0x20, &GameboyCPU::JR, "JR NZ", r8, NO_DATA);
    instrMap.emplace_back(0x21, &GameboyCPU::LDd16, "LD", HL, d16);
    instrMap.emplace_back(0x22, &GameboyCPU::LDfromAtoRef, "LD", HL_INC_ref, A);
    instrMap.emplace_back(0x23, &GameboyCPU::INC16, "INC", HL, NO_DATA);
    instrMap.emplace_back(0x24, &GameboyCPU::INC8, "INC", H, NO_DATA);
    instrMap.emplace_back(0x25, &GameboyCPU::DEC8, "DEC", H, NO_DATA);
    instrMap.emplace_back(0x26, &GameboyCPU::LDd8, "LD", H, d8);
    instrMap.emplace_back(0x27, &GameboyCPU::DAA, "DAA", A, NO_DATA);
    instrMap.emplace_back(0x28, &GameboyCPU::JR, "JR Z", r8, NO_DATA);
    instrMap.emplace_back(0x29, &GameboyCPU::ADDHL, "ADD", HL, HL);
    instrMap.emplace_back(0x2a, &GameboyCPU::LDtoAfromRef, "LD", A, HL_INC_ref);
    instrMap.emplace_back(0x2b, &GameboyCPU::DEC16, "DEC", HL, NO_DATA);
    instrMap.emplace_back(0x2c, &GameboyCPU::INC8, "INC", L, NO_DATA);
    instrMap.emplace_back(0x2d, &GameboyCPU::DEC8, "DEC", L, NO_DATA);
    instrMap.emplace_back(0x2e, &GameboyCPU::LDd8, "LD", L, NO_DATA);
    instrMap.emplace_back(0x2f, &GameboyCPU::CPL, "CPL", A, NO_DATA);

    // 0x30
    instrMap.emplace_back(0x30, &GameboyCPU::JR, "JR NC", r8, NO_DATA);
    instrMap.emplace_back(0x31, &GameboyCPU::LDd16, "LD", SP, d16);
    instrMap.emplace_back(0x32, &GameboyCPU::LDfromAtoRef, "LD", HL_DEC_ref, A);
    instrMap.emplace_back(0x33, &GameboyCPU::INC16, "INC", SP, NO_DATA);
    instrMap.emplace_back(0x34, &GameboyCPU::INC8, "INC", HL_ref, NO_DATA);
    instrMap.emplace_back(0x35, &GameboyCPU::DEC8, "DEC", HL_ref, NO_DATA);
    instrMap.emplace_back(0x36, &GameboyCPU::LDd8, "LD", HL_ref, d8);
    instrMap.emplace_back(0x37, &GameboyCPU::SCF, "SCF", NO_DATA, NO_DATA);
    instrMap.emplace_back(0x38, &GameboyCPU::JR, "JR C", r8, NO_DATA);
    instrMap.emplace_back(0x39, &GameboyCPU::ADDHL, "ADD", HL, SP);
    instrMap.emplace_back(0x3a, &GameboyCPU::LDtoAfromRef, "LD", A, HL_DEC_ref);
    instrMap.emplace_back(0x3b, &GameboyCPU::DEC16, "DEC", SP, NO_DATA);
    instrMap.emplace_back(0x3c, &GameboyCPU::INC8, "INC", A, NO_DATA);
    instrMap.emplace_back(0x3d, &GameboyCPU::DEC8, "DEC", A, NO_DATA);
    instrMap.emplace_back(0x3e, &GameboyCPU::LDd8, "LD", A, d8);
    instrMap.emplace_back(0x3f, &GameboyCPU::CCF, "CCF", NO_DATA, NO_DATA);

    // 0x40
    instrMap.emplace_back(0x40, &GameboyCPU::LDtoB, "LD", B, B);
    instrMap.emplace_back(0x41, &GameboyCPU::LDtoB, "LD", B, C);
    instrMap.emplace_back(0x42, &GameboyCPU::LDtoB, "LD", B, D);
    instrMap.emplace_back(0x43, &GameboyCPU::LDtoB, "LD", B, E);
    instrMap.emplace_back(0x44, &GameboyCPU::LDtoB, "LD", B, H);
    instrMap.emplace_back(0x45, &GameboyCPU::LDtoB, "LD", B, L);
    instrMap.emplace_back(0x46, &GameboyCPU::LDtoB, "LD", B, HL_ref);
    instrMap.emplace_back(0x47, &GameboyCPU::LDtoB, "LD", B, A);
    instrMap.emplace_back(0x48, &GameboyCPU::LDtoC, "LD", C, B);
    instrMap.emplace_back(0x49, &GameboyCPU::LDtoC, "LD", C, C);
    instrMap.emplace_back(0x4a, &GameboyCPU::LDtoC, "LD", C, D);
    instrMap.emplace_back(0x4b, &GameboyCPU::LDtoC, "LD", C, E);
    instrMap.emplace_back(0x4c, &GameboyCPU::LDtoC, "LD", C, H);
    instrMap.emplace_back(0x4d, &GameboyCPU::LDtoC, "LD", C, L);
    instrMap.emplace_back(0x4e, &GameboyCPU::LDtoC, "LD", C, HL_ref);
    instrMap.emplace_back(0x4f, &GameboyCPU::LDtoC, "LD", C, A);

    // 0x50
    instrMap.emplace_back(0x50, &GameboyCPU::LDtoD, "LD", D, B);
    instrMap.emplace_back(0x51, &GameboyCPU::LDtoD, "LD", D, C);
    instrMap.emplace_back(0x52, &GameboyCPU::LDtoD, "LD", D, D);
    instrMap.emplace_back(0x53, &GameboyCPU::LDtoD, "LD", D, E);
    instrMap.emplace_back(0x54, &GameboyCPU::LDtoD, "LD", D, H);
    instrMap.emplace_back(0x55, &GameboyCPU::LDtoD, "LD", D, L);
    instrMap.emplace_back(0x56, &GameboyCPU::LDtoD, "LD", D, HL_ref);
    instrMap.emplace_back(0x57, &GameboyCPU::LDtoD, "LD", D, A);
    instrMap.emplace_back(0x58, &GameboyCPU::LDtoE, "LD", E, B);
    instrMap.emplace_back(0x59, &GameboyCPU::LDtoE, "LD", E, C);
    instrMap.emplace_back(0x5a, &GameboyCPU::LDtoE, "LD", E, D);
    instrMap.emplace_back(0x5b, &GameboyCPU::LDtoE, "LD", E, E);
    instrMap.emplace_back(0x5c, &GameboyCPU::LDtoE, "LD", E, H);
    instrMap.emplace_back(0x5d, &GameboyCPU::LDtoE, "LD", E, L);
    instrMap.emplace_back(0x5e, &GameboyCPU::LDtoE, "LD", E, HL_ref);
    instrMap.emplace_back(0x5f, &GameboyCPU::LDtoE, "LD", E, A);

    // 0x60
    instrMap.emplace_back(0x60, &GameboyCPU::LDtoH, "LD", H, B);
    instrMap.emplace_back(0x61, &GameboyCPU::LDtoH, "LD", H, C);
    instrMap.emplace_back(0x62, &GameboyCPU::LDtoH, "LD", H, D);
    instrMap.emplace_back(0x63, &GameboyCPU::LDtoH, "LD", H, E);
    instrMap.emplace_back(0x64, &GameboyCPU::LDtoH, "LD", H, H);
    instrMap.emplace_back(0x65, &GameboyCPU::LDtoH, "LD", H, L);
    instrMap.emplace_back(0x66, &GameboyCPU::LDtoH, "LD", H, HL_ref);
    instrMap.emplace_back(0x67, &GameboyCPU::LDtoH, "LD", H, A);
    instrMap.emplace_back(0x68, &GameboyCPU::LDtoL, "LD", L, B);
    instrMap.emplace_back(0x69, &GameboyCPU::LDtoL, "LD", L, C);
    instrMap.emplace_back(0x6a, &GameboyCPU::LDtoL, "LD", L, D);
    instrMap.emplace_back(0x6b, &GameboyCPU::LDtoL, "LD", L, E);
    instrMap.emplace_back(0x6c, &GameboyCPU::LDtoL, "LD", L, H);
    instrMap.emplace_back(0x6d, &GameboyCPU::LDtoL, "LD", L, L);
    instrMap.emplace_back(0x6e, &GameboyCPU::LDtoL, "LD", L, HL_ref);
    instrMap.emplace_back(0x6f, &GameboyCPU::LDtoL, "LD", L, A);

    // 0x70
    instrMap.emplace_back(0x70, &GameboyCPU::LDtoHLref, "LD", HL_ref, B);
    instrMap.emplace_back(0x71, &GameboyCPU::LDtoHLref, "LD", HL_ref, C);
    instrMap.emplace_back(0x72, &GameboyCPU::LDtoHLref, "LD", HL_ref, D);
    instrMap.emplace_back(0x73, &GameboyCPU::LDtoHLref, "LD", HL_ref, E);
    instrMap.emplace_back(0x74, &GameboyCPU::LDtoHLref, "LD", HL_ref, H);
    instrMap.emplace_back(0x75, &GameboyCPU::LDtoHLref, "LD", HL_ref, L);
    instrMap.emplace_back(0x76, &GameboyCPU::HALT, "HALT", NO_DATA, NO_DATA);
    instrMap.emplace_back(0x77, &GameboyCPU::LDtoHLref, "LD", HL_ref, A);
    instrMap.emplace_back(0x78, &GameboyCPU::LDtoA, "LD", A, B);
    instrMap.emplace_back(0x79, &GameboyCPU::LDtoA, "LD", A, C);
    instrMap.emplace_back(0x7a, &GameboyCPU::LDtoA, "LD", A, D);
    instrMap.emplace_back(0x7b, &GameboyCPU::LDtoA, "LD", A, E);
    instrMap.emplace_back(0x7c, &GameboyCPU::LDtoA, "LD", A, H);
    instrMap.emplace_back(0x7d, &GameboyCPU::LDtoA, "LD", A, L);
    instrMap.emplace_back(0x7e, &GameboyCPU::LDtoA, "LD", A, HL_ref);
    instrMap.emplace_back(0x7f, &GameboyCPU::LDtoA, "LD", A, A);

    // 0x80
    instrMap.emplace_back(0x80, &GameboyCPU::ADD8, "ADD", A, B);
    instrMap.emplace_back(0x81, &GameboyCPU::ADD8, "ADD", A, C);
    instrMap.emplace_back(0x82, &GameboyCPU::ADD8, "ADD", A, D);
    instrMap.emplace_back(0x83, &GameboyCPU::ADD8, "ADD", A, E);
    instrMap.emplace_back(0x84, &GameboyCPU::ADD8, "ADD", A, H);
    instrMap.emplace_back(0x85, &GameboyCPU::ADD8, "ADD", A, L);
    instrMap.emplace_back(0x86, &GameboyCPU::ADD8, "ADD", A, HL_ref);
    instrMap.emplace_back(0x87, &GameboyCPU::ADD8, "ADD", A, A);
    instrMap.emplace_back(0x88, &GameboyCPU::ADC, "ADC", A, B);
    instrMap.emplace_back(0x89, &GameboyCPU::ADC, "ADC", A, C);
    instrMap.emplace_back(0x8a, &GameboyCPU::ADC, "ADC", A, D);
    instrMap.emplace_back(0x8b, &GameboyCPU::ADC, "ADC", A, E);
    instrMap.emplace_back(0x8c, &GameboyCPU::ADC, "ADC", A, H);
    instrMap.emplace_back(0x8d, &GameboyCPU::ADC, "ADC", A, L);
    instrMap.emplace_back(0x8e, &GameboyCPU::ADC, "ADC", A, HL_ref);
    instrMap.emplace_back(0x8f, &GameboyCPU::ADC, "ADC", A, A);

    // 0x90
    instrMap.emplace_back(0x90, &GameboyCPU::SUB, "SUB", A, B);
    instrMap.emplace_back(0x91, &GameboyCPU::SUB, "SUB", A, C);
    instrMap.emplace_back(0x92, &GameboyCPU::SUB, "SUB", A, D);
    instrMap.emplace_back(0x93, &GameboyCPU::SUB, "SUB", A, E);
    instrMap.emplace_back(0x94, &GameboyCPU::SUB, "SUB", A, H);
    instrMap.emplace_back(0x95, &GameboyCPU::SUB, "SUB", A, L);
    instrMap.emplace_back(0x96, &GameboyCPU::SUB, "SUB", A, HL_ref);
    instrMap.emplace_back(0x97, &GameboyCPU::SUB, "SUB", A, A);
    instrMap.emplace_back(0x98, &GameboyCPU::SBC, "SBC", A, B);
    instrMap.emplace_back(0x99, &GameboyCPU::SBC, "SBC", A, C);
    instrMap.emplace_back(0x9a, &GameboyCPU::SBC, "SBC", A, D);
    instrMap.emplace_back(0x9b, &GameboyCPU::SBC, "SBC", A, E);
    instrMap.emplace_back(0x9c, &GameboyCPU::SBC, "SBC", A, H);
    instrMap.emplace_back(0x9d, &GameboyCPU::SBC, "SBC", A, L);
    instrMap.emplace_back(0x9e, &GameboyCPU::SBC, "SBC", A, HL_ref);
    instrMap.emplace_back(0x9f, &GameboyCPU::SBC, "SBC", A, A);

    // 0xa0
    instrMap.emplace_back(0xa0, &GameboyCPU::AND, "AND", A, B);
    instrMap.emplace_back(0xa1, &GameboyCPU::AND, "AND", A, C);
    instrMap.emplace_back(0xa2, &GameboyCPU::AND, "AND", A, D);
    instrMap.emplace_back(0xa3, &GameboyCPU::AND, "AND", A, E);
    instrMap.emplace_back(0xa4, &GameboyCPU::AND, "AND", A, H);
    instrMap.emplace_back(0xa5, &GameboyCPU::AND, "AND", A, L);
    instrMap.emplace_back(0xa6, &GameboyCPU::AND, "AND", A, HL_ref);
    instrMap.emplace_back(0xa7, &GameboyCPU::AND, "AND", A, A);
    instrMap.emplace_back(0xa8, &GameboyCPU::XOR, "XOR", A, B);
    instrMap.emplace_back(0xa9, &GameboyCPU::XOR, "XOR", A, C);
    instrMap.emplace_back(0xaa, &GameboyCPU::XOR, "XOR", A, D);
    instrMap.emplace_back(0xab, &GameboyCPU::XOR, "XOR", A, E);
    instrMap.emplace_back(0xac, &GameboyCPU::XOR, "XOR", A, H);
    instrMap.emplace_back(0xad, &GameboyCPU::XOR, "XOR", A, L);
    instrMap.emplace_back(0xae, &GameboyCPU::XOR, "XOR", A, HL_ref);
    instrMap.emplace_back(0xaf, &GameboyCPU::XOR, "XOR", A, A);

    // 0xb0
    instrMap.emplace_back(0xb0, &GameboyCPU::OR, "OR", A, B);
    instrMap.emplace_back(0xb1, &GameboyCPU::OR, "OR", A, C);
    instrMap.emplace_back(0xb2, &GameboyCPU::OR, "OR", A, D);
    instrMap.emplace_back(0xb3, &GameboyCPU::OR, "OR", A, E);
    instrMap.emplace_back(0xb4, &GameboyCPU::OR, "OR", A, H);
    instrMap.emplace_back(0xb5, &GameboyCPU::OR, "OR", A, L);
    instrMap.emplace_back(0xb6, &GameboyCPU::OR, "OR", A, HL_ref);
    instrMap.emplace_back(0xb7, &GameboyCPU::OR, "OR", A, A);
    instrMap.emplace_back(0xb8, &GameboyCPU::CP, "CP", A, B);
    instrMap.emplace_back(0xb9, &GameboyCPU::CP, "CP", A, C);
    instrMap.emplace_back(0xba, &GameboyCPU::CP, "CP", A, D);
    instrMap.emplace_back(0xbb, &GameboyCPU::CP, "CP", A, E);
    instrMap.emplace_back(0xbc, &GameboyCPU::CP, "CP", A, H);
    instrMap.emplace_back(0xbd, &GameboyCPU::CP, "CP", A, L);
    instrMap.emplace_back(0xbe, &GameboyCPU::CP, "CP", A, HL_ref);
    instrMap.emplace_back(0xbf, &GameboyCPU::CP, "CP", A, A);

    // 0xc0
    instrMap.emplace_back(0xc0, &GameboyCPU::RET, "RET NZ", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xc1, &GameboyCPU::POP, "POP", BC, NO_DATA);
    instrMap.emplace_back(0xc2, &GameboyCPU::JP, "JP NZ", a16, NO_DATA);
    instrMap.emplace_back(0xc3, &GameboyCPU::JP, "JP", a16, NO_DATA);
    instrMap.emplace_back(0xc4, &GameboyCPU::CALL, "CALL NZ", a16, NO_DATA);
    instrMap.emplace_back(0xc5, &GameboyCPU::PUSH, "PUSH", BC, NO_DATA);
    instrMap.emplace_back(0xc6, &GameboyCPU::ADD8, "ADD", A, d8);
    instrMap.emplace_back(0xc7, &GameboyCPU::RST, "RST $00", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xc8, &GameboyCPU::RET, "RET Z", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xc9, &GameboyCPU::RET, "RET", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xca, &GameboyCPU::JP, "JP Z", a16, NO_DATA);
    instrMap.emplace_back(0xcb, &GameboyCPU::CB, "CB", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xcc, &GameboyCPU::CALL, "CALL Z", a16, NO_DATA);
    instrMap.emplace_back(0xcd, &GameboyCPU::CALL, "CALL", a16, NO_DATA);
    instrMap.emplace_back(0xce, &GameboyCPU::ADC, "ADC", A, d8);
    instrMap.emplace_back(0xcf, &GameboyCPU::RST, "RST $08", NO_DATA, NO_DATA);

    // 0xd0
    instrMap.emplace_back(0xd0, &GameboyCPU::RET, "RET NC", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xd1, &GameboyCPU::POP, "POP", DE, NO_DATA);
    instrMap.emplace_back(0xd2, &GameboyCPU::JP, "JP NC", a16, NO_DATA);
    instrMap.emplace_back(0xd3, &GameboyCPU::NoInstruction, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xd4, &GameboyCPU::CALL, "CALL NC", a16, NO_DATA);
    instrMap.emplace_back(0xd5, &GameboyCPU::PUSH, "PUSH", DE, NO_DATA);
    instrMap.emplace_back(0xd6, &GameboyCPU::SUB, "SUB", A, d8);
    instrMap.emplace_back(0xd7, &GameboyCPU::RST, "RST $10", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xd8, &GameboyCPU::RET, "RET C", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xd9, &GameboyCPU::RETI, "RETI", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xda, &GameboyCPU::JP, "JP C", a16, NO_DATA);
    instrMap.emplace_back(0xdb, &GameboyCPU::NoInstruction, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xdc, &GameboyCPU::CALL, "CALL C", a16, NO_DATA);
    instrMap.emplace_back(0xdd, &GameboyCPU::NoInstruction, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xde, &GameboyCPU::SBC, "SBC", A, d8);
    instrMap.emplace_back(0xdf, &GameboyCPU::RST, "RST $18", NO_DATA, NO_DATA);

    // 0xe0
    instrMap.emplace_back(0xe0, &GameboyCPU::LDH, "LDH", a8_ref, A);
    instrMap.emplace_back(0xe1, &GameboyCPU::POP, "POP", HL, NO_DATA);
    instrMap.emplace_back(0xe2, &GameboyCPU::LDCref, "LD", C_ref, A);
    instrMap.emplace_back(0xe3, &GameboyCPU::NoInstruction, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xe4, &GameboyCPU::NoInstruction, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xe5, &GameboyCPU::PUSH, "PUSH", HL, NO_DATA);
    instrMap.emplace_back(0xe6, &GameboyCPU::AND, "AND", A, d8);
    instrMap.emplace_back(0xe7, &GameboyCPU::RST, "RST $20", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xe8, &GameboyCPU::ADDSPr8, "ADD", SP, r8);
    instrMap.emplace_back(0xe9, &GameboyCPU::JP, "JP", HL_ref, NO_DATA);
    instrMap.emplace_back(0xea, &GameboyCPU::LDHa16, "LD", a16_ref, A);
    instrMap.emplace_back(0xeb, &GameboyCPU::NoInstruction, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xec, &GameboyCPU::NoInstruction, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xed, &GameboyCPU::NoInstruction, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xee, &GameboyCPU::XOR, "XOR", A, d8);
    instrMap.emplace_back(0xef, &GameboyCPU::RST, "RST $28", NO_DATA, NO_DATA);

    // 0xf0
    instrMap.emplace_back(0xf0, &GameboyCPU::LDH, "LD", A, a8_ref);
    instrMap.emplace_back(0xf1, &GameboyCPU::POP, "POP", AF, NO_DATA);
    instrMap.emplace_back(0xf2, &GameboyCPU::LDCref, "LD", A, C_ref);
    instrMap.emplace_back(0xf3, &GameboyCPU::DI, "DI", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xf4, &GameboyCPU::NoInstruction, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xf5, &GameboyCPU::PUSH, "PUSH", AF, NO_DATA);
    instrMap.emplace_back(0xf6, &GameboyCPU::OR, "OR", A, d8);
    instrMap.emplace_back(0xf7, &GameboyCPU::RST, "RST $30", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xf8, &GameboyCPU::LDHLSPr8, "LD", HL, SP_r8);
    instrMap.emplace_back(0xf9, &GameboyCPU::LDSPHL, "LD", SP, HL);
    instrMap.emplace_back(0xfa, &GameboyCPU::LDHa16, "LD", A, a16_ref);
    instrMap.emplace_back(0xfb, &GameboyCPU::EI, "EI", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xfc, &GameboyCPU::NoInstruction, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xfd, &GameboyCPU::NoInstruction, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xfe, &GameboyCPU::CP, "CP", A, d8);
    instrMap.emplace_back(0xff, &GameboyCPU::RST, "RST $38", NO_DATA, NO_DATA);
}

/* ***********************************************************************************************************
*
*   GAMEBOY (COLOR) GameboyCPU BASIC INSTRUCTION SET
*
*********************************************************************************************************** */

/* ***********************************************************************************************************
    CPU CONTROL
*********************************************************************************************************** */
// no instruction
void GameboyCPU::NoInstruction() {
    return;
}

// no operation
void GameboyCPU::NOP() {
    return;
}

// stopped
void GameboyCPU::STOP() {
    u8 isr_requested = mem_instance->GetIO(IF_ADDR);

    bool joyp = (isr_requested & IRQ_JOYPAD);
    bool two_byte = false;
    bool div_reset = false;

    if (joyp) {
        if (machine_ctx->IE & isr_requested) {
            return;
        }
        else {
            two_byte = true;
            halted = true;
        }
    }
    else {
        if (machine_ctx->speed_switch_requested) {
            if (machine_ctx->IE & isr_requested) {
                if (ime) {
                    LOG_ERROR("[emu] STOP Glitch encountered");
                    halted = true;
                    // set IE to 0x00 (this case is undefined behaviour, simply prevent cpu from execution)
                    machine_ctx->IE = 0x00;
                }
                else {
                    div_reset = true;
                }
            }
            else {
                // skipping halted, not necessary
                two_byte = true;
                div_reset = true;
            }
        }
        else {
            two_byte = machine_ctx->IE & isr_requested ? false : true;
            stopped = true;
            div_reset = true;
        }
    }


    if (two_byte) {
        data = mmu_instance->Read8Bit(Regs.PC);
        Regs.PC++;

        if (data) {
            LOG_ERROR("[emu] Wrong usage of STOP instruction at: ", format("{:x}", Regs.PC - 1));
            return;
        }
    }

    if (div_reset) {
        mmu_instance->Write8Bit(0x00, DIV_ADDR);
    }

    if (machine_ctx->speed_switch_requested) {
        machine_ctx->currentSpeed ^= 3;
        machine_ctx->speed_switch_requested = false;

        /*
        // set new APU bitmask
        switch (machine_ctx->currentSpeed) {
        case 1:
            sound_ctx->divApuBitMask = DIV_APU_SINGLESPEED_BIT;
            break;
        case 2:
            sound_ctx->divApuBitMask = DIV_APU_DOUBLESPEED_BIT;
            break;
        }
        */
    }
}

// halted
void GameboyCPU::HALT() {
    halted = true;
}

// flip c
void GameboyCPU::CCF() {
    RESET_FLAGS(FLAG_HCARRY | FLAG_SUB, Regs.F);
    Regs.F ^= FLAG_CARRY;
}

// 1's complement of A
void GameboyCPU::CPL() {
    SET_FLAGS(FLAG_SUB | FLAG_HCARRY, Regs.F);
    Regs.A = ~(Regs.A);
}

// set c
void GameboyCPU::SCF() {
    RESET_FLAGS(FLAG_HCARRY | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_CARRY;
}

// disable interrupts
void GameboyCPU::DI() {
    ime = false;
}

// eneable interrupts
void GameboyCPU::EI() {
    ime = true;
}

// enable CB sm83_instructions for next opcode
void GameboyCPU::CB() {
    // TODO: check for problems, the cb prefixed instruction gets actually executed the same cycle it gets fetched
    //TickTimers();
    return;
}

/* ***********************************************************************************************************
    LOAD
*********************************************************************************************************** */
// load 8/16 bit
void GameboyCPU::LDfromAtoRef() {
    switch (opcode) {
    case 0x02:
        Write8Bit(Regs.A, Regs.BC);
        break;
    case 0x12:
        Write8Bit(Regs.A, Regs.DE);
        break;
    case 0x22:
        Write8Bit(Regs.A, Regs.HL);
        Regs.HL++;
        break;
    case 0x32:
        Write8Bit(Regs.A, Regs.HL);
        Regs.HL--;
        break;
    }
}

void GameboyCPU::LDtoAfromRef() {
    switch (opcode) {
    case 0x0A:
        Regs.A = Read8Bit(Regs.BC);
        break;
    case 0x1A:
        Regs.A = Read8Bit(Regs.DE);
        break;
    case 0x2A:
        Regs.A = Read8Bit(Regs.HL);
        Regs.HL++;
        break;
    case 0x3A:
        Regs.A = Read8Bit(Regs.HL);
        Regs.HL--;
        break;
    }
}

void GameboyCPU::LDd8() {
    Fetch8Bit();

    switch (opcode) {
    case 0x06:
        Regs.BC_.B = (u8)data;
        break;
    case 0x16:
        Regs.DE_.D = (u8)data;
        break;
    case 0x26:
        Regs.HL_.H = (u8)data;
        break;
    case 0x36:
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x0E:
        Regs.BC_.C = (u8)data;
        break;
    case 0x1E:
        Regs.DE_.E = (u8)data;
        break;
    case 0x2E:
        Regs.HL_.L = (u8)data;
        break;
    case 0x3E:
        Regs.A = (u8)data;
        break;
    }
}

void GameboyCPU::LDd16() {
    Fetch16Bit();

    switch (opcode) {
    case 0x01:
        Regs.BC = data;
        break;
    case 0x11:
        Regs.DE = data;
        break;
    case 0x21:
        Regs.HL = data;
        break;
    case 0x31:
        Regs.SP = data;
        break;
    }
}

void GameboyCPU::LDCref() {
    switch (opcode) {
    case 0xE2:
        Write8Bit(Regs.A, Regs.BC_.C | 0xFF00);
        break;
    case 0xF2:
        Regs.A = Read8Bit(Regs.BC_.C | 0xFF00);
        break;
    }
}

void GameboyCPU::LDH() {
    Fetch8Bit();

    switch (opcode) {
    case 0xE0:
        Write8Bit(Regs.A, data | 0xFF00);
        break;
    case 0xF0:
        Regs.A = Read8Bit(data | 0xFF00);
        break;
    }
}

void GameboyCPU::LDHa16() {
    Fetch16Bit();

    switch (opcode) {
    case 0xea:
        Write8Bit(Regs.A, data);
        break;
    case 0xfa:
        Regs.A = Read8Bit(data);
        break;
    }
}

void GameboyCPU::LDSPa16() {
    Fetch16Bit();
    Write16Bit(Regs.SP, data);
}

void GameboyCPU::LDtoB() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B = Regs.BC_.B;
        break;
    case 0x01:
        Regs.BC_.B = Regs.BC_.C;
        break;
    case 0x02:
        Regs.BC_.B = Regs.DE_.D;
        break;
    case 0x03:
        Regs.BC_.B = Regs.DE_.E;
        break;
    case 0x04:
        Regs.BC_.B = Regs.HL_.H;
        break;
    case 0x05:
        Regs.BC_.B = Regs.HL_.L;
        break;
    case 0x06:
        Regs.BC_.B = Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.BC_.B = Regs.A;
        break;
    }
}

void GameboyCPU::LDtoC() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.C = Regs.BC_.B;
        break;
    case 0x01:
        Regs.BC_.C = Regs.BC_.C;
        break;
    case 0x02:
        Regs.BC_.C = Regs.DE_.D;
        break;
    case 0x03:
        Regs.BC_.C = Regs.DE_.E;
        break;
    case 0x04:
        Regs.BC_.C = Regs.HL_.H;
        break;
    case 0x05:
        Regs.BC_.C = Regs.HL_.L;
        break;
    case 0x06:
        Regs.BC_.C = Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.BC_.C = Regs.A;
        break;
    }
}

void GameboyCPU::LDtoD() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.DE_.D = Regs.BC_.B;
        break;
    case 0x01:
        Regs.DE_.D = Regs.BC_.C;
        break;
    case 0x02:
        Regs.DE_.D = Regs.DE_.D;
        break;
    case 0x03:
        Regs.DE_.D = Regs.DE_.E;
        break;
    case 0x04:
        Regs.DE_.D = Regs.HL_.H;
        break;
    case 0x05:
        Regs.DE_.D = Regs.HL_.L;
        break;
    case 0x06:
        Regs.DE_.D = Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.DE_.D = Regs.A;
        break;
    }
}

void GameboyCPU::LDtoE() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.DE_.E = Regs.BC_.B;
        break;
    case 0x01:
        Regs.DE_.E = Regs.BC_.C;
        break;
    case 0x02:
        Regs.DE_.E = Regs.DE_.D;
        break;
    case 0x03:
        Regs.DE_.E = Regs.DE_.E;
        break;
    case 0x04:
        Regs.DE_.E = Regs.HL_.H;
        break;
    case 0x05:
        Regs.DE_.E = Regs.HL_.L;
        break;
    case 0x06:
        Regs.DE_.E = Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.DE_.E = Regs.A;
        break;
    }
}

void GameboyCPU::LDtoH() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.HL_.H = Regs.BC_.B;
        break;
    case 0x01:
        Regs.HL_.H = Regs.BC_.C;
        break;
    case 0x02:
        Regs.HL_.H = Regs.DE_.D;
        break;
    case 0x03:
        Regs.HL_.H = Regs.DE_.E;
        break;
    case 0x04:
        Regs.HL_.H = Regs.HL_.H;
        break;
    case 0x05:
        Regs.HL_.H = Regs.HL_.L;
        break;
    case 0x06:
        Regs.HL_.H = Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.HL_.H = Regs.A;
        break;
    }
}

void GameboyCPU::LDtoL() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.HL_.L = Regs.BC_.B;
        break;
    case 0x01:
        Regs.HL_.L = Regs.BC_.C;
        break;
    case 0x02:
        Regs.HL_.L = Regs.DE_.D;
        break;
    case 0x03:
        Regs.HL_.L = Regs.DE_.E;
        break;
    case 0x04:
        Regs.HL_.L = Regs.HL_.H;
        break;
    case 0x05:
        Regs.HL_.L = Regs.HL_.L;
        break;
    case 0x06:
        Regs.HL_.L = Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.HL_.L = Regs.A;
        break;
    }
}

void GameboyCPU::LDtoHLref() {
    switch (opcode & 0x07) {
    case 0x00:
        Write8Bit(Regs.BC_.B, Regs.HL);
        break;
    case 0x01:
        Write8Bit(Regs.BC_.C, Regs.HL);
        break;
    case 0x02:
        Write8Bit(Regs.DE_.D, Regs.HL);
        break;
    case 0x03:
        Write8Bit(Regs.DE_.E, Regs.HL);
        break;
    case 0x04:
        Write8Bit(Regs.HL_.H, Regs.HL);
        break;
    case 0x05:
        Write8Bit(Regs.HL_.L, Regs.HL);
        break;
    case 0x07:
        Write8Bit(Regs.A, Regs.HL);
        break;
    }
}

void GameboyCPU::LDtoA() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.A = Regs.BC_.B;
        break;
    case 0x01:
        Regs.A = Regs.BC_.C;
        break;
    case 0x02:
        Regs.A = Regs.DE_.D;
        break;
    case 0x03:
        Regs.A = Regs.DE_.E;
        break;
    case 0x04:
        Regs.A = Regs.HL_.H;
        break;
    case 0x05:
        Regs.A = Regs.HL_.L;
        break;
    case 0x06:
        Regs.A = Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.A = Regs.A;
        break;
    }
}

// ld SP to HL and add signed 8 bit immediate data
void GameboyCPU::LDHLSPr8() {
    Fetch8Bit();

    Regs.HL = Regs.SP;
    RESET_ALL_FLAGS(Regs.F);
    ADD_8_FLAGS(Regs.HL, data, Regs.F);
    Regs.HL += *(i8*)&data;

    TickTimers();
}

void GameboyCPU::LDSPHL() {
    Regs.SP = Regs.HL;
    TickTimers();
}

// push PC to Stack
void GameboyCPU::PUSH() {
    switch (opcode) {
    case 0xc5:
        data = Regs.BC;
        break;
    case 0xd5:
        data = Regs.DE;
        break;
    case 0xe5:
        data = Regs.HL;
        break;
    case 0xf5:
        data = ((u16)Regs.A << 8) | Regs.F;
        break;
    }

    stack_push(data);
}

void GameboyCPU::stack_push(const u16& _data) {
    Regs.SP -= 2;
    TickTimers();

    Write16Bit(_data, Regs.SP);
}

void GameboyCPU::isr_push(const u16& _isr_handler) {
    // simulate 2 NOPs
    TickTimers();
    TickTimers();

    stack_push(Regs.PC);
    Regs.PC = _isr_handler;
}

// pop PC from Stack
void GameboyCPU::POP() {
    switch (opcode) {
    case 0xc1:
        Regs.BC = stack_pop();
        break;
    case 0xd1:
        Regs.DE = stack_pop();
        break;
    case 0xe1:
        Regs.HL = stack_pop();
        break;
    case 0xf1:
        data = stack_pop();
        Regs.A = (data & 0xFF00) >> 8;
        Regs.F = data & 0xF0;
        break;
    }
}

u16 GameboyCPU::stack_pop() {
    u16 data = Read16Bit(Regs.SP);
    Regs.SP += 2;
    return data;
}

/* ***********************************************************************************************************
    ARITHMETIC/LOGIC INSTRUCTIONS
*********************************************************************************************************** */
// increment 8/16 bit
void GameboyCPU::INC8() {
    RESET_FLAGS(FLAG_ZERO | FLAG_SUB | FLAG_HCARRY, Regs.F);

    switch (opcode) {
    case 0x04:
        ADD_8_HC(Regs.BC_.B, 1, Regs.F);
        Regs.BC_.B += 1;
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x0c:
        ADD_8_HC(Regs.BC_.C, 1, Regs.F);
        Regs.BC_.C += 1;
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x14:
        ADD_8_HC(Regs.DE_.D, 1, Regs.F);
        Regs.DE_.D += 1;
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x1c:
        ADD_8_HC(Regs.DE_.E, 1, Regs.F);
        Regs.DE_.E += 1;
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x24:
        ADD_8_HC(Regs.HL_.H, 1, Regs.F);
        Regs.HL_.H += 1;
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x2c:
        ADD_8_HC(Regs.HL_.L, 1, Regs.F);
        Regs.HL_.L += 1;
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x34:
        data = Read8Bit(Regs.HL);
        ADD_8_HC(data, 1, Regs.F);
        data = ((data + 1) & 0xFF);
        ZERO_FLAG(data, Regs.F);
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x3c:
        ADD_8_HC(Regs.A, 1, Regs.F);
        Regs.A += 1;
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

void GameboyCPU::INC16() {
    switch (opcode) {
    case 0x03:
        Regs.BC += 1;
        break;
    case 0x13:
        Regs.DE += 1;
        break;
    case 0x23:
        Regs.HL += 1;
        break;
    case 0x33:
        Regs.SP += 1;
        break;
    }
    TickTimers();
}

// decrement 8/16 bit
void GameboyCPU::DEC8() {
    RESET_FLAGS(FLAG_ZERO | FLAG_HCARRY, Regs.F);
    SET_FLAGS(FLAG_SUB, Regs.F);

    switch (opcode) {
    case 0x05:
        SUB_8_HC(Regs.BC_.B, 1, Regs.F);
        Regs.BC_.B -= 1;
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x0d:
        SUB_8_HC(Regs.BC_.C, 1, Regs.F);
        Regs.BC_.C -= 1;
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x15:
        SUB_8_HC(Regs.DE_.D, 1, Regs.F);
        Regs.DE_.D -= 1;
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x1d:
        SUB_8_HC(Regs.DE_.E, 1, Regs.F);
        Regs.DE_.E -= 1;
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x25:
        SUB_8_HC(Regs.HL_.H, 1, Regs.F);
        Regs.HL_.H -= 1;
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x2d:
        SUB_8_HC(Regs.HL_.L, 1, Regs.F);
        Regs.HL_.L -= 1;
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x35:
        data = Read8Bit(Regs.HL);
        SUB_8_HC(data, 1, Regs.F);
        data -= 1;
        ZERO_FLAG(data, Regs.F);
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x3d:
        SUB_8_HC(Regs.A, 1, Regs.F);
        Regs.A -= 1;
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

void GameboyCPU::DEC16() {
    switch (opcode) {
    case 0x0b:
        Regs.BC -= 1;
        break;
    case 0x1b:
        Regs.DE -= 1;
        break;
    case 0x2b:
        Regs.HL -= 1;
        break;
    case 0x3b:
        Regs.SP -= 1;
        break;
    }
    TickTimers();
}

// add with carry + halfcarry
void GameboyCPU::ADD8() {
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode) {
    case 0x80:
        data = Regs.BC_.B;
        break;
    case 0x81:
        data = Regs.BC_.C;
        break;
    case 0x82:
        data = Regs.DE_.D;
        break;
    case 0x83:
        data = Regs.DE_.E;
        break;
    case 0x84:
        data = Regs.HL_.H;
        break;
    case 0x85:
        data = Regs.HL_.L;
        break;
    case 0x86:
        data = Read8Bit(Regs.HL);
        break;
    case 0x87:
        data = Regs.A;
        break;
    case 0xc6:
        Fetch8Bit();
        break;
    }

    ADD_8_FLAGS(Regs.A, data, Regs.F);
    Regs.A += data;
    ZERO_FLAG(Regs.A, Regs.F);
}

void GameboyCPU::ADDHL() {
    RESET_FLAGS(FLAG_SUB | FLAG_HCARRY | FLAG_CARRY, Regs.F);

    switch (opcode) {
    case 0x09:
        data = Regs.BC;
        break;
    case 0x19:
        data = Regs.DE;
        break;
    case 0x29:
        data = Regs.HL;
        break;
    case 0x39:
        data = Regs.SP;
        break;
    }

    ADD_16_FLAGS(Regs.HL, data, Regs.F);
    Regs.HL += data;

    TickTimers();
}

// add for SP + signed immediate data r8
void GameboyCPU::ADDSPr8() {
    Fetch8Bit();

    RESET_ALL_FLAGS(Regs.F);
    ADD_8_FLAGS(Regs.SP, data, Regs.F);
    Regs.SP += *(i8*)&data;

    TickTimers();
    TickTimers();
}

// adc for A + (register or immediate unsigned data d8) + carry
void GameboyCPU::ADC() {
    u8 carry = (Regs.F & FLAG_CARRY ? 1 : 0);
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode) {
    case 0x88:
        data = Regs.BC_.B;
        break;
    case 0x89:
        data = Regs.BC_.C;
        break;
    case 0x8a:
        data = Regs.DE_.D;
        break;
    case 0x8b:
        data = Regs.DE_.E;
        break;
    case 0x8c:
        data = Regs.HL_.H;
        break;
    case 0x8d:
        data = Regs.HL_.L;
        break;
    case 0x8e:
        data = Read8Bit(Regs.HL);
        break;
    case 0x8f:
        data = Regs.A;
        break;
    case 0xce:
        Fetch8Bit();
        break;
    }

    ADC_8_HC(Regs.A, data, carry, Regs.F);
    ADC_8_C(Regs.A, data, carry, Regs.F);
    Regs.A += data + carry;
    ZERO_FLAG(Regs.A, Regs.F);
}

// sub with carry + halfcarry
void GameboyCPU::SUB() {
    Regs.F = FLAG_SUB;

    switch (opcode) {
    case 0x90:
        data = Regs.BC_.B;
        break;
    case 0x91:
        data = Regs.BC_.C;
        break;
    case 0x92:
        data = Regs.DE_.D;
        break;
    case 0x93:
        data = Regs.DE_.E;
        break;
    case 0x94:
        data = Regs.HL_.H;
        break;
    case 0x95:
        data = Regs.HL_.L;
        break;
    case 0x96:
        data = Read8Bit(Regs.HL);
        break;
    case 0x97:
        data = Regs.A;
        break;
    case 0xd6:
        Fetch8Bit();
        break;
    }

    SUB_8_FLAGS(Regs.A, data, Regs.F);
    Regs.A -= data;
    ZERO_FLAG(Regs.A, Regs.F);
}

// adc for A + (register or immediate unsigned data d8) + carry
void GameboyCPU::SBC() {
    u8 carry = (Regs.F & FLAG_CARRY ? 1 : 0);
    Regs.F = FLAG_SUB;

    switch (opcode) {
    case 0x98:
        data = Regs.BC_.B;
        break;
    case 0x99:
        data = Regs.BC_.C;
        break;
    case 0x9a:
        data = Regs.DE_.D;
        break;
    case 0x9b:
        data = Regs.DE_.E;
        break;
    case 0x9c:
        data = Regs.HL_.H;
        break;
    case 0x9d:
        data = Regs.HL_.L;
        break;
    case 0x9e:
        data = Read8Bit(Regs.HL);
        break;
    case 0x9f:
        data = Regs.A;
        break;
    case 0xde:
        Fetch8Bit();
        break;
    }

    SBC_8_HC(Regs.A, data, carry, Regs.F);
    SBC_8_C(Regs.A, data, carry, Regs.F);
    Regs.A -= (data + carry);
    ZERO_FLAG(Regs.A, Regs.F);
}

// bcd code addition and subtraction
void GameboyCPU::DAA() {
    if (Regs.F & FLAG_SUB) {
        if (Regs.F & FLAG_CARRY) { Regs.A -= 0x60; }
        if (Regs.F & FLAG_HCARRY) { Regs.A -= 0x06; }
    }
    else {
        if (Regs.F & FLAG_CARRY || Regs.A > 0x99) {
            Regs.A += 0x60;
            Regs.F |= FLAG_CARRY;
        }
        if (Regs.F & FLAG_HCARRY || (Regs.A & 0x0F) > 0x09) {
            Regs.A += 0x06;
        }
    }
    ZERO_FLAG(Regs.A, Regs.F);
    Regs.F &= ~FLAG_HCARRY;
}

// and with hc=1 and z
void GameboyCPU::AND() {
    Regs.F = FLAG_HCARRY;

    switch (opcode) {
    case 0xa0:
        data = Regs.BC_.B;
        break;
    case 0xa1:
        data = Regs.BC_.C;
        break;
    case 0xa2:
        data = Regs.DE_.D;
        break;
    case 0xa3:
        data = Regs.DE_.E;
        break;
    case 0xa4:
        data = Regs.HL_.H;
        break;
    case 0xa5:
        data = Regs.HL_.L;
        break;
    case 0xa6:
        data = Read8Bit(Regs.HL);
        break;
    case 0xa7:
        data = Regs.A;
        break;
    case 0xe6:
        Fetch8Bit();
        break;
    }

    Regs.A &= data;
    ZERO_FLAG(Regs.A, Regs.F);
}

// or with z
void GameboyCPU::OR() {
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode) {
    case 0xb0:
        data = Regs.BC_.B;
        break;
    case 0xb1:
        data = Regs.BC_.C;
        break;
    case 0xb2:
        data = Regs.DE_.D;
        break;
    case 0xb3:
        data = Regs.DE_.E;
        break;
    case 0xb4:
        data = Regs.HL_.H;
        break;
    case 0xb5:
        data = Regs.HL_.L;
        break;
    case 0xb6:
        data = Read8Bit(Regs.HL);
        break;
    case 0xb7:
        data = Regs.A;
        break;
    case 0xf6:
        Fetch8Bit();
        break;
    }

    Regs.A |= data;
    ZERO_FLAG(Regs.A, Regs.F);
}

// xor with z
void GameboyCPU::XOR() {
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode) {
    case 0xa8:
        data = Regs.BC_.B;
        break;
    case 0xa9:
        data = Regs.BC_.C;
        break;
    case 0xaa:
        data = Regs.DE_.D;
        break;
    case 0xab:
        data = Regs.DE_.E;
        break;
    case 0xac:
        data = Regs.HL_.H;
        break;
    case 0xad:
        data = Regs.HL_.L;
        break;
    case 0xae:
        data = Read8Bit(Regs.HL);
        break;
    case 0xaf:
        data = Regs.A;
        break;
    case 0xee:
        Fetch8Bit();
        break;
    }

    Regs.A ^= data;
    ZERO_FLAG(Regs.A, Regs.F);
}

// compare (subtraction)
void GameboyCPU::CP() {
    Regs.F = FLAG_SUB;

    switch (opcode) {
    case 0xb8:
        data = Regs.BC_.B;
        break;
    case 0xb9:
        data = Regs.BC_.C;
        break;
    case 0xba:
        data = Regs.DE_.D;
        break;
    case 0xbb:
        data = Regs.DE_.E;
        break;
    case 0xbc:
        data = Regs.HL_.H;
        break;
    case 0xbd:
        data = Regs.HL_.L;
        break;
    case 0xbe:
        data = Read8Bit(Regs.HL);
        break;
    case 0xbf:
        data = Regs.A;
        break;
    case 0xfe:
        Fetch8Bit();
        break;
    }

    SUB_8_FLAGS(Regs.A, data, Regs.F);
    ZERO_FLAG(Regs.A ^ (u8)data, Regs.F);
}

/* ***********************************************************************************************************
    ROTATE AND SHIFT
*********************************************************************************************************** */
// rotate A left with carry
void GameboyCPU::RLCA() {
    RESET_ALL_FLAGS(Regs.F);
    Regs.F |= (Regs.A & MSB ? FLAG_CARRY : 0x00);
    Regs.A <<= 1;
    Regs.A |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
}

// rotate A right with carry
void GameboyCPU::RRCA() {
    RESET_ALL_FLAGS(Regs.F);
    Regs.F |= (Regs.A & LSB ? FLAG_CARRY : 0x00);
    Regs.A >>= 1;
    Regs.A |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
}

// rotate A left through carry
void GameboyCPU::RLA() {
    bool carry = Regs.F & FLAG_CARRY;
    RESET_ALL_FLAGS(Regs.F);
    Regs.F |= (Regs.A & MSB ? FLAG_CARRY : 0x00);
    Regs.A <<= 1;
    Regs.A |= (carry ? LSB : 0x00);
}

// rotate A right through carry
void GameboyCPU::RRA() {
    bool carry = Regs.F & FLAG_CARRY;
    RESET_ALL_FLAGS(Regs.F);
    Regs.F |= (Regs.A & LSB ? FLAG_CARRY : 0x00);
    Regs.A >>= 1;
    Regs.A |= (carry ? MSB : 0x00);
}

/* ***********************************************************************************************************
    JUMP INSTRUCTIONS
*********************************************************************************************************** */
// jump to memory location
void GameboyCPU::JP() {
    if (opcode == 0xe9) {
        data = Regs.HL;
        jump_jp();
        return;
    }

    bool carry;
    bool zero;

    Fetch16Bit();

    switch (opcode) {
    case 0xCA:
        zero = Regs.F & FLAG_ZERO;
        if (zero) {
            jump_jp();
            return;
        }
        break;
    case 0xC2:
        zero = Regs.F & FLAG_ZERO;
        if (!zero) {
            jump_jp();
            return;
        }
        break;
    case 0xDA:
        carry = Regs.F & FLAG_CARRY;
        if (carry) {
            jump_jp();
            return;
        }
        break;
    case 0xD2:
        carry = Regs.F & FLAG_CARRY;
        if (!carry) {
            jump_jp();
            return;
        }
        break;
    case 0xC3:
        jump_jp();
        return;
        break;
    }
}

void GameboyCPU::jump_jp() {
    Regs.PC = data;
    if (opcode != 0xe9) { TickTimers(); }
}

// jump relative to memory lecation
void GameboyCPU::JR() {
    bool carry;
    bool zero;

    Fetch8Bit();

    switch (opcode) {
    case 0x28:
        zero = Regs.F & FLAG_ZERO;
        if (zero) {
            jump_jr();
            return;
        }
        break;
    case 0x20:
        zero = Regs.F & FLAG_ZERO;
        if (!zero) {
            jump_jr();
            return;
        }
        break;
    case 0x38:
        carry = Regs.F & FLAG_CARRY;
        if (carry) {
            jump_jr();
            return;
        }
        break;
    case 0x30:
        carry = Regs.F & FLAG_CARRY;
        if (!carry) {
            jump_jr();
            return;
        }
        break;
    default:
        jump_jr();
        break;
    }
}

void GameboyCPU::jump_jr() {
    Regs.PC += *(i8*)&data;
    TickTimers();
}

// call routine at memory location
void GameboyCPU::CALL() {
    bool carry;
    bool zero;

    Fetch16Bit();

    switch (opcode) {
    case 0xCC:
        zero = Regs.F & FLAG_ZERO;
        if (zero) {
            call();
            return;
        }
        break;
    case 0xC4:
        zero = Regs.F & FLAG_ZERO;
        if (!zero) {
            call();
            return;
        }
        break;
    case 0xDC:
        carry = Regs.F & FLAG_CARRY;
        if (carry) {
            call();
            return;
        }
        break;
    case 0xD4:
        carry = Regs.F & FLAG_CARRY;
        if (!carry) {
            call();
            return;
        }
        break;
    default:
        call();
        return;
        break;
    }
}

// call to special addresses
void GameboyCPU::RST() {
    switch (opcode) {
    case 0xc7:
        data = 0x00;
        break;
    case 0xd7:
        data = 0x10;
        break;
    case 0xe7:
        data = 0x20;
        break;
    case 0xf7:
        data = 0x30;
        break;
    case 0xcf:
        data = 0x08;
        break;
    case 0xdf:
        data = 0x18;
        break;
    case 0xef:
        data = 0x28;
        break;
    case 0xff:
        data = 0x38;
        break;
    }
    call();
}

void GameboyCPU::call() {
    stack_push(Regs.PC);
    Regs.PC = data;
}

// return from routine
void GameboyCPU::RET() {
    bool carry;
    bool zero;

    switch (opcode) {
    case 0xC8:
        TickTimers();
        zero = Regs.F & FLAG_ZERO;
        if (zero) {
            ret();
        }
        break;
    case 0xC0:
        TickTimers();
        zero = Regs.F & FLAG_ZERO;
        if (!zero) {
            ret();
        }
        break;
    case 0xD8:
        TickTimers();
        carry = Regs.F & FLAG_CARRY;
        if (carry) {
            ret();
        }
        break;
    case 0xD0:
        TickTimers();
        carry = Regs.F & FLAG_CARRY;
        if (!carry) {
            ret();
        }
        break;
    default:
        ret();
        break;
    }
}

// return and enable interrupts
void GameboyCPU::RETI() {
    ret();
    ime = true;
}

void GameboyCPU::ret() {
    Regs.PC = stack_pop();
    TickTimers();
}

/* ***********************************************************************************************************
*
*   GAMEBOY (COLOR) GameboyCPU CB INSTRUCTION LOOKUP TABLE
*
*********************************************************************************************************** */
void GameboyCPU::setupLookupTableCB() {
    instrMapCB.clear();

    // Elements: opcode, instruction function, machine cycles
    instrMapCB.emplace_back(0x00, &GameboyCPU::RLC, "RLC", B, NO_DATA);
    instrMapCB.emplace_back(0x01, &GameboyCPU::RLC, "RLC", C, NO_DATA);
    instrMapCB.emplace_back(0x02, &GameboyCPU::RLC, "RLC", D, NO_DATA);
    instrMapCB.emplace_back(0x03, &GameboyCPU::RLC, "RLC", E, NO_DATA);
    instrMapCB.emplace_back(0x04, &GameboyCPU::RLC, "RLC", H, NO_DATA);
    instrMapCB.emplace_back(0x05, &GameboyCPU::RLC, "RLC", L, NO_DATA);
    instrMapCB.emplace_back(0x06, &GameboyCPU::RLC, "RLC", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x07, &GameboyCPU::RLC, "RLC", A, NO_DATA);
    instrMapCB.emplace_back(0x08, &GameboyCPU::RRC, "RRC", B, NO_DATA);
    instrMapCB.emplace_back(0x09, &GameboyCPU::RRC, "RRC", C, NO_DATA);
    instrMapCB.emplace_back(0x0a, &GameboyCPU::RRC, "RRC", D, NO_DATA);
    instrMapCB.emplace_back(0x0b, &GameboyCPU::RRC, "RRC", E, NO_DATA);
    instrMapCB.emplace_back(0x0c, &GameboyCPU::RRC, "RRC", H, NO_DATA);
    instrMapCB.emplace_back(0x0d, &GameboyCPU::RRC, "RRC", L, NO_DATA);
    instrMapCB.emplace_back(0x0e, &GameboyCPU::RRC, "RRC", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x0f, &GameboyCPU::RRC, "RRC", A, NO_DATA);

    instrMapCB.emplace_back(0x10, &GameboyCPU::RL, "RL", B, NO_DATA);
    instrMapCB.emplace_back(0x11, &GameboyCPU::RL, "RL", C, NO_DATA);
    instrMapCB.emplace_back(0x12, &GameboyCPU::RL, "RL", D, NO_DATA);
    instrMapCB.emplace_back(0x13, &GameboyCPU::RL, "RL", E, NO_DATA);
    instrMapCB.emplace_back(0x14, &GameboyCPU::RL, "RL", H, NO_DATA);
    instrMapCB.emplace_back(0x15, &GameboyCPU::RL, "RL", L, NO_DATA);
    instrMapCB.emplace_back(0x16, &GameboyCPU::RL, "RL", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x17, &GameboyCPU::RL, "RL", A, NO_DATA);
    instrMapCB.emplace_back(0x18, &GameboyCPU::RR, "RR", B, NO_DATA);
    instrMapCB.emplace_back(0x19, &GameboyCPU::RR, "RR", C, NO_DATA);
    instrMapCB.emplace_back(0x1a, &GameboyCPU::RR, "RR", D, NO_DATA);
    instrMapCB.emplace_back(0x1b, &GameboyCPU::RR, "RR", E, NO_DATA);
    instrMapCB.emplace_back(0x1c, &GameboyCPU::RR, "RR", H, NO_DATA);
    instrMapCB.emplace_back(0x1d, &GameboyCPU::RR, "RR", L, NO_DATA);
    instrMapCB.emplace_back(0x1e, &GameboyCPU::RR, "RR", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x1f, &GameboyCPU::RR, "RR", A, NO_DATA);

    instrMapCB.emplace_back(0x20, &GameboyCPU::SLA, "SLA", B, NO_DATA);
    instrMapCB.emplace_back(0x21, &GameboyCPU::SLA, "SLA", C, NO_DATA);
    instrMapCB.emplace_back(0x22, &GameboyCPU::SLA, "SLA", D, NO_DATA);
    instrMapCB.emplace_back(0x23, &GameboyCPU::SLA, "SLA", E, NO_DATA);
    instrMapCB.emplace_back(0x24, &GameboyCPU::SLA, "SLA", H, NO_DATA);
    instrMapCB.emplace_back(0x25, &GameboyCPU::SLA, "SLA", L, NO_DATA);
    instrMapCB.emplace_back(0x26, &GameboyCPU::SLA, "SLA", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x27, &GameboyCPU::SLA, "SLA", A, NO_DATA);
    instrMapCB.emplace_back(0x28, &GameboyCPU::SRA, "SRA", B, NO_DATA);
    instrMapCB.emplace_back(0x29, &GameboyCPU::SRA, "SRA", C, NO_DATA);
    instrMapCB.emplace_back(0x2a, &GameboyCPU::SRA, "SRA", D, NO_DATA);
    instrMapCB.emplace_back(0x2b, &GameboyCPU::SRA, "SRA", E, NO_DATA);
    instrMapCB.emplace_back(0x2c, &GameboyCPU::SRA, "SRA", H, NO_DATA);
    instrMapCB.emplace_back(0x2d, &GameboyCPU::SRA, "SRA", L, NO_DATA);
    instrMapCB.emplace_back(0x2e, &GameboyCPU::SRA, "SRA", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x2f, &GameboyCPU::SRA, "SRA", A, NO_DATA);

    instrMapCB.emplace_back(0x30, &GameboyCPU::SWAP, "SWAP", B, NO_DATA);
    instrMapCB.emplace_back(0x31, &GameboyCPU::SWAP, "SWAP", C, NO_DATA);
    instrMapCB.emplace_back(0x32, &GameboyCPU::SWAP, "SWAP", D, NO_DATA);
    instrMapCB.emplace_back(0x33, &GameboyCPU::SWAP, "SWAP", E, NO_DATA);
    instrMapCB.emplace_back(0x34, &GameboyCPU::SWAP, "SWAP", H, NO_DATA);
    instrMapCB.emplace_back(0x35, &GameboyCPU::SWAP, "SWAP", L, NO_DATA);
    instrMapCB.emplace_back(0x36, &GameboyCPU::SWAP, "SWAP", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x37, &GameboyCPU::SWAP, "SWAP", A, NO_DATA);
    instrMapCB.emplace_back(0x38, &GameboyCPU::SRL, "SRL", B, NO_DATA);
    instrMapCB.emplace_back(0x39, &GameboyCPU::SRL, "SRL", C, NO_DATA);
    instrMapCB.emplace_back(0x3a, &GameboyCPU::SRL, "SRL", D, NO_DATA);
    instrMapCB.emplace_back(0x3b, &GameboyCPU::SRL, "SRL", E, NO_DATA);
    instrMapCB.emplace_back(0x3c, &GameboyCPU::SRL, "SRL", H, NO_DATA);
    instrMapCB.emplace_back(0x3d, &GameboyCPU::SRL, "SRL", L, NO_DATA);
    instrMapCB.emplace_back(0x3e, &GameboyCPU::SRL, "SRL", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x3f, &GameboyCPU::SRL, "SRL", A, NO_DATA);

    instrMapCB.emplace_back(0x40, &GameboyCPU::BIT0, "BIT0", B, NO_DATA);
    instrMapCB.emplace_back(0x41, &GameboyCPU::BIT0, "BIT0", C, NO_DATA);
    instrMapCB.emplace_back(0x42, &GameboyCPU::BIT0, "BIT0", D, NO_DATA);
    instrMapCB.emplace_back(0x43, &GameboyCPU::BIT0, "BIT0", E, NO_DATA);
    instrMapCB.emplace_back(0x44, &GameboyCPU::BIT0, "BIT0", H, NO_DATA);
    instrMapCB.emplace_back(0x45, &GameboyCPU::BIT0, "BIT0", L, NO_DATA);
    instrMapCB.emplace_back(0x46, &GameboyCPU::BIT0, "BIT0", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x47, &GameboyCPU::BIT0, "BIT0", A, NO_DATA);
    instrMapCB.emplace_back(0x48, &GameboyCPU::BIT1, "BIT1", B, NO_DATA);
    instrMapCB.emplace_back(0x49, &GameboyCPU::BIT1, "BIT1", C, NO_DATA);
    instrMapCB.emplace_back(0x4a, &GameboyCPU::BIT1, "BIT1", D, NO_DATA);
    instrMapCB.emplace_back(0x4b, &GameboyCPU::BIT1, "BIT1", E, NO_DATA);
    instrMapCB.emplace_back(0x4c, &GameboyCPU::BIT1, "BIT1", H, NO_DATA);
    instrMapCB.emplace_back(0x4d, &GameboyCPU::BIT1, "BIT1", L, NO_DATA);
    instrMapCB.emplace_back(0x4e, &GameboyCPU::BIT1, "BIT1", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x4f, &GameboyCPU::BIT1, "BIT1", A, NO_DATA);

    instrMapCB.emplace_back(0x50, &GameboyCPU::BIT2, "BIT2", B, NO_DATA);
    instrMapCB.emplace_back(0x51, &GameboyCPU::BIT2, "BIT2", C, NO_DATA);
    instrMapCB.emplace_back(0x52, &GameboyCPU::BIT2, "BIT2", D, NO_DATA);
    instrMapCB.emplace_back(0x53, &GameboyCPU::BIT2, "BIT2", E, NO_DATA);
    instrMapCB.emplace_back(0x54, &GameboyCPU::BIT2, "BIT2", H, NO_DATA);
    instrMapCB.emplace_back(0x55, &GameboyCPU::BIT2, "BIT2", L, NO_DATA);
    instrMapCB.emplace_back(0x56, &GameboyCPU::BIT2, "BIT2", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x57, &GameboyCPU::BIT2, "BIT2", A, NO_DATA);
    instrMapCB.emplace_back(0x58, &GameboyCPU::BIT3, "BIT3", B, NO_DATA);
    instrMapCB.emplace_back(0x59, &GameboyCPU::BIT3, "BIT3", C, NO_DATA);
    instrMapCB.emplace_back(0x5a, &GameboyCPU::BIT3, "BIT3", D, NO_DATA);
    instrMapCB.emplace_back(0x5b, &GameboyCPU::BIT3, "BIT3", E, NO_DATA);
    instrMapCB.emplace_back(0x5c, &GameboyCPU::BIT3, "BIT3", H, NO_DATA);
    instrMapCB.emplace_back(0x5d, &GameboyCPU::BIT3, "BIT3", L, NO_DATA);
    instrMapCB.emplace_back(0x5e, &GameboyCPU::BIT3, "BIT3", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x5f, &GameboyCPU::BIT3, "BIT3", A, NO_DATA);

    instrMapCB.emplace_back(0x60, &GameboyCPU::BIT4, "BIT4", B, NO_DATA);
    instrMapCB.emplace_back(0x61, &GameboyCPU::BIT4, "BIT4", C, NO_DATA);
    instrMapCB.emplace_back(0x62, &GameboyCPU::BIT4, "BIT4", D, NO_DATA);
    instrMapCB.emplace_back(0x63, &GameboyCPU::BIT4, "BIT4", E, NO_DATA);
    instrMapCB.emplace_back(0x64, &GameboyCPU::BIT4, "BIT4", H, NO_DATA);
    instrMapCB.emplace_back(0x65, &GameboyCPU::BIT4, "BIT4", L, NO_DATA);
    instrMapCB.emplace_back(0x66, &GameboyCPU::BIT4, "BIT4", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x67, &GameboyCPU::BIT4, "BIT4", A, NO_DATA);
    instrMapCB.emplace_back(0x68, &GameboyCPU::BIT5, "BIT5", B, NO_DATA);
    instrMapCB.emplace_back(0x69, &GameboyCPU::BIT5, "BIT5", C, NO_DATA);
    instrMapCB.emplace_back(0x6a, &GameboyCPU::BIT5, "BIT5", D, NO_DATA);
    instrMapCB.emplace_back(0x6b, &GameboyCPU::BIT5, "BIT5", E, NO_DATA);
    instrMapCB.emplace_back(0x6c, &GameboyCPU::BIT5, "BIT5", H, NO_DATA);
    instrMapCB.emplace_back(0x6d, &GameboyCPU::BIT5, "BIT5", L, NO_DATA);
    instrMapCB.emplace_back(0x6e, &GameboyCPU::BIT5, "BIT5", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x6f, &GameboyCPU::BIT5, "BIT5", A, NO_DATA);

    instrMapCB.emplace_back(0x70, &GameboyCPU::BIT6, "BIT6", B, NO_DATA);
    instrMapCB.emplace_back(0x71, &GameboyCPU::BIT6, "BIT6", C, NO_DATA);
    instrMapCB.emplace_back(0x72, &GameboyCPU::BIT6, "BIT6", D, NO_DATA);
    instrMapCB.emplace_back(0x73, &GameboyCPU::BIT6, "BIT6", E, NO_DATA);
    instrMapCB.emplace_back(0x74, &GameboyCPU::BIT6, "BIT6", H, NO_DATA);
    instrMapCB.emplace_back(0x75, &GameboyCPU::BIT6, "BIT6", L, NO_DATA);
    instrMapCB.emplace_back(0x76, &GameboyCPU::BIT6, "BIT6", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x77, &GameboyCPU::BIT6, "BIT6", A, NO_DATA);
    instrMapCB.emplace_back(0x78, &GameboyCPU::BIT7, "BIT7", B, NO_DATA);
    instrMapCB.emplace_back(0x79, &GameboyCPU::BIT7, "BIT7", C, NO_DATA);
    instrMapCB.emplace_back(0x7a, &GameboyCPU::BIT7, "BIT7", D, NO_DATA);
    instrMapCB.emplace_back(0x7b, &GameboyCPU::BIT7, "BIT7", E, NO_DATA);
    instrMapCB.emplace_back(0x7c, &GameboyCPU::BIT7, "BIT7", H, NO_DATA);
    instrMapCB.emplace_back(0x7d, &GameboyCPU::BIT7, "BIT7", L, NO_DATA);
    instrMapCB.emplace_back(0x7e, &GameboyCPU::BIT7, "BIT7", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x7f, &GameboyCPU::BIT7, "BIT7", A, NO_DATA);

    instrMapCB.emplace_back(0x80, &GameboyCPU::RES0, "RES0", B, NO_DATA);
    instrMapCB.emplace_back(0x81, &GameboyCPU::RES0, "RES0", C, NO_DATA);
    instrMapCB.emplace_back(0x82, &GameboyCPU::RES0, "RES0", D, NO_DATA);
    instrMapCB.emplace_back(0x83, &GameboyCPU::RES0, "RES0", E, NO_DATA);
    instrMapCB.emplace_back(0x84, &GameboyCPU::RES0, "RES0", H, NO_DATA);
    instrMapCB.emplace_back(0x85, &GameboyCPU::RES0, "RES0", L, NO_DATA);
    instrMapCB.emplace_back(0x86, &GameboyCPU::RES0, "RES0", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x87, &GameboyCPU::RES0, "RES0", A, NO_DATA);
    instrMapCB.emplace_back(0x88, &GameboyCPU::RES1, "RES1", B, NO_DATA);
    instrMapCB.emplace_back(0x89, &GameboyCPU::RES1, "RES1", C, NO_DATA);
    instrMapCB.emplace_back(0x8a, &GameboyCPU::RES1, "RES1", D, NO_DATA);
    instrMapCB.emplace_back(0x8b, &GameboyCPU::RES1, "RES1", E, NO_DATA);
    instrMapCB.emplace_back(0x8c, &GameboyCPU::RES1, "RES1", H, NO_DATA);
    instrMapCB.emplace_back(0x8d, &GameboyCPU::RES1, "RES1", L, NO_DATA);
    instrMapCB.emplace_back(0x8e, &GameboyCPU::RES1, "RES1", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x8f, &GameboyCPU::RES1, "RES1", A, NO_DATA);

    instrMapCB.emplace_back(0x90, &GameboyCPU::RES2, "RES2", B, NO_DATA);
    instrMapCB.emplace_back(0x91, &GameboyCPU::RES2, "RES2", C, NO_DATA);
    instrMapCB.emplace_back(0x92, &GameboyCPU::RES2, "RES2", D, NO_DATA);
    instrMapCB.emplace_back(0x93, &GameboyCPU::RES2, "RES2", E, NO_DATA);
    instrMapCB.emplace_back(0x94, &GameboyCPU::RES2, "RES2", H, NO_DATA);
    instrMapCB.emplace_back(0x95, &GameboyCPU::RES2, "RES2", L, NO_DATA);
    instrMapCB.emplace_back(0x96, &GameboyCPU::RES2, "RES2", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x97, &GameboyCPU::RES2, "RES2", A, NO_DATA);
    instrMapCB.emplace_back(0x98, &GameboyCPU::RES3, "RES3", B, NO_DATA);
    instrMapCB.emplace_back(0x99, &GameboyCPU::RES3, "RES3", C, NO_DATA);
    instrMapCB.emplace_back(0x9a, &GameboyCPU::RES3, "RES3", D, NO_DATA);
    instrMapCB.emplace_back(0x9b, &GameboyCPU::RES3, "RES3", E, NO_DATA);
    instrMapCB.emplace_back(0x9c, &GameboyCPU::RES3, "RES3", H, NO_DATA);
    instrMapCB.emplace_back(0x9d, &GameboyCPU::RES3, "RES3", L, NO_DATA);
    instrMapCB.emplace_back(0x9e, &GameboyCPU::RES3, "RES3", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x9f, &GameboyCPU::RES3, "RES3", A, NO_DATA);

    instrMapCB.emplace_back(0xa0, &GameboyCPU::RES4, "RES4", B, NO_DATA);
    instrMapCB.emplace_back(0xa1, &GameboyCPU::RES4, "RES4", C, NO_DATA);
    instrMapCB.emplace_back(0xa2, &GameboyCPU::RES4, "RES4", D, NO_DATA);
    instrMapCB.emplace_back(0xa3, &GameboyCPU::RES4, "RES4", E, NO_DATA);
    instrMapCB.emplace_back(0xa4, &GameboyCPU::RES4, "RES4", H, NO_DATA);
    instrMapCB.emplace_back(0xa5, &GameboyCPU::RES4, "RES4", L, NO_DATA);
    instrMapCB.emplace_back(0xa6, &GameboyCPU::RES4, "RES4", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xa7, &GameboyCPU::RES4, "RES4", A, NO_DATA);
    instrMapCB.emplace_back(0xa8, &GameboyCPU::RES5, "RES5", B, NO_DATA);
    instrMapCB.emplace_back(0xa9, &GameboyCPU::RES5, "RES5", C, NO_DATA);
    instrMapCB.emplace_back(0xaa, &GameboyCPU::RES5, "RES5", D, NO_DATA);
    instrMapCB.emplace_back(0xab, &GameboyCPU::RES5, "RES5", E, NO_DATA);
    instrMapCB.emplace_back(0xac, &GameboyCPU::RES5, "RES5", H, NO_DATA);
    instrMapCB.emplace_back(0xad, &GameboyCPU::RES5, "RES5", L, NO_DATA);
    instrMapCB.emplace_back(0xae, &GameboyCPU::RES5, "RES5", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xaf, &GameboyCPU::RES5, "RES5", A, NO_DATA);

    instrMapCB.emplace_back(0xb0, &GameboyCPU::RES6, "RES6", B, NO_DATA);
    instrMapCB.emplace_back(0xb1, &GameboyCPU::RES6, "RES6", C, NO_DATA);
    instrMapCB.emplace_back(0xb2, &GameboyCPU::RES6, "RES6", D, NO_DATA);
    instrMapCB.emplace_back(0xb3, &GameboyCPU::RES6, "RES6", E, NO_DATA);
    instrMapCB.emplace_back(0xb4, &GameboyCPU::RES6, "RES6", H, NO_DATA);
    instrMapCB.emplace_back(0xb5, &GameboyCPU::RES6, "RES6", L, NO_DATA);
    instrMapCB.emplace_back(0xb6, &GameboyCPU::RES6, "RES6", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xb7, &GameboyCPU::RES6, "RES6", A, NO_DATA);
    instrMapCB.emplace_back(0xb8, &GameboyCPU::RES7, "RES7", B, NO_DATA);
    instrMapCB.emplace_back(0xb9, &GameboyCPU::RES7, "RES7", C, NO_DATA);
    instrMapCB.emplace_back(0xba, &GameboyCPU::RES7, "RES7", D, NO_DATA);
    instrMapCB.emplace_back(0xbb, &GameboyCPU::RES7, "RES7", E, NO_DATA);
    instrMapCB.emplace_back(0xbc, &GameboyCPU::RES7, "RES7", H, NO_DATA);
    instrMapCB.emplace_back(0xbd, &GameboyCPU::RES7, "RES7", L, NO_DATA);
    instrMapCB.emplace_back(0xbe, &GameboyCPU::RES7, "RES7", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xbf, &GameboyCPU::RES7, "RES7", A, NO_DATA);

    instrMapCB.emplace_back(0xc0, &GameboyCPU::SET0, "SET0", B, NO_DATA);
    instrMapCB.emplace_back(0xc1, &GameboyCPU::SET0, "SET0", C, NO_DATA);
    instrMapCB.emplace_back(0xc2, &GameboyCPU::SET0, "SET0", D, NO_DATA);
    instrMapCB.emplace_back(0xc3, &GameboyCPU::SET0, "SET0", E, NO_DATA);
    instrMapCB.emplace_back(0xc4, &GameboyCPU::SET0, "SET0", H, NO_DATA);
    instrMapCB.emplace_back(0xc5, &GameboyCPU::SET0, "SET0", L, NO_DATA);
    instrMapCB.emplace_back(0xc6, &GameboyCPU::SET0, "SET0", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xc7, &GameboyCPU::SET0, "SET0", A, NO_DATA);
    instrMapCB.emplace_back(0xc8, &GameboyCPU::SET1, "SET1", B, NO_DATA);
    instrMapCB.emplace_back(0xc9, &GameboyCPU::SET1, "SET1", C, NO_DATA);
    instrMapCB.emplace_back(0xca, &GameboyCPU::SET1, "SET1", D, NO_DATA);
    instrMapCB.emplace_back(0xcb, &GameboyCPU::SET1, "SET1", E, NO_DATA);
    instrMapCB.emplace_back(0xcc, &GameboyCPU::SET1, "SET1", H, NO_DATA);
    instrMapCB.emplace_back(0xcd, &GameboyCPU::SET1, "SET1", L, NO_DATA);
    instrMapCB.emplace_back(0xce, &GameboyCPU::SET1, "SET1", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xcf, &GameboyCPU::SET1, "SET1", A, NO_DATA);

    instrMapCB.emplace_back(0xd0, &GameboyCPU::SET2, "SET2", B, NO_DATA);
    instrMapCB.emplace_back(0xd1, &GameboyCPU::SET2, "SET2", C, NO_DATA);
    instrMapCB.emplace_back(0xd2, &GameboyCPU::SET2, "SET2", D, NO_DATA);
    instrMapCB.emplace_back(0xd3, &GameboyCPU::SET2, "SET2", E, NO_DATA);
    instrMapCB.emplace_back(0xd4, &GameboyCPU::SET2, "SET2", H, NO_DATA);
    instrMapCB.emplace_back(0xd5, &GameboyCPU::SET2, "SET2", L, NO_DATA);
    instrMapCB.emplace_back(0xd6, &GameboyCPU::SET2, "SET2", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xd7, &GameboyCPU::SET2, "SET2", A, NO_DATA);
    instrMapCB.emplace_back(0xd8, &GameboyCPU::SET3, "SET3", B, NO_DATA);
    instrMapCB.emplace_back(0xd9, &GameboyCPU::SET3, "SET3", C, NO_DATA);
    instrMapCB.emplace_back(0xda, &GameboyCPU::SET3, "SET3", D, NO_DATA);
    instrMapCB.emplace_back(0xdb, &GameboyCPU::SET3, "SET3", E, NO_DATA);
    instrMapCB.emplace_back(0xdc, &GameboyCPU::SET3, "SET3", H, NO_DATA);
    instrMapCB.emplace_back(0xdd, &GameboyCPU::SET3, "SET3", L, NO_DATA);
    instrMapCB.emplace_back(0xde, &GameboyCPU::SET3, "SET3", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xdf, &GameboyCPU::SET3, "SET3", A, NO_DATA);

    instrMapCB.emplace_back(0xe0, &GameboyCPU::SET4, "SET4", B, NO_DATA);
    instrMapCB.emplace_back(0xe1, &GameboyCPU::SET4, "SET4", C, NO_DATA);
    instrMapCB.emplace_back(0xe2, &GameboyCPU::SET4, "SET4", D, NO_DATA);
    instrMapCB.emplace_back(0xe3, &GameboyCPU::SET4, "SET4", E, NO_DATA);
    instrMapCB.emplace_back(0xe4, &GameboyCPU::SET4, "SET4", H, NO_DATA);
    instrMapCB.emplace_back(0xe5, &GameboyCPU::SET4, "SET4", L, NO_DATA);
    instrMapCB.emplace_back(0xe6, &GameboyCPU::SET4, "SET4", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xe7, &GameboyCPU::SET4, "SET4", A, NO_DATA);
    instrMapCB.emplace_back(0xe8, &GameboyCPU::SET5, "SET5", B, NO_DATA);
    instrMapCB.emplace_back(0xe9, &GameboyCPU::SET5, "SET5", C, NO_DATA);
    instrMapCB.emplace_back(0xea, &GameboyCPU::SET5, "SET5", D, NO_DATA);
    instrMapCB.emplace_back(0xeb, &GameboyCPU::SET5, "SET5", E, NO_DATA);
    instrMapCB.emplace_back(0xec, &GameboyCPU::SET5, "SET5", H, NO_DATA);
    instrMapCB.emplace_back(0xed, &GameboyCPU::SET5, "SET5", L, NO_DATA);
    instrMapCB.emplace_back(0xee, &GameboyCPU::SET5, "SET5", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xef, &GameboyCPU::SET5, "SET5", A, NO_DATA);

    instrMapCB.emplace_back(0xf0, &GameboyCPU::SET6, "SET6", B, NO_DATA);
    instrMapCB.emplace_back(0xf1, &GameboyCPU::SET6, "SET6", C, NO_DATA);
    instrMapCB.emplace_back(0xf2, &GameboyCPU::SET6, "SET6", D, NO_DATA);
    instrMapCB.emplace_back(0xf3, &GameboyCPU::SET6, "SET6", E, NO_DATA);
    instrMapCB.emplace_back(0xf4, &GameboyCPU::SET6, "SET6", H, NO_DATA);
    instrMapCB.emplace_back(0xf5, &GameboyCPU::SET6, "SET6", L, NO_DATA);
    instrMapCB.emplace_back(0xf6, &GameboyCPU::SET6, "SET6", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xf7, &GameboyCPU::SET6, "SET6", A, NO_DATA);
    instrMapCB.emplace_back(0xf8, &GameboyCPU::SET7, "SET7", B, NO_DATA);
    instrMapCB.emplace_back(0xf9, &GameboyCPU::SET7, "SET7", C, NO_DATA);
    instrMapCB.emplace_back(0xfa, &GameboyCPU::SET7, "SET7", D, NO_DATA);
    instrMapCB.emplace_back(0xfb, &GameboyCPU::SET7, "SET7", E, NO_DATA);
    instrMapCB.emplace_back(0xfc, &GameboyCPU::SET7, "SET7", H, NO_DATA);
    instrMapCB.emplace_back(0xfd, &GameboyCPU::SET7, "SET7", L, NO_DATA);
    instrMapCB.emplace_back(0xfe, &GameboyCPU::SET7, "SET7", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xff, &GameboyCPU::SET7, "SET7", A, NO_DATA);
}

/* ***********************************************************************************************************
*
*   GAMEBOY (COLOR) GameboyCPU CB INSTRUCTION SET
*
*********************************************************************************************************** */
// rotate left
void GameboyCPU::RLC() {
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode & 0x07) {
    case 0x00:
        Regs.F |= (Regs.BC_.B & MSB ? FLAG_CARRY : 0x00);
        Regs.BC_.B <<= 1;
        Regs.BC_.B |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x01:
        Regs.F |= (Regs.BC_.C & MSB ? FLAG_CARRY : 0x00);
        Regs.BC_.C <<= 1;
        Regs.BC_.C |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x02:
        Regs.F |= (Regs.DE_.D & MSB ? FLAG_CARRY : 0x00);
        Regs.DE_.D <<= 1;
        Regs.DE_.D |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x03:
        Regs.F |= (Regs.DE_.E & MSB ? FLAG_CARRY : 0x00);
        Regs.DE_.E <<= 1;
        Regs.DE_.E |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x04:
        Regs.F |= (Regs.HL_.H & MSB ? FLAG_CARRY : 0x00);
        Regs.HL_.H <<= 1;
        Regs.HL_.H |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x05:
        Regs.F |= (Regs.HL_.L & MSB ? FLAG_CARRY : 0x00);
        Regs.HL_.L <<= 1;
        Regs.HL_.L |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        Regs.F |= (data & MSB ? FLAG_CARRY : 0x00);
        data = (data << 1) & 0xFF;
        data |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
        ZERO_FLAG(data, Regs.F);
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.F |= (Regs.A & MSB ? FLAG_CARRY : 0x00);
        Regs.A <<= 1;
        Regs.A |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// rotate right
void GameboyCPU::RRC() {
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode & 0x07) {
    case 0x00:
        Regs.F |= (Regs.BC_.B & LSB ? FLAG_CARRY : 0x00);
        Regs.BC_.B >>= 1;
        Regs.BC_.B |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x01:
        Regs.F |= (Regs.BC_.C & LSB ? FLAG_CARRY : 0x00);
        Regs.BC_.C >>= 1;
        Regs.BC_.C |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x02:
        Regs.F |= (Regs.DE_.D & LSB ? FLAG_CARRY : 0x00);
        Regs.DE_.D >>= 1;
        Regs.DE_.D |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x03:
        Regs.F |= (Regs.DE_.E & LSB ? FLAG_CARRY : 0x00);
        Regs.DE_.E >>= 1;
        Regs.DE_.E |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x04:
        Regs.F |= (Regs.HL_.H & LSB ? FLAG_CARRY : 0x00);
        Regs.HL_.H >>= 1;
        Regs.HL_.H |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x05:
        Regs.F |= (Regs.HL_.L & LSB ? FLAG_CARRY : 0x00);
        Regs.HL_.L >>= 1;
        Regs.HL_.L |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        Regs.F |= (data & LSB ? FLAG_CARRY : 0x00);
        data = (data >> 1) & 0xFF;
        data |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
        ZERO_FLAG(data, Regs.F);
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.F |= (Regs.A & LSB ? FLAG_CARRY : 0x00);
        Regs.A >>= 1;
        Regs.A |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// rotate left through carry
void GameboyCPU::RL() {
    bool carry = Regs.F & FLAG_CARRY;
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode & 0x07) {
    case 0x00:
        Regs.F |= (Regs.BC_.B & MSB ? FLAG_CARRY : 0x00);
        Regs.BC_.B <<= 1;
        Regs.BC_.B |= (carry ? LSB : 0x00);
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x01:
        Regs.F |= (Regs.BC_.C & MSB ? FLAG_CARRY : 0x00);
        Regs.BC_.C <<= 1;
        Regs.BC_.C |= (carry ? LSB : 0x00);
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x02:
        Regs.F |= (Regs.DE_.D & MSB ? FLAG_CARRY : 0x00);
        Regs.DE_.D <<= 1;
        Regs.DE_.D |= (carry ? LSB : 0x00);
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x03:
        Regs.F |= (Regs.DE_.E & MSB ? FLAG_CARRY : 0x00);
        Regs.DE_.E <<= 1;
        Regs.DE_.E |= (carry ? LSB : 0x00);
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x04:
        Regs.F |= (Regs.HL_.H & MSB ? FLAG_CARRY : 0x00);
        Regs.HL_.H <<= 1;
        Regs.HL_.H |= (carry ? LSB : 0x00);
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x05:
        Regs.F |= (Regs.HL_.L & MSB ? FLAG_CARRY : 0x00);
        Regs.HL_.L <<= 1;
        Regs.HL_.L |= (carry ? LSB : 0x00);
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        Regs.F |= (data & MSB ? FLAG_CARRY : 0x00);
        data = (data << 1) & 0xFF;
        data |= (carry ? LSB : 0x00);
        ZERO_FLAG(data, Regs.F);
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.F |= (Regs.A & MSB ? FLAG_CARRY : 0x00);
        Regs.A <<= 1;
        Regs.A |= (carry ? LSB : 0x00);
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// rotate right through carry
void GameboyCPU::RR() {
    bool carry = Regs.F & FLAG_CARRY;
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode & 0x07) {
    case 0x00:
        Regs.F |= (Regs.BC_.B & LSB ? FLAG_CARRY : 0x00);
        Regs.BC_.B >>= 1;
        Regs.BC_.B |= (carry ? MSB : 0x00);
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x01:
        Regs.F |= (Regs.BC_.C & LSB ? FLAG_CARRY : 0x00);
        Regs.BC_.C >>= 1;
        Regs.BC_.C |= (carry ? MSB : 0x00);
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x02:
        Regs.F |= (Regs.DE_.D & LSB ? FLAG_CARRY : 0x00);
        Regs.DE_.D >>= 1;
        Regs.DE_.D |= (carry ? MSB : 0x00);
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x03:
        Regs.F |= (Regs.DE_.E & LSB ? FLAG_CARRY : 0x00);
        Regs.DE_.E >>= 1;
        Regs.DE_.E |= (carry ? MSB : 0x00);
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x04:
        Regs.F |= (Regs.HL_.H & LSB ? FLAG_CARRY : 0x00);
        Regs.HL_.H >>= 1;
        Regs.HL_.H |= (carry ? MSB : 0x00);
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x05:
        Regs.F |= (Regs.HL_.L & LSB ? FLAG_CARRY : 0x00);
        Regs.HL_.L >>= 1;
        Regs.HL_.L |= (carry ? MSB : 0x00);
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        Regs.F |= (data & LSB ? FLAG_CARRY : 0x00);
        data = (data >> 1) & 0xFF;
        data |= (carry ? MSB : 0x00);
        ZERO_FLAG(data, Regs.F);
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.F |= (Regs.A & LSB ? FLAG_CARRY : 0x00);
        Regs.A >>= 1;
        Regs.A |= (carry ? MSB : 0x00);
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// shift left arithmetic (multiply by 2)
void GameboyCPU::SLA() {
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode & 0x07) {
    case 0x00:
        Regs.F |= (Regs.BC_.B & MSB ? FLAG_CARRY : 0x00);
        Regs.BC_.B <<= 1;
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x01:
        Regs.F |= (Regs.BC_.C & MSB ? FLAG_CARRY : 0x00);
        Regs.BC_.C <<= 1;
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x02:
        Regs.F |= (Regs.DE_.D & MSB ? FLAG_CARRY : 0x00);
        Regs.DE_.D <<= 1;
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x03:
        Regs.F |= (Regs.DE_.E & MSB ? FLAG_CARRY : 0x00);
        Regs.DE_.E <<= 1;
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x04:
        Regs.F |= (Regs.HL_.H & MSB ? FLAG_CARRY : 0x00);
        Regs.HL_.H <<= 1;
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x05:
        Regs.F |= (Regs.HL_.L & MSB ? FLAG_CARRY : 0x00);
        Regs.HL_.L <<= 1;
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        Regs.F |= (data & MSB ? FLAG_CARRY : 0x00);
        data = (data << 1) & 0xFF;
        ZERO_FLAG(data, Regs.F);
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.F |= (Regs.A & MSB ? FLAG_CARRY : 0x00);
        Regs.A <<= 1;
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// shift right arithmetic
void GameboyCPU::SRA() {
    RESET_ALL_FLAGS(Regs.F);
    u8 msb;

    switch (opcode & 0x07) {
    case 0x00:
        msb = (Regs.BC_.B & MSB);
        Regs.F |= (Regs.BC_.B & LSB ? FLAG_CARRY : 0x00);
        Regs.BC_.B >>= 1;
        Regs.BC_.B |= msb;
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x01:
        msb = (Regs.BC_.C & MSB);
        Regs.F |= (Regs.BC_.C & LSB ? FLAG_CARRY : 0x00);
        Regs.BC_.C >>= 1;
        Regs.BC_.C |= msb;
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x02:
        msb = (Regs.DE_.D & MSB);
        Regs.F |= (Regs.DE_.D & LSB ? FLAG_CARRY : 0x00);
        Regs.DE_.D >>= 1;
        Regs.DE_.D |= msb;
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x03:
        msb = (Regs.DE_.E & MSB);
        Regs.F |= (Regs.DE_.E & LSB ? FLAG_CARRY : 0x00);
        Regs.DE_.E >>= 1;
        Regs.DE_.E |= msb;
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x04:
        msb = (Regs.HL_.H & MSB);
        Regs.F |= (Regs.HL_.H & LSB ? FLAG_CARRY : 0x00);
        Regs.HL_.H >>= 1;
        Regs.HL_.H |= msb;
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x05:
        msb = (Regs.HL_.L & MSB);
        Regs.F |= (Regs.HL_.L & LSB ? FLAG_CARRY : 0x00);
        Regs.HL_.L >>= 1;
        Regs.HL_.L |= msb;
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        msb = (data & MSB);
        Regs.F |= (data & LSB ? FLAG_CARRY : 0x00);
        data >>= 1;
        data |= msb;
        ZERO_FLAG(data, Regs.F);
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        msb = (Regs.A & MSB);
        Regs.F |= (Regs.A & LSB ? FLAG_CARRY : 0x00);
        Regs.A >>= 1;
        Regs.A |= msb;
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// swap lo<->hi nibble
void GameboyCPU::SWAP() {
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B = (Regs.BC_.B >> 4) | (Regs.BC_.B << 4);
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x01:
        Regs.BC_.C = (Regs.BC_.C >> 4) | (Regs.BC_.C << 4);
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x02:
        Regs.DE_.D = (Regs.DE_.D >> 4) | (Regs.DE_.D << 4);
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x03:
        Regs.DE_.E = (Regs.DE_.E >> 4) | (Regs.DE_.E << 4);
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x04:
        Regs.HL_.H = (Regs.HL_.H >> 4) | (Regs.HL_.H << 4);
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x05:
        Regs.HL_.L = (Regs.HL_.L >> 4) | (Regs.HL_.L << 4);
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        data = (data >> 4) | ((data << 4) & 0xF0);
        ZERO_FLAG(data, Regs.F);
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.A = (Regs.A >> 4) | (Regs.A << 4);
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// shift right logical
void GameboyCPU::SRL() {
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode & 0x07) {
    case 0x00:
        Regs.F |= (Regs.BC_.B & LSB ? FLAG_CARRY : 0x00);
        Regs.BC_.B >>= 1;
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x01:
        Regs.F |= (Regs.BC_.C & LSB ? FLAG_CARRY : 0x00);
        Regs.BC_.C >>= 1;
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x02:
        Regs.F |= (Regs.DE_.D & LSB ? FLAG_CARRY : 0x00);
        Regs.DE_.D >>= 1;
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x03:
        Regs.F |= (Regs.DE_.E & LSB ? FLAG_CARRY : 0x00);
        Regs.DE_.E >>= 1;
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x04:
        Regs.F |= (Regs.HL_.H & LSB ? FLAG_CARRY : 0x00);
        Regs.HL_.H >>= 1;
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x05:
        Regs.F |= (Regs.HL_.L & LSB ? FLAG_CARRY : 0x00);
        Regs.HL_.L >>= 1;
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        Regs.F |= (data & LSB ? FLAG_CARRY : 0x00);
        data = (data >> 1) & 0xFF;
        ZERO_FLAG(data, Regs.F);
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.F |= (Regs.A & LSB ? FLAG_CARRY : 0x00);
        Regs.A >>= 1;
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// test bit 0
void GameboyCPU::BIT0() {
    RESET_FLAGS(FLAG_ZERO | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_HCARRY;

    switch (opcode & 0x07) {
    case 0x00:
        ZERO_FLAG(Regs.BC_.B & 0x01, Regs.F);
        break;
    case 0x01:
        ZERO_FLAG(Regs.BC_.C & 0x01, Regs.F);
        break;
    case 0x02:
        ZERO_FLAG(Regs.DE_.D & 0x01, Regs.F);
        break;
    case 0x03:
        ZERO_FLAG(Regs.DE_.E & 0x01, Regs.F);
        break;
    case 0x04:
        ZERO_FLAG(Regs.HL_.H & 0x01, Regs.F);
        break;
    case 0x05:
        ZERO_FLAG(Regs.HL_.L & 0x01, Regs.F);
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x01, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x01, Regs.F);
        break;
    }
}

// test bit 1
void GameboyCPU::BIT1() {
    RESET_FLAGS(FLAG_ZERO | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_HCARRY;

    switch (opcode & 0x07) {
    case 0x00:
        ZERO_FLAG(Regs.BC_.B & 0x02, Regs.F);
        break;
    case 0x01:
        ZERO_FLAG(Regs.BC_.C & 0x02, Regs.F);
        break;
    case 0x02:
        ZERO_FLAG(Regs.DE_.D & 0x02, Regs.F);
        break;
    case 0x03:
        ZERO_FLAG(Regs.DE_.E & 0x02, Regs.F);
        break;
    case 0x04:
        ZERO_FLAG(Regs.HL_.H & 0x02, Regs.F);
        break;
    case 0x05:
        ZERO_FLAG(Regs.HL_.L & 0x02, Regs.F);
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x02, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x02, Regs.F);
        break;
    }
}

// test bit 2
void GameboyCPU::BIT2() {
    RESET_FLAGS(FLAG_ZERO | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_HCARRY;

    switch (opcode & 0x07) {
    case 0x00:
        ZERO_FLAG(Regs.BC_.B & 0x04, Regs.F);
        break;
    case 0x01:
        ZERO_FLAG(Regs.BC_.C & 0x04, Regs.F);
        break;
    case 0x02:
        ZERO_FLAG(Regs.DE_.D & 0x04, Regs.F);
        break;
    case 0x03:
        ZERO_FLAG(Regs.DE_.E & 0x04, Regs.F);
        break;
    case 0x04:
        ZERO_FLAG(Regs.HL_.H & 0x04, Regs.F);
        break;
    case 0x05:
        ZERO_FLAG(Regs.HL_.L & 0x04, Regs.F);
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x04, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x04, Regs.F);
        break;
    }
}

// test bit 3
void GameboyCPU::BIT3() {
    RESET_FLAGS(FLAG_ZERO | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_HCARRY;

    switch (opcode & 0x07) {
    case 0x00:
        ZERO_FLAG(Regs.BC_.B & 0x08, Regs.F);
        break;
    case 0x01:
        ZERO_FLAG(Regs.BC_.C & 0x08, Regs.F);
        break;
    case 0x02:
        ZERO_FLAG(Regs.DE_.D & 0x08, Regs.F);
        break;
    case 0x03:
        ZERO_FLAG(Regs.DE_.E & 0x08, Regs.F);
        break;
    case 0x04:
        ZERO_FLAG(Regs.HL_.H & 0x08, Regs.F);
        break;
    case 0x05:
        ZERO_FLAG(Regs.HL_.L & 0x08, Regs.F);
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x08, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x08, Regs.F);
        break;
    }
}

// test bit 4
void GameboyCPU::BIT4() {
    RESET_FLAGS(FLAG_ZERO | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_HCARRY;

    switch (opcode & 0x07) {
    case 0x00:
        ZERO_FLAG(Regs.BC_.B & 0x10, Regs.F);
        break;
    case 0x01:
        ZERO_FLAG(Regs.BC_.C & 0x10, Regs.F);
        break;
    case 0x02:
        ZERO_FLAG(Regs.DE_.D & 0x10, Regs.F);
        break;
    case 0x03:
        ZERO_FLAG(Regs.DE_.E & 0x10, Regs.F);
        break;
    case 0x04:
        ZERO_FLAG(Regs.HL_.H & 0x10, Regs.F);
        break;
    case 0x05:
        ZERO_FLAG(Regs.HL_.L & 0x10, Regs.F);
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x10, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x10, Regs.F);
        break;
    }
}

// test bit 5
void GameboyCPU::BIT5() {
    RESET_FLAGS(FLAG_ZERO | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_HCARRY;

    switch (opcode & 0x07) {
    case 0x00:
        ZERO_FLAG(Regs.BC_.B & 0x20, Regs.F);
        break;
    case 0x01:
        ZERO_FLAG(Regs.BC_.C & 0x20, Regs.F);
        break;
    case 0x02:
        ZERO_FLAG(Regs.DE_.D & 0x20, Regs.F);
        break;
    case 0x03:
        ZERO_FLAG(Regs.DE_.E & 0x20, Regs.F);
        break;
    case 0x04:
        ZERO_FLAG(Regs.HL_.H & 0x20, Regs.F);
        break;
    case 0x05:
        ZERO_FLAG(Regs.HL_.L & 0x20, Regs.F);
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x20, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x20, Regs.F);
        break;
    }
}

// test bit 6
void GameboyCPU::BIT6() {
    RESET_FLAGS(FLAG_ZERO | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_HCARRY;

    switch (opcode & 0x07) {
    case 0x00:
        ZERO_FLAG(Regs.BC_.B & 0x40, Regs.F);
        break;
    case 0x01:
        ZERO_FLAG(Regs.BC_.C & 0x40, Regs.F);
        break;
    case 0x02:
        ZERO_FLAG(Regs.DE_.D & 0x40, Regs.F);
        break;
    case 0x03:
        ZERO_FLAG(Regs.DE_.E & 0x40, Regs.F);
        break;
    case 0x04:
        ZERO_FLAG(Regs.HL_.H & 0x40, Regs.F);
        break;
    case 0x05:
        ZERO_FLAG(Regs.HL_.L & 0x40, Regs.F);
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x40, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x40, Regs.F);
        break;
    }
}

// test bit 7
void GameboyCPU::BIT7() {
    RESET_FLAGS(FLAG_ZERO | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_HCARRY;

    switch (opcode & 0x07) {
    case 0x00:
        ZERO_FLAG(Regs.BC_.B & 0x80, Regs.F);
        break;
    case 0x01:
        ZERO_FLAG(Regs.BC_.C & 0x80, Regs.F);
        break;
    case 0x02:
        ZERO_FLAG(Regs.DE_.D & 0x80, Regs.F);
        break;
    case 0x03:
        ZERO_FLAG(Regs.DE_.E & 0x80, Regs.F);
        break;
    case 0x04:
        ZERO_FLAG(Regs.HL_.H & 0x80, Regs.F);
        break;
    case 0x05:
        ZERO_FLAG(Regs.HL_.L & 0x80, Regs.F);
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x80, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x80, Regs.F);
        break;
    }
}

// reset bit 0
void GameboyCPU::RES0() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B &= ~0x01;
        break;
    case 0x01:
        Regs.BC_.C &= ~0x01;
        break;
    case 0x02:
        Regs.DE_.D &= ~0x01;
        break;
    case 0x03:
        Regs.DE_.E &= ~0x01;
        break;
    case 0x04:
        Regs.HL_.H &= ~0x01;
        break;
    case 0x05:
        Regs.HL_.L &= ~0x01;
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        data &= ~0x01;
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x01;
        break;
    }
}

// reset bit 1
void GameboyCPU::RES1() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B &= ~0x02;
        break;
    case 0x01:
        Regs.BC_.C &= ~0x02;
        break;
    case 0x02:
        Regs.DE_.D &= ~0x02;
        break;
    case 0x03:
        Regs.DE_.E &= ~0x02;
        break;
    case 0x04:
        Regs.HL_.H &= ~0x02;
        break;
    case 0x05:
        Regs.HL_.L &= ~0x02;
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        data &= ~0x02;
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x02;
        break;
    }
}

// reset bit 2
void GameboyCPU::RES2() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B &= ~0x04;
        break;
    case 0x01:
        Regs.BC_.C &= ~0x04;
        break;
    case 0x02:
        Regs.DE_.D &= ~0x04;
        break;
    case 0x03:
        Regs.DE_.E &= ~0x04;
        break;
    case 0x04:
        Regs.HL_.H &= ~0x04;
        break;
    case 0x05:
        Regs.HL_.L &= ~0x04;
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        data &= ~0x04;
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x04;
        break;
    }
}

// reset bit 3
void GameboyCPU::RES3() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B &= ~0x08;
        break;
    case 0x01:
        Regs.BC_.C &= ~0x08;
        break;
    case 0x02:
        Regs.DE_.D &= ~0x08;
        break;
    case 0x03:
        Regs.DE_.E &= ~0x08;
        break;
    case 0x04:
        Regs.HL_.H &= ~0x08;
        break;
    case 0x05:
        Regs.HL_.L &= ~0x08;
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        data &= ~0x08;
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x08;
        break;
    }
}

// reset bit 4
void GameboyCPU::RES4() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B &= ~0x10;
        break;
    case 0x01:
        Regs.BC_.C &= ~0x10;
        break;
    case 0x02:
        Regs.DE_.D &= ~0x10;
        break;
    case 0x03:
        Regs.DE_.E &= ~0x10;
        break;
    case 0x04:
        Regs.HL_.H &= ~0x10;
        break;
    case 0x05:
        Regs.HL_.L &= ~0x10;
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        data &= ~0x10;
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x10;
        break;
    }
}

// reset bit 5
void GameboyCPU::RES5() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B &= ~0x20;
        break;
    case 0x01:
        Regs.BC_.C &= ~0x20;
        break;
    case 0x02:
        Regs.DE_.D &= ~0x20;
        break;
    case 0x03:
        Regs.DE_.E &= ~0x20;
        break;
    case 0x04:
        Regs.HL_.H &= ~0x20;
        break;
    case 0x05:
        Regs.HL_.L &= ~0x20;
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        data &= ~0x20;
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x20;
        break;
    }
}

// reset bit 6
void GameboyCPU::RES6() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B &= ~0x40;
        break;
    case 0x01:
        Regs.BC_.C &= ~0x40;
        break;
    case 0x02:
        Regs.DE_.D &= ~0x40;
        break;
    case 0x03:
        Regs.DE_.E &= ~0x40;
        break;
    case 0x04:
        Regs.HL_.H &= ~0x40;
        break;
    case 0x05:
        Regs.HL_.L &= ~0x40;
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        data &= ~0x40;
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x40;
        break;
    }
}

// reset bit 7
void GameboyCPU::RES7() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B &= ~0x80;
        break;
    case 0x01:
        Regs.BC_.C &= ~0x80;
        break;
    case 0x02:
        Regs.DE_.D &= ~0x80;
        break;
    case 0x03:
        Regs.DE_.E &= ~0x80;
        break;
    case 0x04:
        Regs.HL_.H &= ~0x80;
        break;
    case 0x05:
        Regs.HL_.L &= ~0x80;
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        data &= ~0x80;
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x80;
        break;
    }
}

// set bit 0
void GameboyCPU::SET0() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B |= 0x01;
        break;
    case 0x01:
        Regs.BC_.C |= 0x01;
        break;
    case 0x02:
        Regs.DE_.D |= 0x01;
        break;
    case 0x03:
        Regs.DE_.E |= 0x01;
        break;
    case 0x04:
        Regs.HL_.H |= 0x01;
        break;
    case 0x05:
        Regs.HL_.L |= 0x01;
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        data |= 0x01;
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x01;
        break;
    }
}

// set bit 1
void GameboyCPU::SET1() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B |= 0x02;
        break;
    case 0x01:
        Regs.BC_.C |= 0x02;
        break;
    case 0x02:
        Regs.DE_.D |= 0x02;
        break;
    case 0x03:
        Regs.DE_.E |= 0x02;
        break;
    case 0x04:
        Regs.HL_.H |= 0x02;
        break;
    case 0x05:
        Regs.HL_.L |= 0x02;
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        data |= 0x02;
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x02;
        break;
    }
}

// set bit 2
void GameboyCPU::SET2() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B |= 0x04;
        break;
    case 0x01:
        Regs.BC_.C |= 0x04;
        break;
    case 0x02:
        Regs.DE_.D |= 0x04;
        break;
    case 0x03:
        Regs.DE_.E |= 0x04;
        break;
    case 0x04:
        Regs.HL_.H |= 0x04;
        break;
    case 0x05:
        Regs.HL_.L |= 0x04;
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        data |= 0x04;
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x04;
        break;
    }
}

// set bit 3
void GameboyCPU::SET3() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B |= 0x08;
        break;
    case 0x01:
        Regs.BC_.C |= 0x08;
        break;
    case 0x02:
        Regs.DE_.D |= 0x08;
        break;
    case 0x03:
        Regs.DE_.E |= 0x08;
        break;
    case 0x04:
        Regs.HL_.H |= 0x08;
        break;
    case 0x05:
        Regs.HL_.L |= 0x08;
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        data |= 0x08;
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x08;
        break;
    }
}

// set bit 4
void GameboyCPU::SET4() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B |= 0x10;
        break;
    case 0x01:
        Regs.BC_.C |= 0x10;
        break;
    case 0x02:
        Regs.DE_.D |= 0x10;
        break;
    case 0x03:
        Regs.DE_.E |= 0x10;
        break;
    case 0x04:
        Regs.HL_.H |= 0x10;
        break;
    case 0x05:
        Regs.HL_.L |= 0x10;
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        data |= 0x10;
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x10;
        break;
    }
}

// set bit 5
void GameboyCPU::SET5() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B |= 0x20;
        break;
    case 0x01:
        Regs.BC_.C |= 0x20;
        break;
    case 0x02:
        Regs.DE_.D |= 0x20;
        break;
    case 0x03:
        Regs.DE_.E |= 0x20;
        break;
    case 0x04:
        Regs.HL_.H |= 0x20;
        break;
    case 0x05:
        Regs.HL_.L |= 0x20;
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        data |= 0x20;
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x20;
        break;
    }
}

// set bit 6
void GameboyCPU::SET6() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B |= 0x40;
        break;
    case 0x01:
        Regs.BC_.C |= 0x40;
        break;
    case 0x02:
        Regs.DE_.D |= 0x40;
        break;
    case 0x03:
        Regs.DE_.E |= 0x40;
        break;
    case 0x04:
        Regs.HL_.H |= 0x40;
        break;
    case 0x05:
        Regs.HL_.L |= 0x40;
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        data |= 0x40;
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x40;
        break;
    }
}

// set bit 7
void GameboyCPU::SET7() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B |= 0x80;
        break;
    case 0x01:
        Regs.BC_.C |= 0x80;
        break;
    case 0x02:
        Regs.DE_.D |= 0x80;
        break;
    case 0x03:
        Regs.DE_.E |= 0x80;
        break;
    case 0x04:
        Regs.HL_.H |= 0x80;
        break;
    case 0x05:
        Regs.HL_.L |= 0x80;
        break;
    case 0x06:
        data = Read8Bit(Regs.HL);
        data |= 0x80;
        Write8Bit((u8)data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x80;
        break;
    }
}

/* ***********************************************************************************************************
    ACCESS HARDWARE STATUS
*********************************************************************************************************** */
// get current hardware status (currently mapped memory banks, etc.)
void GameboyCPU::GetHardwareInfo(std::vector<data_entry>& _hardware_info) const {
    _hardware_info.clear();
    _hardware_info.emplace_back("Speedmode", format("{:d}", machine_ctx->currentSpeed));
    _hardware_info.emplace_back("ROM banks", format("{:d}", machine_ctx->rom_bank_num));
    _hardware_info.emplace_back("ROM selected", format("{:d}", machine_ctx->rom_bank_selected + 1));
    _hardware_info.emplace_back("RAM banks", format("{:d}", machine_ctx->ram_bank_num));
    _hardware_info.emplace_back("RAM selected", format("{:d}", machine_ctx->ram_bank_selected));
    _hardware_info.emplace_back("WRAM banks", format("{:d}", machine_ctx->wram_bank_num));
    _hardware_info.emplace_back("WRAM selected", format("{:d}", machine_ctx->wram_bank_selected + 1));
    _hardware_info.emplace_back("VRAM banks", format("{:d}", machine_ctx->vram_bank_num));
    _hardware_info.emplace_back("VRAM selected", format("{:d}", machine_ctx->vram_bank_selected));
}

void GameboyCPU::GetInstrDebugFlags(std::vector<reg_entry>& _register_values, std::vector<reg_entry>& _flag_values, std::vector<reg_entry>& _misc_values) const {
    _register_values.clear();
    _register_values.emplace_back(REGISTER_NAMES.at(A) + REGISTER_NAMES.at(F), format("A:{:02x} F:{:02x}", Regs.A, Regs.F));
    _register_values.emplace_back(REGISTER_NAMES.at(BC), format("{:04x}", Regs.BC));
    _register_values.emplace_back(REGISTER_NAMES.at(DE), format("{:04x}", Regs.DE));
    _register_values.emplace_back(REGISTER_NAMES.at(HL), format("{:04x}", Regs.HL));
    _register_values.emplace_back(REGISTER_NAMES.at(SP), format("{:04x}", Regs.SP));
    _register_values.emplace_back(REGISTER_NAMES.at(PC), format("{:04x}", Regs.PC));
    _register_values.emplace_back(REGISTER_NAMES.at(IE), format("{:02x}", machine_ctx->IE));
    _register_values.emplace_back(REGISTER_NAMES.at(IF), format("{:02x}", mem_instance->GetIO(IF_ADDR)));

    _flag_values.clear();
    _flag_values.emplace_back(FLAG_NAMES.at(FLAG_C), format("{:01b}", (Regs.F & FLAG_CARRY) >> 4));
    _flag_values.emplace_back(FLAG_NAMES.at(FLAG_H), format("{:01b}", (Regs.F & FLAG_HCARRY) >> 5));
    _flag_values.emplace_back(FLAG_NAMES.at(FLAG_N), format("{:01b}", (Regs.F & FLAG_SUB) >> 6));
    _flag_values.emplace_back(FLAG_NAMES.at(FLAG_Z), format("{:01b}", (Regs.F & FLAG_ZERO) >> 7));
    _flag_values.emplace_back(FLAG_NAMES.at(FLAG_IME), format("{:01b}", ime ? 1 : 0));
    u8 isr_requested = mem_instance->GetIO(IF_ADDR);
    _flag_values.emplace_back(FLAG_NAMES.at(INT_VBLANK), format("{:01b}", (isr_requested & IRQ_VBLANK)));
    _flag_values.emplace_back(FLAG_NAMES.at(INT_STAT), format("{:01b}", (isr_requested & IRQ_LCD_STAT) >> 1));
    _flag_values.emplace_back(FLAG_NAMES.at(INT_TIMER), format("{:01b}", (isr_requested & IRQ_TIMER) >> 2));
    _flag_values.emplace_back(FLAG_NAMES.at(INT_SERIAL), format("{:01b}", (isr_requested & IRQ_SERIAL) >> 3));
    _flag_values.emplace_back(FLAG_NAMES.at(INT_JOYPAD), format("{:01b}", (isr_requested & IRQ_JOYPAD) >> 4));

    _misc_values.clear();
    _misc_values.emplace_back("LCDC", format("{:02x}", mem_instance->GetIO(LCDC_ADDR)));
    _misc_values.emplace_back("STAT", format("{:02x}", mem_instance->GetIO(STAT_ADDR)));
    _misc_values.emplace_back("WRAM", format("{:02x}", mem_instance->GetIO(CGB_WRAM_SELECT_ADDR)));
    _misc_values.emplace_back("VRAM", format("{:02x}", mem_instance->GetIO(CGB_VRAM_SELECT_ADDR)));
    _misc_values.emplace_back("LY", format("{:02x}", mem_instance->GetIO(LY_ADDR)));
    _misc_values.emplace_back("LYC", format("{:02x}", mem_instance->GetIO(LYC_ADDR)));
    _misc_values.emplace_back("SCX", format("{:02x}", mem_instance->GetIO(SCX_ADDR)));
    _misc_values.emplace_back("SCY", format("{:02x}", mem_instance->GetIO(SCY_ADDR)));
    _misc_values.emplace_back("WX", format("{:02x}", mem_instance->GetIO(WX_ADDR)));
    _misc_values.emplace_back("WY", format("{:02x}", mem_instance->GetIO(WY_ADDR)));
}

/* ***********************************************************************************************************
    PREPARE DEBUG DATA (DISASSEMBLED INSTRUCTIONS)
*********************************************************************************************************** */
void GameboyCPU::DecodeBankContent(TableSection<instr_entry>& _program_buffer, vector<u8>* _bank_data, const int& _offset, const int& _bank_num, const string& _bank_name) {
    u16 data = 0;
    bool cb = false;

    _program_buffer.clear();
    auto current_entry = TableEntry<instr_entry>();                // a table entry consists of the address and a pair of two strings (left and right column of debugger)
    u8* bank_data = _bank_data->data();

    for (u16 addr = 0, i = 0; addr < _bank_data->size(); i++) {
        current_entry = TableEntry<instr_entry>();

        // get opcode and set entry address
        u8 opcode = bank_data[addr];
        get<ST_ENTRY_ADDRESS>(current_entry) = _offset + addr;          // set the entry address

        {
            instr_tuple* instr_ptr;

            // check for default or CB instruction and add opcode to result
            if (cb)     { instr_ptr = &(instrMapCB[opcode]); } 
            else        { instr_ptr = &(instrMap[opcode]); }
            cb = (opcode == 0xCB);

            // proccess instruction mnemonic
            string result_string = get<INSTR_MNEMONIC>(*instr_ptr);
            string result_binary = _bank_name + format("{:d}: {:04x}  {:02x}", _bank_num, _offset + addr, opcode);
            addr++;

            // process instruction arguments
            {
                cgb_data_types arg1 = get<INSTR_ARG_1>(*instr_ptr);
                cgb_data_types arg2 = get<INSTR_ARG_2>(*instr_ptr);

                for (int i = 0; i < 2; i++) {
                    cgb_data_types arg = (i == 0 ? arg1 : arg2);

                    switch (arg) {
                    case AF:
                    case A:
                    case F:
                    case BC:
                    case B:
                    case C:
                    case DE:
                    case D:
                    case E:
                    case HL:
                    case H:
                    case L:
                    case SP:
                    case PC:
                        result_string += (i == 0 ? "" : ",");
                        result_string += " " + REGISTER_NAMES.at(arg);
                        break;
                    case d8:
                    case a8:
                    case a8_ref:
                        data = bank_data[addr]; addr++;

                        result_binary += format(" {:02x}", (u8)data);

                        result_string += (i == 0 ? "" : ",");
                        if (arg == a8_ref) {
                            result_string += format(" (FF00+{:02x})", (u8)(data & 0xFF));
                        } else {
                            result_string += format(" {:02x}", (u8)(data & 0xFF));
                        }
                        break;
                    case r8:
                    {
                        data = bank_data[addr];
                        i8 signed_data = *(i8*)&data;

                        result_binary += format(" {:02x}", data);

                        result_string += (i == 0 ? "" : ",");
                        result_string += format(" {:04x}", addr + signed_data);
                        addr++;
                    }
                        break;
                    case d16:
                    case a16:
                    case a16_ref:
                        data = bank_data[addr];                 addr++;
                        data |= ((u16)bank_data[addr]) << 8;    addr++;

                        result_binary += format(" {:02x}", (u8)(data & 0xFF));
                        result_binary += format(" {:02x}", (u8)((data & 0xFF00) >> 8));

                        result_string += (i == 0 ? "" : ",");
                        if (arg == a16_ref) {
                            result_string = format(" ({:04x})", data);
                        } else {
                            result_string = format(" {:04x}", data);
                        }
                        break;
                    case SP_r8:
                    {
                        data = bank_data[addr]; addr++;
                        i8 signed_data = *(i8*)&data;

                        result_binary += format(" {:02x}", (u8)(data & 0xFF));

                        result_string += (i == 0 ? "" : ",");
                        result_string = format("SP+{:02x}", signed_data);
                    }
                        break;
                    case BC_ref:
                    case DE_ref:
                    case HL_ref:
                    case HL_INC_ref:
                    case HL_DEC_ref:
                    case C_ref:
                        result_string = DATA_NAMES.at(arg);
                        break;
                    default:
                        break;
                    }
                }

                get<ST_ENTRY_DATA>(current_entry).first = result_binary;
                get<ST_ENTRY_DATA>(current_entry).second = result_string;
            }
        }

        _program_buffer.emplace_back(current_entry);
    }
}

void GameboyCPU::SetupInstrDebugTables(Table<instr_entry>& _table) {
    _table = Table<instr_entry>(DEBUG_INSTR_LINES);

    int i = 0;

    TableSection<instr_entry> current_table;
    int offset = 0;

    while (vector<u8>* rom_data = mem_instance->GetProgramData(i)) {
        current_table = TableSection<instr_entry>();

        if (i == 0)     { offset = ROM_0_OFFSET; } 
        else            { offset = ROM_N_OFFSET; }

        DecodeBankContent(current_table, rom_data, offset, i, "ROM");
        _table.AddTableSectionDisposable(current_table);

        i++;
    }
}

void GameboyCPU::SetupInstrDebugTablesTmp(Table<instr_entry>& _table) {
    auto bank_table = TableSection<instr_entry>();
    int bank_num;

    _table = Table<instr_entry>(DEBUG_INSTR_LINES);

    if (Regs.PC >= VRAM_N_OFFSET && Regs.PC < RAM_N_OFFSET) {
        bank_num = mem_instance->GetIO(CGB_VRAM_SELECT_ADDR);
        DecodeBankContent(bank_table, &graphics_ctx->VRAM_N[bank_num], VRAM_N_OFFSET, bank_num, "VRAM");
    } else if (Regs.PC >= RAM_N_OFFSET && Regs.PC < WRAM_0_OFFSET) {
        bank_num = machine_ctx->ram_bank_selected;
        DecodeBankContent(bank_table, &mem_instance->RAM_N[bank_num], RAM_N_OFFSET, bank_num, "RAM");
    } else if (Regs.PC >= WRAM_0_OFFSET && Regs.PC < WRAM_N_OFFSET) {
        bank_num = 0;
        DecodeBankContent(bank_table, &mem_instance->WRAM_0, WRAM_0_OFFSET, bank_num, "WRAM");
    } else if (Regs.PC >= WRAM_N_OFFSET && Regs.PC < MIRROR_WRAM_OFFSET) {
        bank_num = machine_ctx->wram_bank_selected;
        DecodeBankContent(bank_table, &mem_instance->WRAM_N[bank_num], WRAM_N_OFFSET, bank_num + 1, "WRAM");
    } else if (Regs.PC >= HRAM_OFFSET && Regs.PC < IE_OFFSET) {
        bank_num = 0;
        DecodeBankContent(bank_table, &mem_instance->HRAM, HRAM_OFFSET, bank_num, "HRAM");
    } else {
        // TODO
    }

    _table.AddTableSectionDisposable(bank_table);
}