#include "GameboyCPU.h"

/* ***********************************************************************************************************
    INCLUDES
*********************************************************************************************************** */
#include "gameboy_defines.h"
#include <format>
#include "logger.h"
#include "GameboyMEM.h"
#include <unordered_map>

#include <iostream>

using namespace std;

namespace Emulation {
    namespace Gameboy {
        /* ***********************************************************************************************************
            MISC LOOKUPS
        *********************************************************************************************************** */
        enum instr_lookup_enums {
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
            {IEreg, "IE"},
            {IFreg, "IF"}
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
            CONSTRUCTOR
        *********************************************************************************************************** */
        GameboyCPU::GameboyCPU(std::shared_ptr<BaseCartridge> _cartridge) : BaseCPU(_cartridge) {
            setupLookupTable();
            setupLookupTableCB();

            GenerateAssemblyTables(_cartridge);
        }

        /* ***********************************************************************************************************
            INIT CPU
        *********************************************************************************************************** */
        void GameboyCPU::Init() {
            m_MmuInstance = BaseMMU::s_GetInstance();
            m_GraphicsInstance = BaseGPU::s_GetInstance();
            m_SoundInstance = BaseAPU::s_GetInstance();
            m_MemInstance = std::dynamic_pointer_cast<GameboyMEM>(BaseMEM::s_GetInstance());

            machineCtx = m_MemInstance.lock()->GetMachineContext();
            graphics_ctx = m_MemInstance.lock()->GetGraphicsContext();
            sound_ctx = m_MemInstance.lock()->GetSoundContext();

            if (!machineCtx->boot_rom_mapped) {
                InitRegisterStates();
            }

            m_GraphicsInstance = BaseGPU::s_GetInstance();
            m_SoundInstance = BaseAPU::s_GetInstance();
            ticksPerFrame = m_GraphicsInstance.lock()->GetTicksPerFrame((float)(BASE_CLOCK_CPU));
        }

        // initial register states
        void GameboyCPU::InitRegisterStates() {
            Regs = registers();

            Regs.SP = INIT_SP;
            Regs.PC = INIT_PC;

            if (machineCtx->is_cgb) {
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

            while ((currentTicks < (ticksPerFrame * machineCtx->currentSpeed))) {
                RunCpu();
            }

            tickCounter += currentTicks;
        }

        void GameboyCPU::RunCycle() {
            currentTicks = 0;

            do {
                RunCpu();
                if ((currentTicks > (ticksPerFrame * machineCtx->currentSpeed))) { break; }
            } while (machineCtx->halted);

            tickCounter += currentTicks;
        }

        void GameboyCPU::RunCpu() {
            if (machineCtx->stopped) {
                // check button press
                if (m_MemInstance.lock()->GetIO(IF_ADDR) & IRQ_JOYPAD) {
                    machineCtx->stopped = false;
                } else {
                    return;
                }

            } else if (machineCtx->halted) {
                TickTimers();

                // check pending and enabled interrupts
                if (machineCtx->IE & m_MemInstance.lock()->GetIO(IF_ADDR)) {
                    machineCtx->halted = false;
                }
            } else {
                if (!CheckInterrupts()) {
                    ExecuteInstruction();
                }
            }
        }

        void GameboyCPU::ExecuteInstruction() {
            bool ime_enable = imeEnable;

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

            if (ime_enable) {
                ime = true;
                imeEnable = false;
            }
        }

        bool GameboyCPU::CheckInterrupts() {
            if (ime) {
                u8& isr_requested = m_MemInstance.lock()->GetIO(IF_ADDR);
                if ((isr_requested & IRQ_VBLANK) && (machineCtx->IE & IRQ_VBLANK)) {
                    ime = false;

                    isr_push(ISR_VBLANK_HANDLER_ADDR);
                    isr_requested &= ~IRQ_VBLANK;
                    return true;
                }
                else if ((isr_requested & IRQ_LCD_STAT) && (machineCtx->IE & IRQ_LCD_STAT)) {
                    ime = false;

                    isr_push(ISR_LCD_STAT_HANDLER_ADDR);
                    isr_requested &= ~IRQ_LCD_STAT;
                    return true;
                }
                else if ((isr_requested & IRQ_TIMER) && (machineCtx->IE & IRQ_TIMER)) {
                    ime = false;

                    isr_push(ISR_TIMER_HANDLER_ADDR);
                    isr_requested &= ~IRQ_TIMER;
                    return true;
                }
                if ((isr_requested & IRQ_SERIAL) && (machineCtx->IE & IRQ_SERIAL)) {
                    ime = false;

                    isr_push(ISR_SERIAL_HANDLER_ADDR);
                    isr_requested &= ~IRQ_SERIAL;
                    return true;
                }
                else if ((isr_requested & IRQ_JOYPAD) && (machineCtx->IE & IRQ_JOYPAD)) {
                    ime = false;

                    isr_push(ISR_JOYPAD_HANDLER_ADDR);
                    isr_requested &= ~IRQ_JOYPAD;
                    return true;
                }
            }
            return false;
        }

        // ticks the timers for 4 clock cycles / 1 machine cycle and everything thats related to it
        void GameboyCPU::TickTimers() {
            bool div_low_byte_selected = machineCtx->timaDivMask < 0x100;
            u8& div = m_MemInstance.lock()->GetIO(DIV_ADDR);
            bool tima_enabled = m_MemInstance.lock()->GetIO(TAC_ADDR) & TAC_CLOCK_ENABLE;

            if (machineCtx->tima_reload_cycle) {
                div = m_MemInstance.lock()->GetIO(TMA_ADDR);
                if (!machineCtx->tima_reload_if_write) {
                    m_MemInstance.lock()->RequestInterrupts(IRQ_TIMER);
                } else {
                    machineCtx->tima_reload_if_write = false;
                }
                machineCtx->tima_reload_cycle = false;
            } else if (machineCtx->tima_overflow_cycle) {
                machineCtx->tima_reload_cycle = true;
                machineCtx->tima_overflow_cycle = false;
            }

            for (int i = 0; i < TICKS_PER_MC; i++) {
                if (machineCtx->div_low_byte == 0xFF) {
                    machineCtx->div_low_byte = 0x00;

                    if (div == 0xFF) {
                        div = 0x00;
                    } else {
                        div++;
                    }

                    apuDivBitOverflowCur = div & machineCtx->apuDivMask ? true : false;
                    if (!apuDivBitOverflowCur && apuDivBitOverflowPrev) {
                        m_SoundInstance.lock()->ProcessAPU(1);
                    }
                    apuDivBitOverflowPrev = apuDivBitOverflowCur;
                } else {
                    machineCtx->div_low_byte++;
                }

                if (div_low_byte_selected) {
                    timaEnAndDivOverflowCur = tima_enabled && (machineCtx->div_low_byte & machineCtx->timaDivMask ? true : false);
                } else {
                    timaEnAndDivOverflowCur = tima_enabled && (div & (machineCtx->timaDivMask >> 8) ? true : false);
                }

                if (!timaEnAndDivOverflowCur && timaEnAndDivOverflowPrev) {
                    IncrementTIMA();
                }
                timaEnAndDivOverflowPrev = timaEnAndDivOverflowCur;
            }

            currentTicks += TICKS_PER_MC;

            m_GraphicsInstance.lock()->ProcessGPU(TICKS_PER_MC);
            m_SoundInstance.lock()->GenerateSamples(TICKS_PER_MC >> (machineCtx->currentSpeed - 1));
        }

        void GameboyCPU::IncrementTIMA() {
            u8& tima = m_MemInstance.lock()->GetIO(TIMA_ADDR);
            if (tima == 0xFF) {
                tima = 0x00;
                machineCtx->tima_overflow_cycle = true;
            }
            else {
                tima++;
            }
        }

        void GameboyCPU::FetchOpCode() {
            opcode = m_MmuInstance.lock()->Read8Bit(Regs.PC);
            Regs.PC++;
            TickTimers();
        }

        void GameboyCPU::Fetch8Bit() {
            data = m_MmuInstance.lock()->Read8Bit(Regs.PC);
            Regs.PC++;
            TickTimers();
        }

        void GameboyCPU::Fetch16Bit() {
            data = m_MmuInstance.lock()->Read8Bit(Regs.PC);
            Regs.PC++;
            TickTimers();

            data |= (((u16)m_MmuInstance.lock()->Read8Bit(Regs.PC)) << 8);
            Regs.PC++;
            TickTimers();
        }

        void GameboyCPU::Write8Bit(const u8& _data, const u16& _addr) {
            m_MmuInstance.lock()->Write8Bit(_data, _addr);
            TickTimers();
        }

        void GameboyCPU::Write16Bit(const u16& _data, const u16& _addr) {
            m_MmuInstance.lock()->Write8Bit((_data >> 8) & 0xFF, _addr + 1);
            TickTimers();
            m_MmuInstance.lock()->Write8Bit(_data & 0xFF, _addr);
            TickTimers();
        }

        u8 GameboyCPU::Read8Bit(const u16& _addr) {
            u8 data = m_MmuInstance.lock()->Read8Bit(_addr);
            TickTimers();
            return data;
        }

        u16 GameboyCPU::Read16Bit(const u16& _addr) {
            u16 data = m_MmuInstance.lock()->Read8Bit(_addr);
            TickTimers();
            data |= (((u16)m_MmuInstance.lock()->Read8Bit(_addr + 1)) << 8);
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
            instrMap.emplace_back(&GameboyCPU::NOP, "NOP", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::LDd16, "LD", BC, d16);
            instrMap.emplace_back(&GameboyCPU::LDfromAtoRef, "LD", BC_ref, A);
            instrMap.emplace_back(&GameboyCPU::INC16, "INC", BC, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::INC8, "INC", B, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::DEC8, "DEC", B, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::LDd8, "LD", B, d8);
            instrMap.emplace_back(&GameboyCPU::RLCA, "RLCA", A, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::LDSPa16, "LD", a16_ref, SP);
            instrMap.emplace_back(&GameboyCPU::ADDHL, "ADD", HL, BC);
            instrMap.emplace_back(&GameboyCPU::LDtoAfromRef, "LD", A, BC_ref);
            instrMap.emplace_back(&GameboyCPU::DEC16, "DEC", BC, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::INC8, "INC", C, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::DEC8, "DEC", C, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::LDd8, "LD", C, d8);
            instrMap.emplace_back(&GameboyCPU::RRCA, "RRCA", A, NO_DATA_TYPE);

            // 0x10
            instrMap.emplace_back(&GameboyCPU::STOP, "STOP", d8, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::LDd16, "LD", DE, d16);
            instrMap.emplace_back(&GameboyCPU::LDfromAtoRef, "LD", DE_ref, A);
            instrMap.emplace_back(&GameboyCPU::INC16, "INC", DE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::INC8, "INC", D, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::DEC8, "DEC", D, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::LDd8, "LD", D, d8);
            instrMap.emplace_back(&GameboyCPU::RLA, "RLA", A, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::JR, "JR", r8, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::ADDHL, "ADD", HL, DE);
            instrMap.emplace_back(&GameboyCPU::LDtoAfromRef, "LD", A, DE_ref);
            instrMap.emplace_back(&GameboyCPU::DEC16, "DEC", DE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::INC8, "INC", E, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::DEC8, "DEC", E, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::LDd8, "LD", E, d8);
            instrMap.emplace_back(&GameboyCPU::RRA, "RRA", A, NO_DATA_TYPE);

            // 0x20
            instrMap.emplace_back(&GameboyCPU::JR, "JR NZ", r8, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::LDd16, "LD", HL, d16);
            instrMap.emplace_back(&GameboyCPU::LDfromAtoRef, "LD", HL_INC_ref, A);
            instrMap.emplace_back(&GameboyCPU::INC16, "INC", HL, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::INC8, "INC", H, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::DEC8, "DEC", H, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::LDd8, "LD", H, d8);
            instrMap.emplace_back(&GameboyCPU::DAA, "DAA", A, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::JR, "JR Z", r8, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::ADDHL, "ADD", HL, HL);
            instrMap.emplace_back(&GameboyCPU::LDtoAfromRef, "LD", A, HL_INC_ref);
            instrMap.emplace_back(&GameboyCPU::DEC16, "DEC", HL, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::INC8, "INC", L, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::DEC8, "DEC", L, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::LDd8, "LD", L, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::CPL, "CPL", A, NO_DATA_TYPE);

            // 0x30
            instrMap.emplace_back(&GameboyCPU::JR, "JR NC", r8, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::LDd16, "LD", SP, d16);
            instrMap.emplace_back(&GameboyCPU::LDfromAtoRef, "LD", HL_DEC_ref, A);
            instrMap.emplace_back(&GameboyCPU::INC16, "INC", SP, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::INC8, "INC", HL_ref, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::DEC8, "DEC", HL_ref, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::LDd8, "LD", HL_ref, d8);
            instrMap.emplace_back(&GameboyCPU::SCF, "SCF", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::JR, "JR C", r8, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::ADDHL, "ADD", HL, SP);
            instrMap.emplace_back(&GameboyCPU::LDtoAfromRef, "LD", A, HL_DEC_ref);
            instrMap.emplace_back(&GameboyCPU::DEC16, "DEC", SP, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::INC8, "INC", A, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::DEC8, "DEC", A, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::LDd8, "LD", A, d8);
            instrMap.emplace_back(&GameboyCPU::CCF, "CCF", NO_DATA_TYPE, NO_DATA_TYPE);

            // 0x40
            instrMap.emplace_back(&GameboyCPU::LDtoB, "LD", B, B);
            instrMap.emplace_back(&GameboyCPU::LDtoB, "LD", B, C);
            instrMap.emplace_back(&GameboyCPU::LDtoB, "LD", B, D);
            instrMap.emplace_back(&GameboyCPU::LDtoB, "LD", B, E);
            instrMap.emplace_back(&GameboyCPU::LDtoB, "LD", B, H);
            instrMap.emplace_back(&GameboyCPU::LDtoB, "LD", B, L);
            instrMap.emplace_back(&GameboyCPU::LDtoB, "LD", B, HL_ref);
            instrMap.emplace_back(&GameboyCPU::LDtoB, "LD", B, A);
            instrMap.emplace_back(&GameboyCPU::LDtoC, "LD", C, B);
            instrMap.emplace_back(&GameboyCPU::LDtoC, "LD", C, C);
            instrMap.emplace_back(&GameboyCPU::LDtoC, "LD", C, D);
            instrMap.emplace_back(&GameboyCPU::LDtoC, "LD", C, E);
            instrMap.emplace_back(&GameboyCPU::LDtoC, "LD", C, H);
            instrMap.emplace_back(&GameboyCPU::LDtoC, "LD", C, L);
            instrMap.emplace_back(&GameboyCPU::LDtoC, "LD", C, HL_ref);
            instrMap.emplace_back(&GameboyCPU::LDtoC, "LD", C, A);

            // 0x50
            instrMap.emplace_back(&GameboyCPU::LDtoD, "LD", D, B);
            instrMap.emplace_back(&GameboyCPU::LDtoD, "LD", D, C);
            instrMap.emplace_back(&GameboyCPU::LDtoD, "LD", D, D);
            instrMap.emplace_back(&GameboyCPU::LDtoD, "LD", D, E);
            instrMap.emplace_back(&GameboyCPU::LDtoD, "LD", D, H);
            instrMap.emplace_back(&GameboyCPU::LDtoD, "LD", D, L);
            instrMap.emplace_back(&GameboyCPU::LDtoD, "LD", D, HL_ref);
            instrMap.emplace_back(&GameboyCPU::LDtoD, "LD", D, A);
            instrMap.emplace_back(&GameboyCPU::LDtoE, "LD", E, B);
            instrMap.emplace_back(&GameboyCPU::LDtoE, "LD", E, C);
            instrMap.emplace_back(&GameboyCPU::LDtoE, "LD", E, D);
            instrMap.emplace_back(&GameboyCPU::LDtoE, "LD", E, E);
            instrMap.emplace_back(&GameboyCPU::LDtoE, "LD", E, H);
            instrMap.emplace_back(&GameboyCPU::LDtoE, "LD", E, L);
            instrMap.emplace_back(&GameboyCPU::LDtoE, "LD", E, HL_ref);
            instrMap.emplace_back(&GameboyCPU::LDtoE, "LD", E, A);

            // 0x60
            instrMap.emplace_back(&GameboyCPU::LDtoH, "LD", H, B);
            instrMap.emplace_back(&GameboyCPU::LDtoH, "LD", H, C);
            instrMap.emplace_back(&GameboyCPU::LDtoH, "LD", H, D);
            instrMap.emplace_back(&GameboyCPU::LDtoH, "LD", H, E);
            instrMap.emplace_back(&GameboyCPU::LDtoH, "LD", H, H);
            instrMap.emplace_back(&GameboyCPU::LDtoH, "LD", H, L);
            instrMap.emplace_back(&GameboyCPU::LDtoH, "LD", H, HL_ref);
            instrMap.emplace_back(&GameboyCPU::LDtoH, "LD", H, A);
            instrMap.emplace_back(&GameboyCPU::LDtoL, "LD", L, B);
            instrMap.emplace_back(&GameboyCPU::LDtoL, "LD", L, C);
            instrMap.emplace_back(&GameboyCPU::LDtoL, "LD", L, D);
            instrMap.emplace_back(&GameboyCPU::LDtoL, "LD", L, E);
            instrMap.emplace_back(&GameboyCPU::LDtoL, "LD", L, H);
            instrMap.emplace_back(&GameboyCPU::LDtoL, "LD", L, L);
            instrMap.emplace_back(&GameboyCPU::LDtoL, "LD", L, HL_ref);
            instrMap.emplace_back(&GameboyCPU::LDtoL, "LD", L, A);

            // 0x70
            instrMap.emplace_back(&GameboyCPU::LDtoHLref, "LD", HL_ref, B);
            instrMap.emplace_back(&GameboyCPU::LDtoHLref, "LD", HL_ref, C);
            instrMap.emplace_back(&GameboyCPU::LDtoHLref, "LD", HL_ref, D);
            instrMap.emplace_back(&GameboyCPU::LDtoHLref, "LD", HL_ref, E);
            instrMap.emplace_back(&GameboyCPU::LDtoHLref, "LD", HL_ref, H);
            instrMap.emplace_back(&GameboyCPU::LDtoHLref, "LD", HL_ref, L);
            instrMap.emplace_back(&GameboyCPU::HALT, "HALT", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::LDtoHLref, "LD", HL_ref, A);
            instrMap.emplace_back(&GameboyCPU::LDtoA, "LD", A, B);
            instrMap.emplace_back(&GameboyCPU::LDtoA, "LD", A, C);
            instrMap.emplace_back(&GameboyCPU::LDtoA, "LD", A, D);
            instrMap.emplace_back(&GameboyCPU::LDtoA, "LD", A, E);
            instrMap.emplace_back(&GameboyCPU::LDtoA, "LD", A, H);
            instrMap.emplace_back(&GameboyCPU::LDtoA, "LD", A, L);
            instrMap.emplace_back(&GameboyCPU::LDtoA, "LD", A, HL_ref);
            instrMap.emplace_back(&GameboyCPU::LDtoA, "LD", A, A);

            // 0x80
            instrMap.emplace_back(&GameboyCPU::ADD8, "ADD", A, B);
            instrMap.emplace_back(&GameboyCPU::ADD8, "ADD", A, C);
            instrMap.emplace_back(&GameboyCPU::ADD8, "ADD", A, D);
            instrMap.emplace_back(&GameboyCPU::ADD8, "ADD", A, E);
            instrMap.emplace_back(&GameboyCPU::ADD8, "ADD", A, H);
            instrMap.emplace_back(&GameboyCPU::ADD8, "ADD", A, L);
            instrMap.emplace_back(&GameboyCPU::ADD8, "ADD", A, HL_ref);
            instrMap.emplace_back(&GameboyCPU::ADD8, "ADD", A, A);
            instrMap.emplace_back(&GameboyCPU::ADC, "ADC", A, B);
            instrMap.emplace_back(&GameboyCPU::ADC, "ADC", A, C);
            instrMap.emplace_back(&GameboyCPU::ADC, "ADC", A, D);
            instrMap.emplace_back(&GameboyCPU::ADC, "ADC", A, E);
            instrMap.emplace_back(&GameboyCPU::ADC, "ADC", A, H);
            instrMap.emplace_back(&GameboyCPU::ADC, "ADC", A, L);
            instrMap.emplace_back(&GameboyCPU::ADC, "ADC", A, HL_ref);
            instrMap.emplace_back(&GameboyCPU::ADC, "ADC", A, A);

            // 0x90
            instrMap.emplace_back(&GameboyCPU::SUB, "SUB", A, B);
            instrMap.emplace_back(&GameboyCPU::SUB, "SUB", A, C);
            instrMap.emplace_back(&GameboyCPU::SUB, "SUB", A, D);
            instrMap.emplace_back(&GameboyCPU::SUB, "SUB", A, E);
            instrMap.emplace_back(&GameboyCPU::SUB, "SUB", A, H);
            instrMap.emplace_back(&GameboyCPU::SUB, "SUB", A, L);
            instrMap.emplace_back(&GameboyCPU::SUB, "SUB", A, HL_ref);
            instrMap.emplace_back(&GameboyCPU::SUB, "SUB", A, A);
            instrMap.emplace_back(&GameboyCPU::SBC, "SBC", A, B);
            instrMap.emplace_back(&GameboyCPU::SBC, "SBC", A, C);
            instrMap.emplace_back(&GameboyCPU::SBC, "SBC", A, D);
            instrMap.emplace_back(&GameboyCPU::SBC, "SBC", A, E);
            instrMap.emplace_back(&GameboyCPU::SBC, "SBC", A, H);
            instrMap.emplace_back(&GameboyCPU::SBC, "SBC", A, L);
            instrMap.emplace_back(&GameboyCPU::SBC, "SBC", A, HL_ref);
            instrMap.emplace_back(&GameboyCPU::SBC, "SBC", A, A);

            // 0xa0
            instrMap.emplace_back(&GameboyCPU::AND, "AND", A, B);
            instrMap.emplace_back(&GameboyCPU::AND, "AND", A, C);
            instrMap.emplace_back(&GameboyCPU::AND, "AND", A, D);
            instrMap.emplace_back(&GameboyCPU::AND, "AND", A, E);
            instrMap.emplace_back(&GameboyCPU::AND, "AND", A, H);
            instrMap.emplace_back(&GameboyCPU::AND, "AND", A, L);
            instrMap.emplace_back(&GameboyCPU::AND, "AND", A, HL_ref);
            instrMap.emplace_back(&GameboyCPU::AND, "AND", A, A);
            instrMap.emplace_back(&GameboyCPU::XOR, "XOR", A, B);
            instrMap.emplace_back(&GameboyCPU::XOR, "XOR", A, C);
            instrMap.emplace_back(&GameboyCPU::XOR, "XOR", A, D);
            instrMap.emplace_back(&GameboyCPU::XOR, "XOR", A, E);
            instrMap.emplace_back(&GameboyCPU::XOR, "XOR", A, H);
            instrMap.emplace_back(&GameboyCPU::XOR, "XOR", A, L);
            instrMap.emplace_back(&GameboyCPU::XOR, "XOR", A, HL_ref);
            instrMap.emplace_back(&GameboyCPU::XOR, "XOR", A, A);

            // 0xb0
            instrMap.emplace_back(&GameboyCPU::OR, "OR", A, B);
            instrMap.emplace_back(&GameboyCPU::OR, "OR", A, C);
            instrMap.emplace_back(&GameboyCPU::OR, "OR", A, D);
            instrMap.emplace_back(&GameboyCPU::OR, "OR", A, E);
            instrMap.emplace_back(&GameboyCPU::OR, "OR", A, H);
            instrMap.emplace_back(&GameboyCPU::OR, "OR", A, L);
            instrMap.emplace_back(&GameboyCPU::OR, "OR", A, HL_ref);
            instrMap.emplace_back(&GameboyCPU::OR, "OR", A, A);
            instrMap.emplace_back(&GameboyCPU::CP, "CP", A, B);
            instrMap.emplace_back(&GameboyCPU::CP, "CP", A, C);
            instrMap.emplace_back(&GameboyCPU::CP, "CP", A, D);
            instrMap.emplace_back(&GameboyCPU::CP, "CP", A, E);
            instrMap.emplace_back(&GameboyCPU::CP, "CP", A, H);
            instrMap.emplace_back(&GameboyCPU::CP, "CP", A, L);
            instrMap.emplace_back(&GameboyCPU::CP, "CP", A, HL_ref);
            instrMap.emplace_back(&GameboyCPU::CP, "CP", A, A);

            // 0xc0
            instrMap.emplace_back(&GameboyCPU::RET, "RET NZ", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::POP, "POP", BC, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::JP, "JP NZ", a16, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::JP, "JP", a16, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::CALL, "CALL NZ", a16, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::PUSH, "PUSH", BC, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::ADD8, "ADD", A, d8);
            instrMap.emplace_back(&GameboyCPU::RST, "RST $00", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::RET, "RET Z", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::RET, "RET", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::JP, "JP Z", a16, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::CB, "CB", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::CALL, "CALL Z", a16, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::CALL, "CALL", a16, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::ADC, "ADC", A, d8);
            instrMap.emplace_back(&GameboyCPU::RST, "RST $08", NO_DATA_TYPE, NO_DATA_TYPE);

            // 0xd0
            instrMap.emplace_back(&GameboyCPU::RET, "RET NC", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::POP, "POP", DE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::JP, "JP NC", a16, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::NoInstruction, "NoInstruction", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::CALL, "CALL NC", a16, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::PUSH, "PUSH", DE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::SUB, "SUB", A, d8);
            instrMap.emplace_back(&GameboyCPU::RST, "RST $10", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::RET, "RET C", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::RETI, "RETI", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::JP, "JP C", a16, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::NoInstruction, "NoInstruction", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::CALL, "CALL C", a16, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::NoInstruction, "NoInstruction", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::SBC, "SBC", A, d8);
            instrMap.emplace_back(&GameboyCPU::RST, "RST $18", NO_DATA_TYPE, NO_DATA_TYPE);

            // 0xe0
            instrMap.emplace_back(&GameboyCPU::LDH, "LDH", a8_ref, A);
            instrMap.emplace_back(&GameboyCPU::POP, "POP", HL, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::LDCref, "LD", C_ref, A);
            instrMap.emplace_back(&GameboyCPU::NoInstruction, "NoInstruction", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::NoInstruction, "NoInstruction", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::PUSH, "PUSH", HL, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::AND, "AND", A, d8);
            instrMap.emplace_back(&GameboyCPU::RST, "RST $20", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::ADDSPr8, "ADD", SP, r8);
            instrMap.emplace_back(&GameboyCPU::JP, "JP", HL_ref, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::LDHa16, "LD", a16_ref, A);
            instrMap.emplace_back(&GameboyCPU::NoInstruction, "NoInstruction", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::NoInstruction, "NoInstruction", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::NoInstruction, "NoInstruction", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::XOR, "XOR", A, d8);
            instrMap.emplace_back(&GameboyCPU::RST, "RST $28", NO_DATA_TYPE, NO_DATA_TYPE);

            // 0xf0
            instrMap.emplace_back(&GameboyCPU::LDH, "LD", A, a8_ref);
            instrMap.emplace_back(&GameboyCPU::POP, "POP", AF, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::LDCref, "LD", A, C_ref);
            instrMap.emplace_back(&GameboyCPU::DI, "DI", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::NoInstruction, "NoInstruction", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::PUSH, "PUSH", AF, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::OR, "OR", A, d8);
            instrMap.emplace_back(&GameboyCPU::RST, "RST $30", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::LDHLSPr8, "LD", HL, SP_r8);
            instrMap.emplace_back(&GameboyCPU::LDSPHL, "LD", SP, HL);
            instrMap.emplace_back(&GameboyCPU::LDHa16, "LD", A, a16_ref);
            instrMap.emplace_back(&GameboyCPU::EI, "EI", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::NoInstruction, "NoInstruction", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::NoInstruction, "NoInstruction", NO_DATA_TYPE, NO_DATA_TYPE);
            instrMap.emplace_back(&GameboyCPU::CP, "CP", A, d8);
            instrMap.emplace_back(&GameboyCPU::RST, "RST $38", NO_DATA_TYPE, NO_DATA_TYPE);
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
            u8 isr_requested = m_MemInstance.lock()->GetIO(IF_ADDR);

            bool joyp = (isr_requested & IRQ_JOYPAD);
            bool two_byte = false;
            bool div_reset = false;

            if (joyp) {
                if (machineCtx->IE & isr_requested) {
                    return;
                }
                else {
                    two_byte = true;
                    machineCtx->halted = true;
                }
            }
            else {
                if (machineCtx->speed_switch_requested) {
                    if (machineCtx->IE & isr_requested) {
                        if (ime) {
                            LOG_ERROR("[emu] STOP Glitch encountered");
                            machineCtx->halted = true;
                            // set IE to 0x00 (this case is undefined behaviour, simply prevent cpu from execution)
                            machineCtx->IE = 0x00;
                        }
                        else {
                            div_reset = true;
                        }
                    }
                    else {
                        // skipping machineCtx->halted, not necessary
                        two_byte = true;
                        div_reset = true;
                    }
                }
                else {
                    two_byte = machineCtx->IE & isr_requested ? false : true;
                    machineCtx->stopped = true;
                    div_reset = true;
                }
            }


            if (two_byte) {
                data = m_MmuInstance.lock()->Read8Bit(Regs.PC);
                Regs.PC++;

                if (data) {
                    LOG_ERROR("[emu] Wrong usage of STOP instruction at: ", format("{:x}", Regs.PC - 1));
                    return;
                }
            }

            if (div_reset) {
                m_MmuInstance.lock()->Write8Bit(0x00, DIV_ADDR);
            }

            if (machineCtx->speed_switch_requested) {
                machineCtx->currentSpeed ^= 3;
                machineCtx->speed_switch_requested = false;

                // selected bit ticks at 1024 Hz
                machineCtx->apuDivMask = (machineCtx->currentSpeed == 2 ? APU_DIV_BIT_DOUBLESPEED : APU_DIV_BIT_SINGLESPEED);
            }
        }

        // machineCtx->halted
        void GameboyCPU::HALT() {
            machineCtx->halted = true;
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
            // gets enabled with with 1 cycle reverbDelay -> execute one more instruction
            imeEnable = true;
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

        // push to Stack
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

            //AddToCallstack(_isr_handler);

            stack_push(Regs.PC);
            Regs.PC = _isr_handler;
        }

        // pop from Stack
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
            //AddToCallstack(data);

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

            //RemoveFromCallstack();
        }

        /* ***********************************************************************************************************
        *
        *   GAMEBOY (COLOR) GameboyCPU CB INSTRUCTION LOOKUP TABLE
        *
        *********************************************************************************************************** */
        void GameboyCPU::setupLookupTableCB() {
            instrMapCB.clear();

            // Elements: opcode, instruction function, machine cycles
            instrMapCB.emplace_back(&GameboyCPU::RLC, "RLC", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RLC, "RLC", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RLC, "RLC", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RLC, "RLC", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RLC, "RLC", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RLC, "RLC", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RLC, "RLC", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RLC, "RLC", A, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RRC, "RRC", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RRC, "RRC", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RRC, "RRC", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RRC, "RRC", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RRC, "RRC", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RRC, "RRC", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RRC, "RRC", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RRC, "RRC", A, NO_DATA_TYPE);

            instrMapCB.emplace_back(&GameboyCPU::RL, "RL", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RL, "RL", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RL, "RL", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RL, "RL", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RL, "RL", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RL, "RL", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RL, "RL", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RL, "RL", A, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RR, "RR", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RR, "RR", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RR, "RR", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RR, "RR", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RR, "RR", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RR, "RR", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RR, "RR", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RR, "RR", A, NO_DATA_TYPE);

            instrMapCB.emplace_back(&GameboyCPU::SLA, "SLA", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SLA, "SLA", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SLA, "SLA", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SLA, "SLA", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SLA, "SLA", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SLA, "SLA", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SLA, "SLA", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SLA, "SLA", A, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SRA, "SRA", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SRA, "SRA", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SRA, "SRA", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SRA, "SRA", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SRA, "SRA", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SRA, "SRA", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SRA, "SRA", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SRA, "SRA", A, NO_DATA_TYPE);

            instrMapCB.emplace_back(&GameboyCPU::SWAP, "SWAP", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SWAP, "SWAP", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SWAP, "SWAP", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SWAP, "SWAP", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SWAP, "SWAP", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SWAP, "SWAP", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SWAP, "SWAP", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SWAP, "SWAP", A, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SRL, "SRL", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SRL, "SRL", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SRL, "SRL", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SRL, "SRL", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SRL, "SRL", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SRL, "SRL", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SRL, "SRL", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SRL, "SRL", A, NO_DATA_TYPE);

            instrMapCB.emplace_back(&GameboyCPU::BIT0, "BIT0", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT0, "BIT0", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT0, "BIT0", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT0, "BIT0", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT0, "BIT0", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT0, "BIT0", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT0, "BIT0", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT0, "BIT0", A, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT1, "BIT1", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT1, "BIT1", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT1, "BIT1", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT1, "BIT1", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT1, "BIT1", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT1, "BIT1", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT1, "BIT1", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT1, "BIT1", A, NO_DATA_TYPE);

            instrMapCB.emplace_back(&GameboyCPU::BIT2, "BIT2", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT2, "BIT2", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT2, "BIT2", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT2, "BIT2", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT2, "BIT2", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT2, "BIT2", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT2, "BIT2", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT2, "BIT2", A, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT3, "BIT3", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT3, "BIT3", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT3, "BIT3", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT3, "BIT3", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT3, "BIT3", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT3, "BIT3", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT3, "BIT3", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT3, "BIT3", A, NO_DATA_TYPE);

            instrMapCB.emplace_back(&GameboyCPU::BIT4, "BIT4", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT4, "BIT4", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT4, "BIT4", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT4, "BIT4", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT4, "BIT4", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT4, "BIT4", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT4, "BIT4", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT4, "BIT4", A, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT5, "BIT5", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT5, "BIT5", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT5, "BIT5", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT5, "BIT5", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT5, "BIT5", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT5, "BIT5", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT5, "BIT5", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT5, "BIT5", A, NO_DATA_TYPE);

            instrMapCB.emplace_back(&GameboyCPU::BIT6, "BIT6", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT6, "BIT6", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT6, "BIT6", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT6, "BIT6", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT6, "BIT6", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT6, "BIT6", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT6, "BIT6", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT6, "BIT6", A, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT7, "BIT7", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT7, "BIT7", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT7, "BIT7", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT7, "BIT7", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT7, "BIT7", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT7, "BIT7", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT7, "BIT7", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::BIT7, "BIT7", A, NO_DATA_TYPE);

            instrMapCB.emplace_back(&GameboyCPU::RES0, "RES0", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES0, "RES0", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES0, "RES0", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES0, "RES0", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES0, "RES0", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES0, "RES0", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES0, "RES0", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES0, "RES0", A, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES1, "RES1", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES1, "RES1", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES1, "RES1", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES1, "RES1", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES1, "RES1", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES1, "RES1", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES1, "RES1", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES1, "RES1", A, NO_DATA_TYPE);

            instrMapCB.emplace_back(&GameboyCPU::RES2, "RES2", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES2, "RES2", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES2, "RES2", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES2, "RES2", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES2, "RES2", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES2, "RES2", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES2, "RES2", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES2, "RES2", A, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES3, "RES3", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES3, "RES3", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES3, "RES3", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES3, "RES3", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES3, "RES3", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES3, "RES3", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES3, "RES3", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES3, "RES3", A, NO_DATA_TYPE);

            instrMapCB.emplace_back(&GameboyCPU::RES4, "RES4", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES4, "RES4", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES4, "RES4", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES4, "RES4", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES4, "RES4", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES4, "RES4", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES4, "RES4", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES4, "RES4", A, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES5, "RES5", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES5, "RES5", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES5, "RES5", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES5, "RES5", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES5, "RES5", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES5, "RES5", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES5, "RES5", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES5, "RES5", A, NO_DATA_TYPE);

            instrMapCB.emplace_back(&GameboyCPU::RES6, "RES6", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES6, "RES6", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES6, "RES6", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES6, "RES6", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES6, "RES6", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES6, "RES6", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES6, "RES6", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES6, "RES6", A, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES7, "RES7", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES7, "RES7", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES7, "RES7", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES7, "RES7", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES7, "RES7", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES7, "RES7", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES7, "RES7", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::RES7, "RES7", A, NO_DATA_TYPE);

            instrMapCB.emplace_back(&GameboyCPU::SET0, "SET0", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET0, "SET0", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET0, "SET0", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET0, "SET0", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET0, "SET0", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET0, "SET0", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET0, "SET0", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET0, "SET0", A, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET1, "SET1", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET1, "SET1", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET1, "SET1", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET1, "SET1", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET1, "SET1", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET1, "SET1", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET1, "SET1", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET1, "SET1", A, NO_DATA_TYPE);

            instrMapCB.emplace_back(&GameboyCPU::SET2, "SET2", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET2, "SET2", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET2, "SET2", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET2, "SET2", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET2, "SET2", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET2, "SET2", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET2, "SET2", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET2, "SET2", A, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET3, "SET3", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET3, "SET3", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET3, "SET3", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET3, "SET3", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET3, "SET3", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET3, "SET3", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET3, "SET3", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET3, "SET3", A, NO_DATA_TYPE);

            instrMapCB.emplace_back(&GameboyCPU::SET4, "SET4", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET4, "SET4", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET4, "SET4", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET4, "SET4", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET4, "SET4", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET4, "SET4", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET4, "SET4", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET4, "SET4", A, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET5, "SET5", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET5, "SET5", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET5, "SET5", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET5, "SET5", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET5, "SET5", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET5, "SET5", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET5, "SET5", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET5, "SET5", A, NO_DATA_TYPE);

            instrMapCB.emplace_back(&GameboyCPU::SET6, "SET6", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET6, "SET6", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET6, "SET6", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET6, "SET6", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET6, "SET6", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET6, "SET6", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET6, "SET6", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET6, "SET6", A, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET7, "SET7", B, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET7, "SET7", C, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET7, "SET7", D, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET7, "SET7", E, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET7, "SET7", H, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET7, "SET7", L, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET7, "SET7", HL_ref, NO_DATA_TYPE);
            instrMapCB.emplace_back(&GameboyCPU::SET7, "SET7", A, NO_DATA_TYPE);
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
            ACCESS HARDWARE INFO
        *********************************************************************************************************** */
        void GameboyCPU::UpdateDebugData(debug_data* _data) const {
            _data->pc = (int)Regs.PC;

            if (_data->pc < ROM_N_OFFSET) {
                _data->bank = 0;
            } else if (_data->pc < VRAM_N_OFFSET) {
                _data->bank = machineCtx->rom_bank_selected + 1;
            } else {
                _data->bank = -1;
            }

            _data->callstack = callstack;
        }

        int GameboyCPU::GetPlayerCount() const {
            return GB_PLAYER_COUNT;
        }

        // get current hardware status (currently mapped memory banks, etc.)
        void GameboyCPU::GetHardwareInfo(std::vector<data_entry>& _hardware_info) const {
            _hardware_info.clear();
            _hardware_info.emplace_back("Speedmode", format("{:d}", machineCtx->currentSpeed));
            _hardware_info.emplace_back("ROM banks", format("{:d}", machineCtx->rom_bank_num));
            _hardware_info.emplace_back("ROM selected", format("{:d}", machineCtx->rom_bank_selected + 1));
            _hardware_info.emplace_back("RAM banks", format("{:d}", machineCtx->ram_bank_num));
            _hardware_info.emplace_back("RAM selected", format("{:d}", machineCtx->ram_bank_selected));
            _hardware_info.emplace_back("WRAM banks", format("{:d}", machineCtx->wram_bank_num));
            _hardware_info.emplace_back("WRAM selected", format("{:d}", machineCtx->wram_bank_selected + 1));
            _hardware_info.emplace_back("VRAM banks", format("{:d}", machineCtx->vram_bank_num));
            _hardware_info.emplace_back("VRAM selected", format("{:d}", machineCtx->vram_bank_selected));
        }

        void GameboyCPU::GetInstrDebugFlags(std::vector<reg_entry>& _register_values, std::vector<reg_entry>& _flag_values, std::vector<reg_entry>& _misc_values) const {
            _register_values.clear();
            _register_values.emplace_back(REGISTER_NAMES.at(A) + REGISTER_NAMES.at(F), format("A:{:02x} F:{:02x}", Regs.A, Regs.F));
            _register_values.emplace_back(REGISTER_NAMES.at(BC), format("{:04x}", Regs.BC));
            _register_values.emplace_back(REGISTER_NAMES.at(DE), format("{:04x}", Regs.DE));
            _register_values.emplace_back(REGISTER_NAMES.at(HL), format("{:04x}", Regs.HL));
            _register_values.emplace_back(REGISTER_NAMES.at(SP), format("{:04x}", Regs.SP));
            _register_values.emplace_back(REGISTER_NAMES.at(PC), format("{:04x}", Regs.PC));
            _register_values.emplace_back(REGISTER_NAMES.at(IEreg), format("{:02x}", machineCtx->IE));
            _register_values.emplace_back(REGISTER_NAMES.at(IFreg), format("{:02x}", m_MemInstance.lock()->GetIO(IF_ADDR)));

            _flag_values.clear();
            _flag_values.emplace_back(FLAG_NAMES.at(FLAG_C), format("{:01b}", (Regs.F & FLAG_CARRY) >> 4));
            _flag_values.emplace_back(FLAG_NAMES.at(FLAG_H), format("{:01b}", (Regs.F & FLAG_HCARRY) >> 5));
            _flag_values.emplace_back(FLAG_NAMES.at(FLAG_N), format("{:01b}", (Regs.F & FLAG_SUB) >> 6));
            _flag_values.emplace_back(FLAG_NAMES.at(FLAG_Z), format("{:01b}", (Regs.F & FLAG_ZERO) >> 7));
            _flag_values.emplace_back(FLAG_NAMES.at(FLAG_IME), format("{:01b}", ime ? 1 : 0));
            u8 isr_requested = m_MemInstance.lock()->GetIO(IF_ADDR);
            _flag_values.emplace_back(FLAG_NAMES.at(INT_VBLANK), format("{:01b}", (isr_requested & IRQ_VBLANK)));
            _flag_values.emplace_back(FLAG_NAMES.at(INT_STAT), format("{:01b}", (isr_requested & IRQ_LCD_STAT) >> 1));
            _flag_values.emplace_back(FLAG_NAMES.at(INT_TIMER), format("{:01b}", (isr_requested & IRQ_TIMER) >> 2));
            _flag_values.emplace_back(FLAG_NAMES.at(INT_SERIAL), format("{:01b}", (isr_requested & IRQ_SERIAL) >> 3));
            _flag_values.emplace_back(FLAG_NAMES.at(INT_JOYPAD), format("{:01b}", (isr_requested & IRQ_JOYPAD) >> 4));

            _misc_values.clear();
            _misc_values.emplace_back("LCDC", format("{:08b} (bin)", m_MemInstance.lock()->GetIO(LCDC_ADDR)));
            _misc_values.emplace_back("STAT", format("{:08b} (bin)", m_MemInstance.lock()->GetIO(STAT_ADDR)));
            _misc_values.emplace_back("WRAM", format("{:01d} (dec)", m_MemInstance.lock()->GetIO(CGB_WRAM_SELECT_ADDR)));
            _misc_values.emplace_back("VRAM", format("{:01d} (dec)", m_MemInstance.lock()->GetIO(CGB_VRAM_SELECT_ADDR)));
            _misc_values.emplace_back("LY", format("{:03d} (dec)", m_MemInstance.lock()->GetIO(LY_ADDR)));
            _misc_values.emplace_back("LYC", format("{:03d} (dec)", m_MemInstance.lock()->GetIO(LYC_ADDR)));
            _misc_values.emplace_back("SCX", format("{:03d} (dec)", m_MemInstance.lock()->GetIO(SCX_ADDR)));
            _misc_values.emplace_back("SCY", format("{:03d} (dec)", m_MemInstance.lock()->GetIO(SCY_ADDR)));
            _misc_values.emplace_back("WX", format("{:03d} (dec)", m_MemInstance.lock()->GetIO(WX_ADDR)));
            _misc_values.emplace_back("WY", format("{:03d} (dec)", m_MemInstance.lock()->GetIO(WY_ADDR)));
            _misc_values.emplace_back("Mode", format("{:1d} (dec)", graphics_ctx->mode));
        }

        void GameboyCPU::GetMemoryTypes(std::map<int, std::string>& _map) const {
            _map.clear();
            _map[MEM_TYPE::ROM0] = "ROM";
            _map[MEM_TYPE::RAMn] = "RAM";
            _map[MEM_TYPE::VRAM] = "VRAM";
            _map[MEM_TYPE::WRAM0] = "WRAM";
            _map[MEM_TYPE::OAM] = "OAM";
            _map[MEM_TYPE::IO] = "IO";
            _map[MEM_TYPE::HRAM] = "HRAM";
            _map[MEM_TYPE::IE] = "IE";
        }

        /* ***********************************************************************************************************
            PREPARE DEBUG DATA
        *********************************************************************************************************** */
        void GameboyCPU::DisassembleBankContent(assembly_table& _program_buffer, u8* _bank_data, const int& _offset, const size_t& _size, const int& _bank_num, const string& _bank_name) {
            u16 data = 0;
            bool cb = false;

            _program_buffer.clear();
            auto current_entry = std::tuple<int, instr_entry>();                // a table entry consists of the address and a pair of two strings (left and right column of debugger)

            std::string info = "";

            for (u16 addr = 0; addr < _size;) {
                current_entry = std::tuple<int, instr_entry>();

                if (_bank_num == 0 && _bank_name.compare("ROM") == 0) {
                    switch (addr) {
                    case ISR_VBLANK_HANDLER_ADDR:
                        info = "VBLANK";
                        break;
                    case ISR_LCD_STAT_HANDLER_ADDR:
                        info = "STAT";
                        break;
                    case ISR_TIMER_HANDLER_ADDR:
                        info = "TIMER";
                        break;
                    case ISR_SERIAL_HANDLER_ADDR:
                        info = "SERIAL";
                        break;
                    case ISR_JOYPAD_HANDLER_ADDR:
                        info = "JOYPAD";
                        break;
                    case ROM_HEAD_LOGO:
                        get<0>(current_entry) = addr;
                        get<1>(current_entry).first = "ROM" + to_string(_bank_num) + ": " + format("{:04x}  ", addr);
                        get<1>(current_entry).second = "- HEADER INFO -";
                        addr = ROM_HEAD_END + 1;
                        _program_buffer.emplace_back(current_entry);
                        current_entry = std::tuple<int, instr_entry>();
                        break;
                    }
                }

                // get opcode and set entry address
                u8 opcode = _bank_data[addr];
                get<0>(current_entry) = _offset + addr;          // set the entry address

                {
                    instr_tuple* instr_ptr;

                    // check for default or CB instruction and add opcode to result
                    instr_ptr = &(instrMap[opcode]);
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
                                data = _bank_data[addr]; addr++;

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
                                data = _bank_data[addr];
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
                                data = _bank_data[addr];                 addr++;
                                data |= ((u16)_bank_data[addr]) << 8;    addr++;

                                result_binary += format(" {:02x}", (u8)(data & 0xFF));
                                result_binary += format(" {:02x}", (u8)((data & 0xFF00) >> 8));

                                result_string += (i == 0 ? "" : ",");
                                if (arg == a16_ref) {
                                    result_string += format(" ({:04x})", data);
                                } else {
                                    result_string += format(" {:04x}", data);
                                }
                                break;
                            case SP_r8:
                            {
                                data = _bank_data[addr]; addr++;
                                i8 signed_data = *(i8*)&data;

                                result_binary += format(" {:02x}", (u8)(data & 0xFF));

                                result_string += (i == 0 ? "" : ",");
                                result_string += format("SP+{:02x}", signed_data);
                            }
                                break;
                            case BC_ref:
                            case DE_ref:
                            case HL_ref:
                            case HL_INC_ref:
                            case HL_DEC_ref:
                            case C_ref:
                                result_string += (i == 0 ? "" : ",");

                                result_string += " " + DATA_NAMES.at(arg);
                                break;
                            default:
                                break;
                            }
                        }

                        if (cb) {
                            opcode = _bank_data[addr];
                            addr++;
                            instr_ptr = &(instrMapCB[opcode]);

                            result_string += " " + get<INSTR_MNEMONIC>(*instr_ptr);
                            result_binary += " " + format("{:02x}", opcode);
                        }
                
                        if (info.compare("") != 0) {
                            while (result_string.length() < 20) {
                                result_string += " ";
                            }
                        }

                        get<1>(current_entry).first = result_binary;
                        get<1>(current_entry).second = result_string + info;
                        info = "";
                    }
                }

                _program_buffer.emplace_back(current_entry);
            }
        }

        void GameboyCPU::GenerateAssemblyTables(std::shared_ptr<BaseCartridge> _cartridge) {
            auto& rom_data = _cartridge->GetRom();
            size_t bank_num = rom_data.size() / ROM_N_SIZE;
            asmTables = assembly_tables(bank_num);

            assembly_table current_table;
            int offset = 0;

            for (int i = 0; i < (int)bank_num; i++) {
                current_table = assembly_table();

                if (i == 0)     { offset = ROM_0_OFFSET; } 
                else            { offset = ROM_N_OFFSET; }

                auto* rom_data_ptr = rom_data.data() + i * ROM_N_SIZE;
                DisassembleBankContent(current_table, rom_data_ptr, offset, ROM_N_SIZE, i, "ROM");
                asmTables[i] = current_table;
            }
        }

        void GameboyCPU::GenerateTemporaryAssemblyTable(assembly_tables& _table) {
            int bank_num;
            _table = assembly_tables(1);

            if (Regs.PC >= VRAM_N_OFFSET && Regs.PC < RAM_N_OFFSET) {
                bank_num = m_MemInstance.lock()->GetIO(CGB_VRAM_SELECT_ADDR);
                DisassembleBankContent(_table.back(), graphics_ctx->VRAM_N[bank_num].data(), VRAM_N_OFFSET, VRAM_N_SIZE, bank_num, "VRAM");
            } else if (Regs.PC >= RAM_N_OFFSET && Regs.PC < WRAM_0_OFFSET) {
                bank_num = machineCtx->ram_bank_selected;
                std::vector<u8> ram = std::vector<u8>(RAM_N_SIZE);
                memcpy(ram.data(), m_MemInstance.lock()->RAM_N[bank_num], RAM_N_SIZE);
                DisassembleBankContent(_table.back(), ram.data(), RAM_N_OFFSET, RAM_N_SIZE, bank_num, "RAM");
            } else if (Regs.PC >= WRAM_0_OFFSET && Regs.PC < WRAM_N_OFFSET) {
                bank_num = 0;
                DisassembleBankContent(_table.back(), m_MemInstance.lock()->WRAM_0.data(), WRAM_0_OFFSET, WRAM_0_SIZE, bank_num, "WRAM");
            } else if (Regs.PC >= WRAM_N_OFFSET && Regs.PC < MIRROR_WRAM_OFFSET) {
                bank_num = machineCtx->wram_bank_selected;
                DisassembleBankContent(_table.back(), m_MemInstance.lock()->WRAM_N[bank_num].data(), WRAM_N_OFFSET, WRAM_N_SIZE, bank_num + 1, "WRAM");
            } else if (Regs.PC >= HRAM_OFFSET && Regs.PC < IE_OFFSET) {
                bank_num = 0;
                DisassembleBankContent(_table.back(), m_MemInstance.lock()->HRAM.data(), HRAM_OFFSET, HRAM_SIZE, bank_num, "HRAM");
            } else {
                // TODO
            }
        }

        void GameboyCPU::AddToCallstack(const u16& _dest) {
            callstack_data cs_data = {};
            cs_data.src_addr = Regs.PC;
            cs_data.dest_addr = data;

            bool not_found = false;

            switch (Regs.PC & 0xF000) {
            case 0x0000:
            case 0x1000:
            case 0x2000:
            case 0x3000:
                cs_data.src_mem_type = 0;
                cs_data.src_bank = 0;
                break;
            case 0x4000:
            case 0x5000:
            case 0x6000:
            case 0x7000:
                cs_data.src_mem_type = 0;
                cs_data.src_bank = machineCtx->rom_bank_selected + 1;
                break;
            case 0xA000:
            case 0xB000:
                cs_data.src_mem_type = 2;
                cs_data.src_bank = machineCtx->ram_bank_selected;
                break;
            case 0xC000:
                cs_data.src_mem_type = 3;
                cs_data.src_bank = 0;
                break;
            case 0xD000:
                cs_data.src_mem_type = 3;
                cs_data.src_bank = machineCtx->wram_bank_selected + 1;
                break;
            case 0xF000:
                if (((Regs.PC & 0xFF80) == 0xFF80) && Regs.PC != 0xFFFF) {
                    cs_data.src_mem_type = 4;
                    cs_data.src_bank = 0;
                } else {
                    not_found = true;
                }
                break;
            default:
                not_found = true;
                break;
            }

            if (not_found) {
                LOG_ERROR("[emu] source memory type not implemented for callstack: ", std::format("{:04x}", Regs.PC));
                return;
            }

            not_found = false;

            switch (_dest & 0xF000) {
            case 0x0000:
            case 0x1000:
            case 0x2000:
            case 0x3000:
                cs_data.dest_mem_type = 0;
                cs_data.dest_bank = 0;
                break;
            case 0x4000:
            case 0x5000:
            case 0x6000:
            case 0x7000:
                cs_data.dest_mem_type = 0;
                cs_data.dest_bank = machineCtx->rom_bank_selected + 1;
                break;
            case 0xA000:
            case 0xB000:
                cs_data.dest_mem_type = 2;
                cs_data.dest_bank = machineCtx->ram_bank_selected;
                break;
            case 0xC000:
                cs_data.dest_mem_type = 3;
                cs_data.dest_bank = 0;
                break;
            case 0xD000:
                cs_data.dest_mem_type = 3;
                cs_data.dest_bank = machineCtx->wram_bank_selected + 1;
                break;
            case 0xF000:
                if (((_dest & 0xFF80) == 0xFF80) && _dest != 0xFFFF) {
                    cs_data.dest_mem_type = 4;
                    cs_data.dest_bank = 0;
                } else {
                    not_found = true;
                }
                break;
            default:
                not_found = true;
                break;
            }

            if (not_found) {
                LOG_ERROR("[emu] destination memory type not implemented for callstack: ", std::format("{:04x}", _dest));
                return;
            }

            callstack.emplace_back(cs_data);
            stackpointerLastCall = Regs.SP;
            callCount++;

            //callCount -= returnCount;
            //returnCount = 0;

            LOG_WARN(callstack.size(), ": ", callCount, ", ", returnCount);
        }

        void GameboyCPU::RemoveFromCallstack() {
            if (stackpointerLastCall == Regs.SP) {
                callstack.pop_back();
            }
            returnCount++;
            LOG_WARN(callstack.size(), ": ", callCount, ", ", returnCount);
        }
    }
}