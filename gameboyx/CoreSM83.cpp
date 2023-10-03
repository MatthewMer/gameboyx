#include "CoreSM83.h"

/* ***********************************************************************************************************
    INCLUDES
*********************************************************************************************************** */
#include "gameboy_defines.h"
#include <format>
#include "logger.h"

using namespace std;

/* ***********************************************************************************************************
    MISC LOOKUPS
*********************************************************************************************************** */
static const vector<pair<cgb_data_types, string>> register_names{
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
    {PC, "PC"}
};

static string get_register_name(const cgb_data_types& _type) {
    for (const auto& [type, val] : register_names) {
        if (type == _type) { return val; }
    }

    return "";
}

static const vector<pair<cgb_data_types, string>> data_names{
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
    {C_ref, "(C + $FF00)"},
};

static string get_data_name(const cgb_data_types& _type) {
    for (const auto& [type, val] : data_names) {
        if (type == _type) { return val; }
    }

    return "";
}

static string resolve_data_enum(const cgb_data_types& _type, const int& _addr, const u16& _data) {
    string result = "";
    u8 data;

    switch (_type) {
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
        result = get_register_name(_type);
        break;
    case d8:
    case a8:
        result = format("{:x}", (u8)(_data & 0xFF));
        break;
    case d16:
    case a16:
        result = format("{:x}", _data);
        break;
    case r8:
        data = (u8)(_data & 0xFF);
        result = format("{:x}", _addr + *(i8*)&data);
        break;
    case a8_ref:
        result = format("({:x} + $FF00)", (u8)(_data & 0xFF));
        break;
    case a16_ref:
        result = format("({:x})", _data);
        break;
    case SP_r8:
        data = (u8)(_data & 0xFF);
        result = format("SP+{:x}", *(i8*)&data);
        break;
    case BC_ref:
    case DE_ref:
    case HL_ref:
    case HL_INC_ref:
    case HL_DEC_ref:
    case C_ref:
        result = get_data_name(_type);
        break;
    }
    
    return result;
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

#define ADD_8_FLAGS(d, s, f) {f &= ~(FLAG_CARRY | FLAG_HCARRY); f |= ((u16)(d & 0xFF) + (s & 0xFF) > 0xFF ? FLAG_CARRY : 0); f |= ((d & 0xF) + (s & 0xF) > 0xF ? FLAG_HCARRY : 0);}
#define ADD_8_C(d, s, f) {f &= ~FLAG_CARRY; f |= ((u16)(d & 0xFF) + (s & 0xFF) > 0xFF ? FLAG_CARRY : 0);}
#define ADD_8_HC(d, s, f) {f &= ~FLAG_HCARRY; f |= ((d & 0xF) + (s & 0xF) > 0xF ? FLAG_HCARRY : 0);}

#define ADC_8_C(d, s, c, f) {f &= ~FLAG_CARRY; f |= ((u16)(d & 0xFF) + (u16)(s & 0xFF) + c > 0xFF ? FLAG_CARRY : 0);}
#define ADC_8_HC(d, s, c, f) {f &= ~FLAG_HCARRY; f |= ((d & 0xF) + (s & 0xF) + c > 0xF ? FLAG_HCARRY : 0);}

#define ADD_16_FLAGS(d, s, f) {f &= ~(FLAG_CARRY | FLAG_HCARRY); f |= ((u32)(d & 0xFFFF) + (s & 0xFFFF) > 0xFFFF ? FLAG_CARRY : 0); f |= ((u16)(d & 0xFFF) + (s & 0xFFF) > 0xFFF ? FLAG_HCARRY : 0);}

#define SUB_8_FLAGS(d, s, f) {f &= ~(FLAG_CARRY | FLAG_HCARRY); f |= ((d & 0xFF) < (s & 0xFF) ? FLAG_CARRY : 0); f |= ((d & 0xF) < (s & 0xF) ? FLAG_HCARRY : 0);}
#define SUB_8_C(d, s, f) {f &= ~FLAG_CARRY; f |= ((d & 0xFF) < (s & 0xFF) ? FLAG_CARRY : 0);}
#define SUB_8_HC(d, s, f) {f &= ~FLAG_HCARRY; f|= ((d & 0xF) < (s & 0xF) ? FLAG_HCARRY : 0);}

#define SBC_8_C(d, s, c, f) {f &= ~FLAG_CARRY; f |= ((d & 0xFF) < (((u16)s & 0xFF) + c) ? FLAG_CARRY : 0);}
#define SBC_8_HC(d, s, c, f) {f &= ~FLAG_HCARRY; f |= ((d & 0xF) < ((s & 0xF) + c) ? FLAG_HCARRY : 0);}

#define ZERO_FLAG(x, f) {f &= ~FLAG_ZERO; f |= (x == 0 ? FLAG_ZERO : 0);}

/* ***********************************************************************************************************
    CONSTRUCTOR
*********************************************************************************************************** */
CoreSM83::CoreSM83(message_buffer& _msg_buffer) : CoreBase(_msg_buffer){
    machine_ctx = MemorySM83::getInstance()->GetMachineContext();
    InitCpu(*Cartridge::getInstance());

    setupLookupTable();
    setupLookupTableCB();
    

    InitMessageBufferProgram(_msg_buffer.program_buffer);
}

/* ***********************************************************************************************************
    INIT CPU
*********************************************************************************************************** */
// initial cpu state
void CoreSM83::InitCpu(const Cartridge& _cart_obj) {
    InitRegisterStates();

    machineCyclesPerFrame = ((BASE_CLOCK_CPU / 4) * pow(10, 6)) / DISPLAY_FREQUENCY;
}

// initial register states
void CoreSM83::InitRegisterStates() {
    Regs = gbc_registers();

    Regs.A = (CGB_AF & 0xFF00) >> 8;
    Regs.F = CGB_AF & 0xFF;

    Regs.BC = CGB_BC;
    Regs.SP = CGB_SP;
    Regs.PC = CGB_PC;

    if (machine_ctx->isCgb) {
        Regs.DE = CGB_CGB_DE;
        Regs.HL = CGB_CGB_HL;
    }
    else {
        Regs.DE = CGB_DMG_DE;
        Regs.HL = CGB_DMG_HL;
    }
}


/* ***********************************************************************************************************
    RUN CPU
*********************************************************************************************************** */
void CoreSM83::RunCycles() {
    if (halted) {
        halted = (machine_ctx->IE & machine_ctx->IF) == 0x00;
    }
    else {
        if (msgBuffer.instruction_debug_enabled) {
            if (msgBuffer.pause_execution) {
                return;
            }
            else {
                RunCpu();
                msgBuffer.pause_execution = true;
            }
        }
        else {
            while (machineCycles < machineCyclesPerFrame * machine_ctx->currentSpeed) {
                RunCpu();
                if (halted) { return; }
            }
        }
    }
}

void CoreSM83::RunCpu() {
    ExecuteInterrupts();
    ExecuteInstruction();
    ExecuteMachineCycles();
}

void CoreSM83::ExecuteInstruction() {
    opcode = mmu_instance->Read8Bit(Regs.PC);
    curPC = Regs.PC;
    Regs.PC++;

    if (opcodeCB) {
        instrPtr = &instrMapCB[opcode];
        opcodeCB = false;
    }
    else {
        instrPtr = &instrMap[opcode];
    }

    functionPtr = get<1>(*instrPtr);
    currentMachineCycles = get<2>(*instrPtr);
    (this->*functionPtr)();
    machineCycles += currentMachineCycles;
    machineCycleCounterClock += currentMachineCycles;
}

void CoreSM83::ExecuteInterrupts() {
    if (ime) {
        if (machine_ctx->IF & ISR_VBLANK) {
            if (machine_ctx->IE & ISR_VBLANK) {
                ime = false;

                isr_push(ISR_VBLANK_HANDLER);
                machine_ctx->IF &= ~ISR_VBLANK;
            }
        }
        if (machine_ctx->IF & ISR_LCD_STAT) {
            if (machine_ctx->IE & ISR_LCD_STAT) {
                ime = false;

                isr_push(ISR_LCD_STAT_HANDLER);
                machine_ctx->IF &= ~ISR_LCD_STAT;
            }
        }
        if (machine_ctx->IF & ISR_TIMER) {
            if (machine_ctx->IE & ISR_TIMER) {
                ime = false;

                isr_push(ISR_TIMER_HANDLER);
                machine_ctx->IF &= ~ISR_TIMER;
            }
        }
        /*if (machine_ctx->IF & ISR_SERIAL) {
            // not implemented
        }*/
        if (machine_ctx->IF & ISR_JOYPAD) {
            if (machine_ctx->IE & ISR_JOYPAD) {
                ime = false;

                isr_push(ISR_JOYPAD_HANDLER);
                machine_ctx->IF &= ~ISR_JOYPAD;
            }
        }
    }
}

void CoreSM83::ExecuteMachineCycles() {
    machine_ctx->machineCyclesDIVCounter += currentMachineCycles;
    if (machine_ctx->machineCyclesDIVCounter > machine_ctx->machineCyclesPerDIVIncrement) {
        machine_ctx->machineCyclesDIVCounter -= machine_ctx->machineCyclesPerDIVIncrement;

        if (machine_ctx->DIV == 0xFF) {
            machine_ctx->DIV = 0x00;
            // TODO: interrupt or whatever
        }
        else {
            machine_ctx->DIV++;
        }
    }

    if (machine_ctx->TAC & TAC_CLOCK_ENABLE) {
        machine_ctx->machineCyclesTIMACounter += currentMachineCycles;
        if (machine_ctx->machineCyclesTIMACounter > machine_ctx->machineCyclesPerTIMAIncrement) {
            machine_ctx->machineCyclesTIMACounter -= machine_ctx->machineCyclesPerTIMAIncrement;

            if (machine_ctx->TIMA == 0xFF) {
                machine_ctx->TIMA = machine_ctx->TMA;
                // request interrupt
                machine_ctx->IF |= ISR_TIMER;
            }
            else {
                machine_ctx->TIMA++;
            }
        }
    }
}

/* ***********************************************************************************************************
    ACCESS CPU STATUS
*********************************************************************************************************** */
// return delta t per frame in nanoseconds
int CoreSM83::GetDelayTime() {
    return (1.f / DISPLAY_FREQUENCY) * pow(10, 9);
}

// get the machines LCD vertical frequency
int CoreSM83::GetDisplayFrequency() const {
    return DISPLAY_FREQUENCY;
}

// check if cpu executed machinecycles per frame
bool CoreSM83::CheckNextFrame() {
    if (machineCycles >= machineCyclesPerFrame * machine_ctx->currentSpeed) {
        machineCycles -= machineCyclesPerFrame * machine_ctx->currentSpeed;
        return true;
    }
    else {
        return false;
    }
}

// return clock cycles per second
u32 CoreSM83::GetPassedClockCycles() {
    u32 result = machineCycleCounterClock * 4;
    machineCycleCounterClock = 0;
    return result;
}

void CoreSM83::GetCurrentMemoryLocation(message_buffer& _msg_buffer) const {
    _msg_buffer.current_pc = (int)Regs.PC;
    _msg_buffer.current_rom_bank = (Regs.PC < ROM_BANK_N_OFFSET ? 0 : machine_ctx->rom_bank_selected + 1);
}

/* ***********************************************************************************************************
    ACCESS HARDWARE STATUS
*********************************************************************************************************** */
// get hardware info startup
void CoreSM83::GetStartupHardwareInfo(message_buffer& _msg_buffer) const {
    _msg_buffer.rom_bank_num = machine_ctx->rom_bank_num;
    _msg_buffer.ram_bank_num = machine_ctx->ram_bank_num;
    _msg_buffer.wram_bank_num = machine_ctx->wram_bank_num;
    _msg_buffer.rom_bank_size = ROM_BANK_N_SIZE;
}

// get current hardware status (currently mapped memory banks)
void CoreSM83::GetCurrentHardwareState(message_buffer& _msg_buffer) const {
    _msg_buffer.rom_bank_selected = machine_ctx->rom_bank_selected + 1;
    _msg_buffer.ram_bank_selected = machine_ctx->ram_bank_selected;
    _msg_buffer.wram_bank_selected = machine_ctx->wram_bank_selected + 1;
}

void CoreSM83::InitMessageBufferProgram(vector<vector<tuple<int, int, string, string>>>& _program_buffer) {
    _program_buffer.clear();
    
    auto rom = vector<vector<u8>>();
    MemorySM83::getInstance()->CopyRomForDebug(rom);

    u16 addr = 0;
    int bank = 0;
    u16 data = 0;
    bool cb = false;

    int i = 0;
    
    _program_buffer.push_back(vector<tuple<int, int, string, string>>());
    for (addr = 0, i = 0; addr < ROM_BANK_0_SIZE; i++) {

        // print rom header info
        if (addr == ROM_HEAD_LOGO) {
            tuple<int, int, string, string> current_entry(i, addr, "ROM" + to_string(bank) + ": " + format("{:x}  ", addr), "- HEADER INFO -");
            addr = ROM_HEAD_END + 1;
            _program_buffer.back().push_back(current_entry);
        }
        else {

            u8 opcode = rom[0][addr];
            tuple<int, int, string, string> current_entry(i, addr, "", "");

            instr_tuple* instr_ptr;

            if (cb) {
                instr_ptr = &instrMapCB[opcode];
                cb = false;
            }
            else {
                instr_ptr = &instrMap[opcode];
            }

            string raw_data = "ROM" + to_string(bank) + ": " + format("{:x}  ", addr);
            addr++;

            raw_data += format("{:x} ", opcode);

            // first parameter
            switch (get<4>(*instr_ptr)) {
            case d8:
            case a8:
            case a8_ref:
            case r8:
                data = rom[0][addr++];
                raw_data += format("{:x} ", (u8)data);
                break;
            case d16:
            case a16:
            case a16_ref:
                data = rom[0][addr++];
                data |= ((u16)rom[0][addr++]) << 8;
                raw_data += format("{:x} ", (u8)(data & 0xFF));
                raw_data += format("{:x} ", (u8)((data & 0xFF00) >> 8));
                break;
            }


            // second parameter
            switch (get<5>(*instr_ptr)) {
            case d8:
            case a8:
            case a8_ref:
            case r8:
                data = rom[0][addr++];
                raw_data += format("{:x} ", (u8)data);
                break;
            case d16:
            case a16:
            case a16_ref:
                data = rom[0][addr++];
                data |= ((u16)rom[0][addr++]) << 8;
                raw_data += format("{:x} ", (u8)(data & 0xFF));
                raw_data += format("{:x} ", (u8)((data & 0xFF00) >> 8));
                break;
            }
            get<2>(current_entry) = raw_data;

            string asm_data = get<3>(*instr_ptr);
            string args;
            if (get<4>(*instr_ptr) != NO_DATA) {
                args = resolve_data_enum(get<4>(*instr_ptr), addr, data);
            }
            if (get<5>(*instr_ptr) != NO_DATA) {
                args += ", " + resolve_data_enum(get<5>(*instr_ptr), addr, data);
            }
            get<3>(current_entry) = asm_data;
            if (args.compare("") != 0) {
                get<3>(current_entry) += " " + args;
            }

            _program_buffer.back().push_back(current_entry);
        }
    }

    for (bank = 1; bank < machine_ctx->rom_bank_num; bank++) {
        _program_buffer.push_back(vector<tuple<int, int, string, string>>());
        for (addr = 0, i = 0; addr < ROM_BANK_0_SIZE; i++) {

            u8 opcode = rom[bank][addr];
            tuple<int, int, string, string> current_entry(i, addr + ROM_BANK_N_OFFSET, "", "");

            instr_tuple* instr_ptr;

            if (cb) {
                instr_ptr = &instrMapCB[opcode];
                cb = false;
            }
            else {
                instr_ptr = &instrMap[opcode];
            }

            string raw_data = "ROM" + to_string(bank) + ": " + format("{:x}  ", addr + ROM_BANK_N_OFFSET);
            addr++;

            raw_data += format("{:x} ", opcode);

            // first parameter
            switch (get<4>(*instr_ptr)) {
            case d8:
            case a8:
            case a8_ref:
            case r8:
                data = rom[bank][addr++];
                raw_data += format("{:x} ", (u8)data);
                break;
            case d16:
            case a16:
            case a16_ref:
                data = rom[bank][addr++];
                data |= ((u16)rom[0][addr++]) << 8;
                raw_data += format("{:x} ", (u8)(data & 0xFF));
                raw_data += format("{:x} ", (u8)((data & 0xFF00) >> 8));
                break;
            }


            // second parameter
            switch (get<5>(*instr_ptr)) {
            case d8:
            case a8:
            case a8_ref:
            case r8:
                data = rom[bank][addr++];
                raw_data += format("{:x} ", (u8)data);
                break;
            case d16:
            case a16:
            case a16_ref:
                data = rom[bank][addr++];
                data |= ((u16)rom[0][addr++]) << 8;
                raw_data += format("{:x} ", (u8)(data & 0xFF));
                raw_data += format("{:x} ", (u8)((data & 0xFF00) >> 8));
                break;
            }
            get<2>(current_entry) = raw_data;

            string asm_data = get<3>(*instr_ptr);
            string args;
            if (get<4>(*instr_ptr) != NO_DATA) {
                args = resolve_data_enum(get<4>(*instr_ptr), addr + ROM_BANK_N_OFFSET, data);
            }
            if (get<5>(*instr_ptr) != NO_DATA) {
                args += ", " + resolve_data_enum(get<5>(*instr_ptr), addr + ROM_BANK_N_OFFSET, data);
            }
            get<3>(current_entry) = asm_data;
            if (args.compare("") != 0) {
                get<3>(current_entry) += " " + args;
            }

            _program_buffer.back().push_back(current_entry);
        }
    }
}

void CoreSM83::GetCurrentRegisterValues(vector<pair<string, string>>& _register_values) {
    _register_values.clear();

    _register_values.emplace_back(get_register_name(A), format("{:x}", Regs.A));
    _register_values.emplace_back(get_register_name(F), format("{:x}", Regs.F));
    _register_values.emplace_back(get_register_name(BC), format("{:x}", Regs.BC));
    _register_values.emplace_back(get_register_name(DE), format("{:x}", Regs.DE));
    _register_values.emplace_back(get_register_name(HL), format("{:x}", Regs.HL));
    _register_values.emplace_back(get_register_name(SP), format("{:x}", Regs.SP));
    _register_values.emplace_back(get_register_name(PC), format("{:x}", Regs.PC));
}

/* ***********************************************************************************************************
*
*   GAMEBOY (COLOR) CORESM83 INSTRUCTION LOOKUP TABLE
*
*********************************************************************************************************** */
void CoreSM83::setupLookupTable() {
    instrMap.clear();

    // Elements: opcode, instruction function, machine cycles, bytes (without opcode), instr_info
    
    // 0x00
    instrMap.emplace_back(0x00, &CoreSM83::NOP, 1, "NOP", NO_DATA, NO_DATA);
    instrMap.emplace_back(0x01, &CoreSM83::LDd16, 3, "LD", BC, d16);
    instrMap.emplace_back(0x02, &CoreSM83::LDfromAtoRef, 2, "LD", BC_ref, A);
    instrMap.emplace_back(0x03, &CoreSM83::INC16, 2, "INC", BC, NO_DATA);
    instrMap.emplace_back(0x04, &CoreSM83::INC8, 1, "INC", B, NO_DATA);
    instrMap.emplace_back(0x05, &CoreSM83::DEC8, 1, "DEC", B, NO_DATA);
    instrMap.emplace_back(0x06, &CoreSM83::LDd8, 2, "LD", B, d8);
    instrMap.emplace_back(0x07, &CoreSM83::RLCA, 1, "RLCA", A, NO_DATA);
    instrMap.emplace_back(0x08, &CoreSM83::LDSPa16, 5, "LD", a16_ref, SP);
    instrMap.emplace_back(0x09, &CoreSM83::ADDHL, 2, "ADD", HL, BC);
    instrMap.emplace_back(0x0a, &CoreSM83::LDtoAfromRef, 2, "LD", A, BC_ref);
    instrMap.emplace_back(0x0b, &CoreSM83::DEC16, 2, "DEC", BC, NO_DATA);
    instrMap.emplace_back(0x0c, &CoreSM83::INC8, 1, "INC", C, NO_DATA);
    instrMap.emplace_back(0x0d, &CoreSM83::DEC8, 1, "DEC", C, NO_DATA);
    instrMap.emplace_back(0x0e, &CoreSM83::LDd8, 2, "LD", C, d8);
    instrMap.emplace_back(0x0f, &CoreSM83::RRCA, 1, "RRCA", A, NO_DATA);
    
    // 0x10
    instrMap.emplace_back(0x10, &CoreSM83::STOP, 1, "STOP", d8, NO_DATA);
    instrMap.emplace_back(0x11, &CoreSM83::LDd16, 3, "LD", DE, d16);
    instrMap.emplace_back(0x12, &CoreSM83::LDfromAtoRef, 2, "LD", DE_ref, A);
    instrMap.emplace_back(0x13, &CoreSM83::INC16, 2, "INC", DE, NO_DATA);
    instrMap.emplace_back(0x14, &CoreSM83::INC8, 1, "INC", D, NO_DATA);
    instrMap.emplace_back(0x15, &CoreSM83::DEC8, 1, "DEC", D, NO_DATA);
    instrMap.emplace_back(0x16, &CoreSM83::LDd8, 2, "LD", D, d8);
    instrMap.emplace_back(0x17, &CoreSM83::RLA, 1, "RLA", A, NO_DATA);
    instrMap.emplace_back(0x18, &CoreSM83::JR, 3, "JR", r8, NO_DATA);
    instrMap.emplace_back(0x19, &CoreSM83::ADDHL, 2, "ADD", HL, DE);
    instrMap.emplace_back(0x1a, &CoreSM83::LDtoAfromRef, 2, "LD", A, DE_ref);
    instrMap.emplace_back(0x1b, &CoreSM83::DEC16, 2, "DEC", DE, NO_DATA);
    instrMap.emplace_back(0x1c, &CoreSM83::INC8, 1, "INC", E, NO_DATA);
    instrMap.emplace_back(0x1d, &CoreSM83::DEC8, 1, "DEC", E, NO_DATA);
    instrMap.emplace_back(0x1e, &CoreSM83::LDd8, 2, "LD", E, d8);
    instrMap.emplace_back(0x1f, &CoreSM83::RRA, 1, "RRA", A, NO_DATA);

    // 0x20
    instrMap.emplace_back(0x20, &CoreSM83::JR, 0, "JR NZ", r8, NO_DATA);
    instrMap.emplace_back(0x21, &CoreSM83::LDd16, 3, "LD", HL, d16);
    instrMap.emplace_back(0x22, &CoreSM83::LDfromAtoRef, 2, "LD", HL_INC_ref, A);
    instrMap.emplace_back(0x23, &CoreSM83::INC16, 2, "INC", HL, NO_DATA);
    instrMap.emplace_back(0x24, &CoreSM83::INC8, 1, "INC", H, NO_DATA);
    instrMap.emplace_back(0x25, &CoreSM83::DEC8, 1, "DEC", H, NO_DATA);
    instrMap.emplace_back(0x26, &CoreSM83::LDd8, 2, "LD", H, d8);
    instrMap.emplace_back(0x27, &CoreSM83::DAA, 1, "DAA", A, NO_DATA);
    instrMap.emplace_back(0x28, &CoreSM83::JR, 0, "JR Z", r8, NO_DATA);
    instrMap.emplace_back(0x29, &CoreSM83::ADDHL, 2, "ADD", HL, HL);
    instrMap.emplace_back(0x2a, &CoreSM83::LDtoAfromRef, 2, "LD", A, HL_INC_ref);
    instrMap.emplace_back(0x2b, &CoreSM83::DEC16, 2, "DEC", HL, NO_DATA);
    instrMap.emplace_back(0x2c, &CoreSM83::INC8, 1, "INC", L, NO_DATA);
    instrMap.emplace_back(0x2d, &CoreSM83::DEC8, 1, "DEC", L, NO_DATA);
    instrMap.emplace_back(0x2e, &CoreSM83::LDd8, 2, "LD", L, NO_DATA);
    instrMap.emplace_back(0x2f, &CoreSM83::CPL, 1, "CPL", A, NO_DATA);
    
    // 0x30
    instrMap.emplace_back(0x30, &CoreSM83::JR, 0, "JR NC", r8, NO_DATA);
    instrMap.emplace_back(0x31, &CoreSM83::LDd16, 3, "LD", SP, d16);
    instrMap.emplace_back(0x32, &CoreSM83::LDfromAtoRef, 2, "LD", HL_DEC_ref, A);
    instrMap.emplace_back(0x33, &CoreSM83::INC16, 2, "INC", SP, NO_DATA);
    instrMap.emplace_back(0x34, &CoreSM83::INC8, 1, "INC", HL_ref, NO_DATA);
    instrMap.emplace_back(0x35, &CoreSM83::DEC8, 1, "DEC", HL_ref, NO_DATA);
    instrMap.emplace_back(0x36, &CoreSM83::LDd8, 2, "LD", HL_ref, d8);
    instrMap.emplace_back(0x37, &CoreSM83::SCF, 1, "SCF", NO_DATA, NO_DATA);
    instrMap.emplace_back(0x38, &CoreSM83::JR, 0, "JR C", r8, NO_DATA);
    instrMap.emplace_back(0x39, &CoreSM83::ADDHL, 2, "ADD", HL, SP);
    instrMap.emplace_back(0x3a, &CoreSM83::LDtoAfromRef, 2, "LD", A, HL_DEC_ref);
    instrMap.emplace_back(0x3b, &CoreSM83::DEC16, 2, "DEC", SP, NO_DATA);
    instrMap.emplace_back(0x3c, &CoreSM83::INC8, 1, "INC", A, NO_DATA);
    instrMap.emplace_back(0x3d, &CoreSM83::DEC8, 1, "DEC", A, NO_DATA);
    instrMap.emplace_back(0x3e, &CoreSM83::LDd8, 2, "LD", A, d8);
    instrMap.emplace_back(0x3f, &CoreSM83::CCF, 1, "CCF", NO_DATA, NO_DATA);
    
    // 0x40
    instrMap.emplace_back(0x40, &CoreSM83::LDtoB, 1, "LD", B, B);
    instrMap.emplace_back(0x41, &CoreSM83::LDtoB, 1, "LD", B, C);
    instrMap.emplace_back(0x42, &CoreSM83::LDtoB, 1, "LD", B, D);
    instrMap.emplace_back(0x43, &CoreSM83::LDtoB, 1, "LD", B, E);
    instrMap.emplace_back(0x44, &CoreSM83::LDtoB, 1, "LD", B, H);
    instrMap.emplace_back(0x45, &CoreSM83::LDtoB, 1, "LD", B, L);
    instrMap.emplace_back(0x46, &CoreSM83::LDtoB, 2, "LD", B, HL_ref);
    instrMap.emplace_back(0x47, &CoreSM83::LDtoB, 1, "LD", B, A);
    instrMap.emplace_back(0x48, &CoreSM83::LDtoC, 1, "LD", C, B);
    instrMap.emplace_back(0x49, &CoreSM83::LDtoC, 1, "LD", C, C);
    instrMap.emplace_back(0x4a, &CoreSM83::LDtoC, 1, "LD", C, D);
    instrMap.emplace_back(0x4b, &CoreSM83::LDtoC, 1, "LD", C, E);
    instrMap.emplace_back(0x4c, &CoreSM83::LDtoC, 1, "LD", C, H);
    instrMap.emplace_back(0x4d, &CoreSM83::LDtoC, 1, "LD", C, L);
    instrMap.emplace_back(0x4e, &CoreSM83::LDtoC, 2, "LD", C, HL_ref);
    instrMap.emplace_back(0x4f, &CoreSM83::LDtoC, 1, "LD", C, A);

    // 0x50
    instrMap.emplace_back(0x50, &CoreSM83::LDtoD, 1, "LD", D, B);
    instrMap.emplace_back(0x51, &CoreSM83::LDtoD, 1, "LD", D, C);
    instrMap.emplace_back(0x52, &CoreSM83::LDtoD, 1, "LD", D, D);
    instrMap.emplace_back(0x53, &CoreSM83::LDtoD, 1, "LD", D, E);
    instrMap.emplace_back(0x54, &CoreSM83::LDtoD, 1, "LD", D, H);
    instrMap.emplace_back(0x55, &CoreSM83::LDtoD, 1, "LD", D, L);
    instrMap.emplace_back(0x56, &CoreSM83::LDtoD, 2, "LD", D, HL_ref);
    instrMap.emplace_back(0x57, &CoreSM83::LDtoD, 1, "LD", D, A);
    instrMap.emplace_back(0x58, &CoreSM83::LDtoE, 1, "LD", E, B);
    instrMap.emplace_back(0x59, &CoreSM83::LDtoE, 1, "LD", E, C);
    instrMap.emplace_back(0x5a, &CoreSM83::LDtoE, 1, "LD", E, D);
    instrMap.emplace_back(0x5b, &CoreSM83::LDtoE, 1, "LD", E, E);
    instrMap.emplace_back(0x5c, &CoreSM83::LDtoE, 1, "LD", E, H);
    instrMap.emplace_back(0x5d, &CoreSM83::LDtoE, 1, "LD", E, L);
    instrMap.emplace_back(0x5e, &CoreSM83::LDtoE, 2, "LD", E, HL_ref);
    instrMap.emplace_back(0x5f, &CoreSM83::LDtoE, 1, "LD", E, A);

    // 0x60
    instrMap.emplace_back(0x60, &CoreSM83::LDtoH, 1, "LD", H, B);
    instrMap.emplace_back(0x61, &CoreSM83::LDtoH, 1, "LD", H, C);
    instrMap.emplace_back(0x62, &CoreSM83::LDtoH, 1, "LD", H, D);
    instrMap.emplace_back(0x63, &CoreSM83::LDtoH, 1, "LD", H, E);
    instrMap.emplace_back(0x64, &CoreSM83::LDtoH, 1, "LD", H, H);
    instrMap.emplace_back(0x65, &CoreSM83::LDtoH, 1, "LD", H, L);
    instrMap.emplace_back(0x66, &CoreSM83::LDtoH, 2, "LD", H, HL_ref);
    instrMap.emplace_back(0x67, &CoreSM83::LDtoH, 1, "LD", H, A);
    instrMap.emplace_back(0x68, &CoreSM83::LDtoL, 1, "LD", L, B);
    instrMap.emplace_back(0x69, &CoreSM83::LDtoL, 1, "LD", L, C);
    instrMap.emplace_back(0x6a, &CoreSM83::LDtoL, 1, "LD", L, D);
    instrMap.emplace_back(0x6b, &CoreSM83::LDtoL, 1, "LD", L, E);
    instrMap.emplace_back(0x6c, &CoreSM83::LDtoL, 1, "LD", L, H);
    instrMap.emplace_back(0x6d, &CoreSM83::LDtoL, 1, "LD", L, L);
    instrMap.emplace_back(0x6e, &CoreSM83::LDtoL, 2, "LD", L, HL_ref);
    instrMap.emplace_back(0x6f, &CoreSM83::LDtoL, 1, "LD", L, A);

    // 0x70
    instrMap.emplace_back(0x70, &CoreSM83::LDtoHLref, 2, "LD", HL_ref, B);
    instrMap.emplace_back(0x71, &CoreSM83::LDtoHLref, 2, "LD", HL_ref, C);
    instrMap.emplace_back(0x72, &CoreSM83::LDtoHLref, 2, "LD", HL_ref, D);
    instrMap.emplace_back(0x73, &CoreSM83::LDtoHLref, 2, "LD", HL_ref, E);
    instrMap.emplace_back(0x74, &CoreSM83::LDtoHLref, 2, "LD", HL_ref, H);
    instrMap.emplace_back(0x75, &CoreSM83::LDtoHLref, 2, "LD", HL_ref, L);
    instrMap.emplace_back(0x76, &CoreSM83::HALT, 1, "HALT", NO_DATA, NO_DATA);
    instrMap.emplace_back(0x77, &CoreSM83::LDtoHLref, 2, "LD", HL_ref, A);
    instrMap.emplace_back(0x78, &CoreSM83::LDtoA, 1, "LD", A, B);
    instrMap.emplace_back(0x79, &CoreSM83::LDtoA, 1, "LD", A, C);
    instrMap.emplace_back(0x7a, &CoreSM83::LDtoA, 1, "LD", A, D);
    instrMap.emplace_back(0x7b, &CoreSM83::LDtoA, 1, "LD", A, E);
    instrMap.emplace_back(0x7c, &CoreSM83::LDtoA, 1, "LD", A, H);
    instrMap.emplace_back(0x7d, &CoreSM83::LDtoA, 1, "LD", A, L);
    instrMap.emplace_back(0x7e, &CoreSM83::LDtoA, 2, "LD", A, HL_ref);
    instrMap.emplace_back(0x7f, &CoreSM83::LDtoA, 1, "LD", A, A);

    // 0x80
    instrMap.emplace_back(0x80, &CoreSM83::ADD8, 1, "ADD", A, B);
    instrMap.emplace_back(0x81, &CoreSM83::ADD8, 1, "ADD", A, C);
    instrMap.emplace_back(0x82, &CoreSM83::ADD8, 1, "ADD", A, D);
    instrMap.emplace_back(0x83, &CoreSM83::ADD8, 1, "ADD", A, E);
    instrMap.emplace_back(0x84, &CoreSM83::ADD8, 1, "ADD", A, H);
    instrMap.emplace_back(0x85, &CoreSM83::ADD8, 1, "ADD", A, L);
    instrMap.emplace_back(0x86, &CoreSM83::ADD8, 2, "ADD", A, HL_ref);
    instrMap.emplace_back(0x87, &CoreSM83::ADD8, 1, "ADD", A, A);
    instrMap.emplace_back(0x88, &CoreSM83::ADC, 1, "ADC", A, B);
    instrMap.emplace_back(0x89, &CoreSM83::ADC, 1, "ADC", A, C);
    instrMap.emplace_back(0x8a, &CoreSM83::ADC, 1, "ADC", A, D);
    instrMap.emplace_back(0x8b, &CoreSM83::ADC, 1, "ADC", A, E);
    instrMap.emplace_back(0x8c, &CoreSM83::ADC, 1, "ADC", A, H);
    instrMap.emplace_back(0x8d, &CoreSM83::ADC, 1, "ADC", A, L);
    instrMap.emplace_back(0x8e, &CoreSM83::ADC, 2, "ADC", A, HL_ref);
    instrMap.emplace_back(0x8f, &CoreSM83::ADC, 1, "ADC", A, A);

    // 0x90
    instrMap.emplace_back(0x90, &CoreSM83::SUB, 1, "SUB", A, B);
    instrMap.emplace_back(0x91, &CoreSM83::SUB, 1, "SUB", A, C);
    instrMap.emplace_back(0x92, &CoreSM83::SUB, 1, "SUB", A, D);
    instrMap.emplace_back(0x93, &CoreSM83::SUB, 1, "SUB", A, E);
    instrMap.emplace_back(0x94, &CoreSM83::SUB, 1, "SUB", A, H);
    instrMap.emplace_back(0x95, &CoreSM83::SUB, 1, "SUB", A, L);
    instrMap.emplace_back(0x96, &CoreSM83::SUB, 1, "SUB", A, HL_ref);
    instrMap.emplace_back(0x97, &CoreSM83::SUB, 2, "SUB", A, A);
    instrMap.emplace_back(0x98, &CoreSM83::SBC, 1, "SBC", A, B);
    instrMap.emplace_back(0x99, &CoreSM83::SBC, 1, "SBC", A, C);
    instrMap.emplace_back(0x9a, &CoreSM83::SBC, 1, "SBC", A, D);
    instrMap.emplace_back(0x9b, &CoreSM83::SBC, 1, "SBC", A, E);
    instrMap.emplace_back(0x9c, &CoreSM83::SBC, 1, "SBC", A, H);
    instrMap.emplace_back(0x9d, &CoreSM83::SBC, 1, "SBC", A, L);
    instrMap.emplace_back(0x9e, &CoreSM83::SBC, 2, "SBC", A, HL_ref);
    instrMap.emplace_back(0x9f, &CoreSM83::SBC, 1, "SBC", A, A);

    // 0xa0
    instrMap.emplace_back(0xa0, &CoreSM83::AND, 1, "AND", A, B);
    instrMap.emplace_back(0xa1, &CoreSM83::AND, 1, "AND", A, C);
    instrMap.emplace_back(0xa2, &CoreSM83::AND, 1, "AND", A, D);
    instrMap.emplace_back(0xa3, &CoreSM83::AND, 1, "AND", A, E);
    instrMap.emplace_back(0xa4, &CoreSM83::AND, 1, "AND", A, H);
    instrMap.emplace_back(0xa5, &CoreSM83::AND, 1, "AND", A, L);
    instrMap.emplace_back(0xa6, &CoreSM83::AND, 2, "AND", A, HL_ref);
    instrMap.emplace_back(0xa7, &CoreSM83::AND, 1, "AND", A, A);
    instrMap.emplace_back(0xa8, &CoreSM83::XOR, 1, "XOR", A, B);
    instrMap.emplace_back(0xa9, &CoreSM83::XOR, 1, "XOR", A, C);
    instrMap.emplace_back(0xaa, &CoreSM83::XOR, 1, "XOR", A, D);
    instrMap.emplace_back(0xab, &CoreSM83::XOR, 1, "XOR", A, E);
    instrMap.emplace_back(0xac, &CoreSM83::XOR, 1, "XOR", A, H);
    instrMap.emplace_back(0xad, &CoreSM83::XOR, 1, "XOR", A, L);
    instrMap.emplace_back(0xae, &CoreSM83::XOR, 2, "XOR", A, HL_ref);
    instrMap.emplace_back(0xaf, &CoreSM83::XOR, 1, "XOR", A, A);

    // 0xb0
    instrMap.emplace_back(0xb0, &CoreSM83::OR, 1, "OR", A, B);
    instrMap.emplace_back(0xb1, &CoreSM83::OR, 1, "OR", A, C);
    instrMap.emplace_back(0xb2, &CoreSM83::OR, 1, "OR", A, D);
    instrMap.emplace_back(0xb3, &CoreSM83::OR, 1, "OR", A, E);
    instrMap.emplace_back(0xb4, &CoreSM83::OR, 1, "OR", A, H);
    instrMap.emplace_back(0xb5, &CoreSM83::OR, 1, "OR", A, L);
    instrMap.emplace_back(0xb6, &CoreSM83::OR, 2, "OR", A, HL_ref);
    instrMap.emplace_back(0xb7, &CoreSM83::OR, 1, "OR", A, A);
    instrMap.emplace_back(0xb8, &CoreSM83::CP, 1, "CP", A, B);
    instrMap.emplace_back(0xb9, &CoreSM83::CP, 1, "CP", A, C);
    instrMap.emplace_back(0xba, &CoreSM83::CP, 1, "CP", A, D);
    instrMap.emplace_back(0xbb, &CoreSM83::CP, 1, "CP", A, E);
    instrMap.emplace_back(0xbc, &CoreSM83::CP, 1, "CP", A, H);
    instrMap.emplace_back(0xbd, &CoreSM83::CP, 1, "CP", A, L);
    instrMap.emplace_back(0xbe, &CoreSM83::CP, 2, "CP", A, HL_ref);
    instrMap.emplace_back(0xbf, &CoreSM83::CP, 1, "CP", A, A);

    // 0xc0
    instrMap.emplace_back(0xc0, &CoreSM83::RET, 0, "RET NZ", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xc1, &CoreSM83::POP, 3, "POP", BC, NO_DATA);
    instrMap.emplace_back(0xc2, &CoreSM83::JP, 0, "JP NZ", a16, NO_DATA);
    instrMap.emplace_back(0xc3, &CoreSM83::JP, 4, "JP", a16, NO_DATA);
    instrMap.emplace_back(0xc4, &CoreSM83::CALL, 0, "CALL NZ", a16, NO_DATA);
    instrMap.emplace_back(0xc5, &CoreSM83::PUSH, 4, "PUSH", BC, NO_DATA);
    instrMap.emplace_back(0xc6, &CoreSM83::ADD8, 2, "ADD", A, d8);
    instrMap.emplace_back(0xc7, &CoreSM83::RST, 4, "RST $00", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xc8, &CoreSM83::RET, 0, "RET Z", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xc9, &CoreSM83::RET, 4, "RET", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xca, &CoreSM83::JP, 0, "JP Z", a16, NO_DATA);
    instrMap.emplace_back(0xcb, &CoreSM83::CB, 1, "CB", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xcc, &CoreSM83::CALL, 0, "CALL Z", a16, NO_DATA);
    instrMap.emplace_back(0xcd, &CoreSM83::CALL, 6, "CALL", a16, NO_DATA);
    instrMap.emplace_back(0xce, &CoreSM83::ADC, 2, "ADC", A, d8);
    instrMap.emplace_back(0xcf, &CoreSM83::RST, 4, "RST $08", NO_DATA, NO_DATA);

    // 0xd0
    instrMap.emplace_back(0xd0, &CoreSM83::RET, 0, "RET NC", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xd1, &CoreSM83::POP, 3, "POP", DE, NO_DATA);
    instrMap.emplace_back(0xd2, &CoreSM83::JP, 0, "JP NC", a16, NO_DATA);
    instrMap.emplace_back(0xd3, &CoreSM83::NoInstruction, 0, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xd4, &CoreSM83::CALL, 0, "CALL NC", a16, NO_DATA);
    instrMap.emplace_back(0xd5, &CoreSM83::PUSH, 4, "PUSH", DE, NO_DATA);
    instrMap.emplace_back(0xd6, &CoreSM83::SUB, 2, "SUB", A, d8);
    instrMap.emplace_back(0xd7, &CoreSM83::RST, 4, "RST $10", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xd8, &CoreSM83::RET, 0, "RET C", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xd9, &CoreSM83::RETI, 4, "RETI", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xda, &CoreSM83::JP, 0, "JP C", a16, NO_DATA);
    instrMap.emplace_back(0xdb, &CoreSM83::NoInstruction, 0, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xdc, &CoreSM83::CALL, 0, "CALL C", a16, NO_DATA);
    instrMap.emplace_back(0xdd, &CoreSM83::NoInstruction, 0, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xde, &CoreSM83::SBC, 2, "SBC", A, d8);
    instrMap.emplace_back(0xdf, &CoreSM83::RST, 4, "RST $18", NO_DATA, NO_DATA);

    // 0xe0
    instrMap.emplace_back(0xe0, &CoreSM83::LDH, 3, "LDH", a8_ref, A);
    instrMap.emplace_back(0xe1, &CoreSM83::POP, 3, "POP", HL, NO_DATA);
    instrMap.emplace_back(0xe2, &CoreSM83::LDCref, 2, "LD", C_ref, A);
    instrMap.emplace_back(0xe3, &CoreSM83::NoInstruction, 0, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xe4, &CoreSM83::NoInstruction, 0, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xe5, &CoreSM83::PUSH, 4, "PUSH", HL, NO_DATA);
    instrMap.emplace_back(0xe6, &CoreSM83::AND, 2, "AND", A, d8);
    instrMap.emplace_back(0xe7, &CoreSM83::RST, 4, "RST $20", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xe8, &CoreSM83::ADDSPr8, 4, "ADD", SP, r8);
    instrMap.emplace_back(0xe9, &CoreSM83::JP, 1, "JP", HL_ref, NO_DATA);
    instrMap.emplace_back(0xea, &CoreSM83::LDHa16, 4, "LD", a16_ref, A);
    instrMap.emplace_back(0xeb, &CoreSM83::NoInstruction, 0, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xec, &CoreSM83::NoInstruction, 0, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xed, &CoreSM83::NoInstruction, 0, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xee, &CoreSM83::XOR, 2, "XOR", A, d8);
    instrMap.emplace_back(0xef, &CoreSM83::RST, 4, "RST $28", NO_DATA, NO_DATA);

    // 0xf0
    instrMap.emplace_back(0xf0, &CoreSM83::LDH, 3, "LD", A, a8_ref);
    instrMap.emplace_back(0xf1, &CoreSM83::POP, 3, "POP", AF, NO_DATA);
    instrMap.emplace_back(0xf2, &CoreSM83::LDCref, 2, "LD", A, C_ref);
    instrMap.emplace_back(0xf3, &CoreSM83::DI, 1, "DI", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xf4, &CoreSM83::NoInstruction, 0, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xf5, &CoreSM83::PUSH, 4, "PUSH", AF, NO_DATA);
    instrMap.emplace_back(0xf6, &CoreSM83::OR, 2, "OR", A, d8);
    instrMap.emplace_back(0xf7, &CoreSM83::RST, 4, "RST $30", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xf8, &CoreSM83::LDHLSPr8, 3, "LD", HL, SP_r8);
    instrMap.emplace_back(0xf9, &CoreSM83::LDSPHL, 2, "LD", SP, HL);
    instrMap.emplace_back(0xfa, &CoreSM83::LDHa16, 4, "LD", A, a16_ref);
    instrMap.emplace_back(0xfb, &CoreSM83::EI, 1, "EI", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xfc, &CoreSM83::NoInstruction, 0, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xfd, &CoreSM83::NoInstruction, 0, "NoInstruction", NO_DATA, NO_DATA);
    instrMap.emplace_back(0xfe, &CoreSM83::CP, 2, "CP", A, d8);
    instrMap.emplace_back(0xff, &CoreSM83::RST, 4, "RST $38", NO_DATA, NO_DATA);
}

/* ***********************************************************************************************************
*
*   GAMEBOY (COLOR) CORESM83 BASIC INSTRUCTION SET
*
*********************************************************************************************************** */

/* ***********************************************************************************************************
    CPU CONTROL
*********************************************************************************************************** */
// no instruction
void CoreSM83::NoInstruction(){
    return;
}

// no operation
void CoreSM83::NOP() {
    return;
}

// halted
void CoreSM83::STOP() {
    data = mmu_instance->Read8Bit(Regs.PC);
    Regs.PC++;

    if (data != 0x00) { 
        LOG_ERROR("Wrong usage of STOP instruction"); 
        return;
    }

    mmu_instance->Write8Bit(0x00, DIV_ADDR);
    halted = true;
}

// set halted state
void CoreSM83::HALT() {
    halted = true;
}

// flip c
void CoreSM83::CCF() {
    RESET_FLAGS(FLAG_HCARRY | FLAG_SUB, Regs.F);
    Regs.F ^= FLAG_CARRY;
}

// set c
void CoreSM83::SCF() {
    RESET_FLAGS(FLAG_HCARRY | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_CARRY;
}

// disable interrupts
void CoreSM83::DI() {
    ime = false;
}

// eneable interrupts
void CoreSM83::EI() {
    ime = true;
}

// enable CB sm83_instructions for next opcode
void CoreSM83::CB() {
    opcodeCB = true;
}

/* ***********************************************************************************************************
    LOAD
*********************************************************************************************************** */
// load 8/16 bit
void CoreSM83::LDfromAtoRef() {
    switch (opcode) {
    case 0x02:
        mmu_instance->Write8Bit(Regs.A, Regs.BC);
        break;
    case 0x12:
        mmu_instance->Write8Bit(Regs.A, Regs.DE);
        break;
    case 0x22:
        mmu_instance->Write8Bit(Regs.A, Regs.HL);
        Regs.HL++;
        break;
    case 0x32:
        Regs.HL--;
        mmu_instance->Write8Bit(Regs.A, Regs.HL);
        break;
    }
}

void CoreSM83::LDtoAfromRef() {
    switch (opcode) {
    case 0x0A:
        Regs.A = mmu_instance->Read8Bit(Regs.BC);
        break;
    case 0x1A:
        Regs.A = mmu_instance->Read8Bit(Regs.DE);
        break;
    case 0x2A:
        Regs.A = mmu_instance->Read8Bit(Regs.HL);
        Regs.HL++;
        break;
    case 0x3A:
        Regs.HL--;
        Regs.A = mmu_instance->Read8Bit(Regs.HL);
        break;
    }
}

void CoreSM83::LDd8() {
    data = mmu_instance->Read8Bit(Regs.PC);
    Regs.PC++;

    switch (opcode) {
    case 0x06:
        Regs.BC_.B = data;
        break;
    case 0x16:
        Regs.DE_.D = data;
        break;
    case 0x26:
        Regs.HL_.H = data;
        break;
    case 0x36:
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x0E:
        Regs.BC_.C = data;
        break;
    case 0x1E:
        Regs.DE_.E = data;
        break;
    case 0x2E:
        Regs.HL_.L = data;
        break;
    case 0x3E:
        Regs.A = data;
        break;
    }
}

void CoreSM83::LDd16() {
    data = mmu_instance->Read16Bit(Regs.PC);
    Regs.PC += 2;

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

void CoreSM83::LDCref() {
    switch (opcode) {
    case 0xE2:
        mmu_instance->Write8Bit(Regs.A, Regs.BC_.C | 0xFF00);
        break;
    case 0xF2:
        Regs.A = mmu_instance->Read8Bit(Regs.BC_.C | 0xFF00);
        break;
    }
}

void CoreSM83::LDH() {
    data = mmu_instance->Read8Bit(Regs.PC);
    Regs.PC++;

    switch (opcode) {
    case 0xE0:
        mmu_instance->Write8Bit(Regs.A, (data & 0xFF) | 0xFF00);
        break;
    case 0xF0:
        Regs.A = mmu_instance->Read8Bit((data & 0xFF) | 0xFF00);
        break;
    }
}

void CoreSM83::LDHa16() {
    data = mmu_instance->Read16Bit(Regs.PC);
    Regs.PC += 2;

    switch (opcode) {
    case 0xea:
        mmu_instance->Write8Bit(Regs.A, data);
        break;
    case 0xfa:
        Regs.A = mmu_instance->Read8Bit(data);
        break;
    }
}

void CoreSM83::LDSPa16() {
    data = mmu_instance->Read16Bit(Regs.PC);
    Regs.PC += 2;

    mmu_instance->Write16Bit(Regs.SP, data);
}

void CoreSM83::LDtoB() {
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
        Regs.BC_.B = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.BC_.B = Regs.A;
        break;
    }
}

void CoreSM83::LDtoC() {
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
        Regs.BC_.C = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.BC_.C = Regs.A;
        break;
    }
}

void CoreSM83::LDtoD() {
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
        Regs.DE_.D = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.DE_.D = Regs.A;
        break;
    }
}

void CoreSM83::LDtoE() {
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
        Regs.DE_.E = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.DE_.E = Regs.A;
        break;
    }
}

void CoreSM83::LDtoH() {
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
        Regs.HL_.H = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.HL_.H = Regs.A;
        break;
    }
}

void CoreSM83::LDtoL() {
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
        Regs.HL_.L = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.HL_.L = Regs.A;
        break;
    }
}

void CoreSM83::LDtoHLref() {
    switch (opcode & 0x07) {
    case 0x00:
        mmu_instance->Write8Bit(Regs.BC_.B, Regs.HL);
        break;
    case 0x01:
        mmu_instance->Write8Bit(Regs.BC_.C, Regs.HL);
        break;
    case 0x02:
        mmu_instance->Write8Bit(Regs.DE_.D, Regs.HL);
        break;
    case 0x03:
        mmu_instance->Write8Bit(Regs.DE_.E, Regs.HL);
        break;
    case 0x04:
        mmu_instance->Write8Bit(Regs.HL_.H, Regs.HL);
        break;
    case 0x05:
        mmu_instance->Write8Bit(Regs.HL_.L, Regs.HL);
        break;
    case 0x07:
        mmu_instance->Write8Bit(Regs.A, Regs.HL);
        break;
    }
}

void CoreSM83::LDtoA() {
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
        Regs.A = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.A = Regs.A;
        break;
    }
}

// ld SP to HL and add signed 8 bit immediate data
void CoreSM83::LDHLSPr8() {
    data = mmu_instance->Read8Bit(Regs.PC);
    Regs.PC++;

    Regs.HL = Regs.SP;
    RESET_ALL_FLAGS(Regs.F);
    ADD_8_FLAGS(Regs.HL, data, Regs.F);
    Regs.HL += *(i8*)&data;
}

void CoreSM83::LDSPHL() {
    Regs.SP = Regs.HL;
}

// push PC to Stack
void CoreSM83::PUSH() {
    Regs.SP -= 2;

    switch (opcode) {
    case 0xc5:
        mmu_instance->Write16Bit(Regs.BC, Regs.SP);
        break;
    case 0xd5:
        mmu_instance->Write16Bit(Regs.DE, Regs.SP);
        break;
    case 0xe5:
        mmu_instance->Write16Bit(Regs.HL, Regs.SP);
        break;
    case 0xf5:
        data = ((u16)Regs.A << 8) | Regs.F;
        mmu_instance->Write16Bit(data, Regs.SP);
        break;
    }
}

void CoreSM83::isr_push(const u8& _isr_handler) {
    Regs.SP -= 2;
    mmu_instance->Write16Bit(Regs.PC, Regs.SP);
    Regs.PC = _isr_handler;
}

// pop PC from Stack
void CoreSM83::POP() {
    switch (opcode) {
    case 0xc1:
        Regs.BC = mmu_instance->Read16Bit(Regs.SP);
        break;
    case 0xd1:
        Regs.DE = mmu_instance->Read16Bit(Regs.SP);
        break;
    case 0xe1:
        Regs.HL = mmu_instance->Read16Bit(Regs.SP);
        break;
    case 0xf1:
        data = mmu_instance->Read16Bit(Regs.SP);
        Regs.A = (data & 0xFF00) >> 8;
        Regs.F = data & 0xFF;
        break;
    }

    Regs.SP += 2;
}

/* ***********************************************************************************************************
    ARITHMETIC/LOGIC INSTRUCTIONS
*********************************************************************************************************** */
// increment 8/16 bit
void CoreSM83::INC8() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        ADD_8_HC(data, 1, Regs.F);
        data += 1;
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x3c:
        ADD_8_HC(Regs.A, 1, Regs.F);
        Regs.A += 1;
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

void CoreSM83::INC16() {
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
}

// decrement 8/16 bit
void CoreSM83::DEC8() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        SUB_8_HC(data, 1, Regs.F);
        data -= 1;
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x3d:
        SUB_8_HC(Regs.A, 1, Regs.F);
        Regs.A -= 1;
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

void CoreSM83::DEC16() {
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
}

// add with carry + halfcarry
void CoreSM83::ADD8() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x87:
        data = Regs.A;
        break;
    case 0xc6:
        data = mmu_instance->Read8Bit(Regs.PC);
        Regs.PC++;
        break;
    }

    ADD_8_FLAGS(Regs.A, data, Regs.F);
    Regs.A += data;
    ZERO_FLAG(Regs.A, Regs.F);
}

void CoreSM83::ADDHL() {
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
}

// add for SP + signed immediate data r8
void CoreSM83::ADDSPr8() {
    data = mmu_instance->Read8Bit(Regs.PC);
    Regs.PC++;

    RESET_ALL_FLAGS(Regs.F);
    ADD_8_FLAGS(Regs.SP, data, Regs.F);
    Regs.SP += *(i8*)&data;
}

// adc for A + (register or immediate unsigned data d8) + carry
void CoreSM83::ADC() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x8f:
        data = Regs.A;
        break;
    case 0xce:
        data = mmu_instance->Read8Bit(Regs.PC);
        Regs.PC++;
        break;
    }

    ADC_8_HC(Regs.A, data, carry, Regs.F);
    ADC_8_C(Regs.A, data, carry, Regs.F);
    Regs.A += data + carry;
    ZERO_FLAG(Regs.A, Regs.F);
}

// sub with carry + halfcarry
void CoreSM83::SUB() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x97:
        data = Regs.A;
        break;
    case 0xd6:
        data = mmu_instance->Read8Bit(Regs.PC);
        Regs.PC++;
        break;
    }

    SUB_8_FLAGS(Regs.A, data, Regs.F);
    Regs.A -= data;
    ZERO_FLAG(Regs.A, Regs.F);
}

// adc for A + (register or immediate unsigned data d8) + carry
void CoreSM83::SBC() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x9f:
        data = Regs.A;
        break;
    case 0xde:
        data = mmu_instance->Read8Bit(Regs.PC);
        Regs.PC++;
        break;
    }

    SBC_8_HC(Regs.A, data, carry, Regs.F);
    SBC_8_C(Regs.A, data, carry, Regs.F);
    Regs.A -= (data + carry);
    ZERO_FLAG(Regs.A, Regs.F);
}

// bcd code addition and subtraction
void CoreSM83::DAA() {
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
void CoreSM83::AND() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0xa7:
        data = Regs.A;
        break;
    case 0xe6:
        data = mmu_instance->Read8Bit(Regs.PC);
        Regs.PC++;
        break;
    }

    Regs.A &= data;
    ZERO_FLAG(Regs.A, Regs.F);
}

// or with z
void CoreSM83::OR() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0xb7:
        data = Regs.A;
        break;
    case 0xf6:
        data = mmu_instance->Read8Bit(Regs.PC);
        Regs.PC++;
        break;
    }

    Regs.A |= data;
    ZERO_FLAG(Regs.A, Regs.F);
}

// xor with z
void CoreSM83::XOR() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0xaf:
        data = Regs.A;
        break;
    case 0xee:
        data = mmu_instance->Read8Bit(Regs.PC);
        Regs.PC++;
        break;
    }

    Regs.A ^= data;
    ZERO_FLAG(Regs.A, Regs.F);
}

// compare (subtraction)
void CoreSM83::CP() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0xbf:
        data = Regs.A;
        break;
    case 0xfe:
        data = mmu_instance->Read8Bit(Regs.PC);
        Regs.PC++;
        break;
    }

    SUB_8_FLAGS(Regs.A, data, Regs.F);
    Regs.A -= data;
    ZERO_FLAG(Regs.A, Regs.F);
}

// 1's complement of A
void CoreSM83::CPL() {
    SET_FLAGS(FLAG_SUB | FLAG_HCARRY, Regs.F);
    Regs.A = ~(Regs.A);
}

/* ***********************************************************************************************************
    ROTATE AND SHIFT
*********************************************************************************************************** */
// rotate A left with carry
void CoreSM83::RLCA() {
    RESET_ALL_FLAGS(Regs.F);
    Regs.F |= (Regs.A & MSB ? FLAG_CARRY : 0x00);
    Regs.A <<= 1;
    Regs.A |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
}

// rotate A right with carry
void CoreSM83::RRCA() {
    RESET_ALL_FLAGS(Regs.F);
    Regs.F |= (Regs.A & LSB ? FLAG_CARRY : 0x00);
    Regs.A >>= 1;
    Regs.A |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
}

// rotate A left through carry
void CoreSM83::RLA() {
    static bool carry = Regs.F & FLAG_CARRY;
    RESET_ALL_FLAGS(Regs.F);
    Regs.F |= (Regs.A & MSB ? FLAG_CARRY : 0x00);
    Regs.A <<= 1;
    Regs.A |= (carry ? LSB : 0x00);
}

// rotate A right through carry
void CoreSM83::RRA() {
    static bool carry = Regs.F & FLAG_CARRY;
    RESET_ALL_FLAGS(Regs.F);
    Regs.F |= (Regs.A & LSB ? FLAG_CARRY : 0x00);
    Regs.A >>= 1;
    Regs.A |= (carry ? MSB : 0x00);
}

/* ***********************************************************************************************************
    JUMP INSTRUCTIONS
*********************************************************************************************************** */
// jump to memory location
void CoreSM83::JP() {
    if (opcode == 0xe9) {
        data = Regs.HL;
        jump_jp();
        return;
    }

    static bool carry;
    static bool zero;

    data = mmu_instance->Read16Bit(Regs.PC);
    Regs.PC += 2;

    switch (opcode) {
    case 0xCA:
        zero = Regs.F & FLAG_ZERO;
        if (zero) { 
            jump_jp(); 
            currentMachineCycles += 4;
            return;
        }
        break;
    case 0xC2:
        zero = Regs.F & FLAG_ZERO;
        if (!zero) { 
            jump_jp(); 
            currentMachineCycles += 4;
            return;
        }
        break;
    case 0xDA:
        carry = Regs.F & FLAG_CARRY;
        if (carry) { 
            jump_jp(); 
            currentMachineCycles += 4;
            return;
        }
        break;
    case 0xD2:
        carry = Regs.F & FLAG_CARRY;
        if (!carry) { 
            jump_jp(); 
            currentMachineCycles += 4;
            return;
        }
        break;
    case 0xC3:
        jump_jp();
        return;
        break;
    }
    currentMachineCycles += 3;
}

// jump relative to memory lecation
void CoreSM83::JR() {
    static bool carry;
    static bool zero;

    data = mmu_instance->Read8Bit(Regs.PC);
    Regs.PC += 1;

    switch (opcode) {
    case 0x28:
        zero = Regs.F & FLAG_ZERO;
        if (zero) { 
            jump_jr(); 
            currentMachineCycles += 3;
            return;
        }
        break;
    case 0x20:
        zero = Regs.F & FLAG_ZERO;
        if (!zero) { 
            jump_jr(); 
            currentMachineCycles += 3;
            return;
        }
        break;
    case 0x38:
        carry = Regs.F & FLAG_CARRY;
        if (carry) { 
            jump_jr(); 
            currentMachineCycles += 3;
            return;
        }
        break;
    case 0x30:
        carry = Regs.F & FLAG_CARRY;
        if (!carry) { 
            jump_jr();
            currentMachineCycles += 3;
            return;
        }
        break;
    default:
        jump_jr();
        break;
    }
    currentMachineCycles += 2;
}

void CoreSM83::jump_jp() {
    Regs.PC = data;
}

void CoreSM83::jump_jr() {
    Regs.PC += *(i8*)&data;
}

// call routine at memory location
void CoreSM83::CALL() {
    static bool carry;
    static bool zero;

    data = mmu_instance->Read16Bit(Regs.PC);
    Regs.PC += 2;

    switch (opcode) {
    case 0xCC:
        zero = Regs.F & FLAG_ZERO;
        if (zero) { 
            call(); 
            currentMachineCycles += 6;
            return;
        }
        break;
    case 0xC4:
        zero = Regs.F & FLAG_ZERO;
        if (!zero) { 
            call(); 
            currentMachineCycles += 6;
            return;
        }
        break;
    case 0xDC:
        carry = Regs.F & FLAG_CARRY;
        if (carry) { 
            call(); 
            currentMachineCycles += 6;
            return;
        }
        break;
    case 0xD4:
        carry = Regs.F & FLAG_CARRY;
        if (!carry) { 
            call(); 
            currentMachineCycles += 6;
            return;
        }
        break;
    default:
        call();
        currentMachineCycles += 6;
        return;
        break;
    }
    currentMachineCycles += 3;
}

// call to interrupt vectors
void CoreSM83::RST() {
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

void CoreSM83::call() {
    Regs.SP -= 2;
    mmu_instance->Write16Bit(Regs.PC, Regs.SP);
    Regs.PC = data;
}

// return from routine
void CoreSM83::RET() {
    static bool carry;
    static bool zero;

    switch (opcode) {
    case 0xC8:
        zero = Regs.F & FLAG_ZERO;
        if (zero) { 
            ret(); 
            currentMachineCycles += 5;
            return;
        }
        break;
    case 0xC0:
        zero = Regs.F & FLAG_ZERO;
        if (!zero) { 
            ret(); 
            currentMachineCycles += 5;
            return;
        }
        break;
    case 0xD8:
        carry = Regs.F & FLAG_CARRY;
        if (carry) { 
            ret(); 
            currentMachineCycles += 5;
            return;
        }
        break;
    case 0xD0:
        carry = Regs.F & FLAG_CARRY;
        if (!carry) { 
            ret(); 
            currentMachineCycles += 5;
            return;
        }
        break;
    default:
        ret();
        currentMachineCycles += 4;
        return;
        break;
    }
    currentMachineCycles += 2;
}

// return and enable interrupts
void CoreSM83::RETI() {
    ret();
    ime = true;
}

void CoreSM83::ret() {
    Regs.PC = mmu_instance->Read16Bit(Regs.SP);
    Regs.SP += 2;
}

/* ***********************************************************************************************************
*
*   GAMEBOY (COLOR) CORESM83 CB INSTRUCTION LOOKUP TABLE
*
*********************************************************************************************************** */
void CoreSM83::setupLookupTableCB() {
    instrMapCB.clear();

    // Elements: opcode, instruction function, machine cycles
    instrMapCB.emplace_back(0x00, &CoreSM83::RLC, 2, "RLC", B,     NO_DATA);
    instrMapCB.emplace_back(0x01, &CoreSM83::RLC, 2, "RLC", C,     NO_DATA);
    instrMapCB.emplace_back(0x02, &CoreSM83::RLC, 2, "RLC", D,     NO_DATA);
    instrMapCB.emplace_back(0x03, &CoreSM83::RLC, 2, "RLC", E,     NO_DATA);
    instrMapCB.emplace_back(0x04, &CoreSM83::RLC, 2, "RLC", H,     NO_DATA);
    instrMapCB.emplace_back(0x05, &CoreSM83::RLC, 2, "RLC", L,     NO_DATA);
    instrMapCB.emplace_back(0x06, &CoreSM83::RLC, 4, "RLC", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x07, &CoreSM83::RLC, 2, "RLC", A,     NO_DATA);
    instrMapCB.emplace_back(0x08, &CoreSM83::RRC, 2, "RRC", B,     NO_DATA);
    instrMapCB.emplace_back(0x09, &CoreSM83::RRC, 2, "RRC", C,     NO_DATA);
    instrMapCB.emplace_back(0x0a, &CoreSM83::RRC, 2, "RRC", D,     NO_DATA);
    instrMapCB.emplace_back(0x0b, &CoreSM83::RRC, 2, "RRC", E,     NO_DATA);
    instrMapCB.emplace_back(0x0c, &CoreSM83::RRC, 2, "RRC", H,     NO_DATA);
    instrMapCB.emplace_back(0x0d, &CoreSM83::RRC, 2, "RRC", L,     NO_DATA);
    instrMapCB.emplace_back(0x0e, &CoreSM83::RRC, 4, "RRC", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x0f, &CoreSM83::RRC, 2, "RRC", A,     NO_DATA);

    instrMapCB.emplace_back(0x10, &CoreSM83::RL, 2, "RL", B,     NO_DATA);
    instrMapCB.emplace_back(0x11, &CoreSM83::RL, 2, "RL", C,     NO_DATA);
    instrMapCB.emplace_back(0x12, &CoreSM83::RL, 2, "RL", D,     NO_DATA);
    instrMapCB.emplace_back(0x13, &CoreSM83::RL, 2, "RL", E,     NO_DATA);
    instrMapCB.emplace_back(0x14, &CoreSM83::RL, 2, "RL", H,     NO_DATA);
    instrMapCB.emplace_back(0x15, &CoreSM83::RL, 2, "RL", L,     NO_DATA);
    instrMapCB.emplace_back(0x16, &CoreSM83::RL, 4, "RL", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x17, &CoreSM83::RL, 2, "RL", A,     NO_DATA);
    instrMapCB.emplace_back(0x18, &CoreSM83::RR, 2, "RR", B,     NO_DATA);
    instrMapCB.emplace_back(0x19, &CoreSM83::RR, 2, "RR", C,     NO_DATA);
    instrMapCB.emplace_back(0x1a, &CoreSM83::RR, 2, "RR", D,     NO_DATA);
    instrMapCB.emplace_back(0x1b, &CoreSM83::RR, 2, "RR", E,     NO_DATA);
    instrMapCB.emplace_back(0x1c, &CoreSM83::RR, 2, "RR", H,     NO_DATA);
    instrMapCB.emplace_back(0x1d, &CoreSM83::RR, 2, "RR", L,     NO_DATA);
    instrMapCB.emplace_back(0x1e, &CoreSM83::RR, 4, "RR", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x1f, &CoreSM83::RR, 2, "RR", A,     NO_DATA);

    instrMapCB.emplace_back(0x20, &CoreSM83::SLA, 2, "SLA", B,     NO_DATA);
    instrMapCB.emplace_back(0x21, &CoreSM83::SLA, 2, "SLA", C,     NO_DATA);
    instrMapCB.emplace_back(0x22, &CoreSM83::SLA, 2, "SLA", D,     NO_DATA);
    instrMapCB.emplace_back(0x23, &CoreSM83::SLA, 2, "SLA", E,     NO_DATA);
    instrMapCB.emplace_back(0x24, &CoreSM83::SLA, 2, "SLA", H,     NO_DATA);
    instrMapCB.emplace_back(0x25, &CoreSM83::SLA, 2, "SLA", L,     NO_DATA);
    instrMapCB.emplace_back(0x26, &CoreSM83::SLA, 4, "SLA", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x27, &CoreSM83::SLA, 2, "SLA", A,     NO_DATA);
    instrMapCB.emplace_back(0x28, &CoreSM83::SRA, 2, "SRA", B,     NO_DATA);
    instrMapCB.emplace_back(0x29, &CoreSM83::SRA, 2, "SRA", C,     NO_DATA);
    instrMapCB.emplace_back(0x2a, &CoreSM83::SRA, 2, "SRA", D,     NO_DATA);
    instrMapCB.emplace_back(0x2b, &CoreSM83::SRA, 2, "SRA", E,     NO_DATA);
    instrMapCB.emplace_back(0x2c, &CoreSM83::SRA, 2, "SRA", H,     NO_DATA);
    instrMapCB.emplace_back(0x2d, &CoreSM83::SRA, 2, "SRA", L,     NO_DATA);
    instrMapCB.emplace_back(0x2e, &CoreSM83::SRA, 4, "SRA", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x2f, &CoreSM83::SRA, 2, "SRA", A,     NO_DATA);

    instrMapCB.emplace_back(0x30, &CoreSM83::SWAP, 2, "SWAP", B,     NO_DATA);
    instrMapCB.emplace_back(0x31, &CoreSM83::SWAP, 2, "SWAP", C,     NO_DATA);
    instrMapCB.emplace_back(0x32, &CoreSM83::SWAP, 2, "SWAP", D,     NO_DATA);
    instrMapCB.emplace_back(0x33, &CoreSM83::SWAP, 2, "SWAP", E,     NO_DATA);
    instrMapCB.emplace_back(0x34, &CoreSM83::SWAP, 2, "SWAP", H,     NO_DATA);
    instrMapCB.emplace_back(0x35, &CoreSM83::SWAP, 2, "SWAP", L,     NO_DATA);
    instrMapCB.emplace_back(0x36, &CoreSM83::SWAP, 4, "SWAP", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x37, &CoreSM83::SWAP, 2, "SWAP", A,     NO_DATA);
    instrMapCB.emplace_back(0x38, &CoreSM83::SRL, 2, "SRL", B,     NO_DATA);
    instrMapCB.emplace_back(0x39, &CoreSM83::SRL, 2, "SRL", C,     NO_DATA);
    instrMapCB.emplace_back(0x3a, &CoreSM83::SRL, 2, "SRL", D,     NO_DATA);
    instrMapCB.emplace_back(0x3b, &CoreSM83::SRL, 2, "SRL", E,     NO_DATA);
    instrMapCB.emplace_back(0x3c, &CoreSM83::SRL, 2, "SRL", H,     NO_DATA);
    instrMapCB.emplace_back(0x3d, &CoreSM83::SRL, 2, "SRL", L,     NO_DATA);
    instrMapCB.emplace_back(0x3e, &CoreSM83::SRL, 4, "SRL", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x3f, &CoreSM83::SRL, 2, "SRL", A,     NO_DATA);

    instrMapCB.emplace_back(0x40, &CoreSM83::BIT0, 2, "BIT0", B,     NO_DATA);
    instrMapCB.emplace_back(0x41, &CoreSM83::BIT0, 2, "BIT0", C,     NO_DATA);
    instrMapCB.emplace_back(0x42, &CoreSM83::BIT0, 2, "BIT0", D,     NO_DATA);
    instrMapCB.emplace_back(0x43, &CoreSM83::BIT0, 2, "BIT0", E,     NO_DATA);
    instrMapCB.emplace_back(0x44, &CoreSM83::BIT0, 2, "BIT0", H,     NO_DATA);
    instrMapCB.emplace_back(0x45, &CoreSM83::BIT0, 2, "BIT0", L,     NO_DATA);
    instrMapCB.emplace_back(0x46, &CoreSM83::BIT0, 4, "BIT0", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x47, &CoreSM83::BIT0, 2, "BIT0", A,     NO_DATA);
    instrMapCB.emplace_back(0x48, &CoreSM83::BIT1, 2, "BIT1", B,     NO_DATA);
    instrMapCB.emplace_back(0x49, &CoreSM83::BIT1, 2, "BIT1", C,     NO_DATA);
    instrMapCB.emplace_back(0x4a, &CoreSM83::BIT1, 2, "BIT1", D,     NO_DATA);
    instrMapCB.emplace_back(0x4b, &CoreSM83::BIT1, 2, "BIT1", E,     NO_DATA);
    instrMapCB.emplace_back(0x4c, &CoreSM83::BIT1, 2, "BIT1", H,     NO_DATA);
    instrMapCB.emplace_back(0x4d, &CoreSM83::BIT1, 2, "BIT1", L,     NO_DATA);
    instrMapCB.emplace_back(0x4e, &CoreSM83::BIT1, 4, "BIT1", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x4f, &CoreSM83::BIT1, 2, "BIT1", A,     NO_DATA);

    instrMapCB.emplace_back(0x50, &CoreSM83::BIT2, 2, "BIT2", B,     NO_DATA);
    instrMapCB.emplace_back(0x51, &CoreSM83::BIT2, 2, "BIT2", C,     NO_DATA);
    instrMapCB.emplace_back(0x52, &CoreSM83::BIT2, 2, "BIT2", D,     NO_DATA);
    instrMapCB.emplace_back(0x53, &CoreSM83::BIT2, 2, "BIT2", E,     NO_DATA);
    instrMapCB.emplace_back(0x54, &CoreSM83::BIT2, 2, "BIT2", H,     NO_DATA);
    instrMapCB.emplace_back(0x55, &CoreSM83::BIT2, 2, "BIT2", L,     NO_DATA);
    instrMapCB.emplace_back(0x56, &CoreSM83::BIT2, 4, "BIT2", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x57, &CoreSM83::BIT2, 2, "BIT2", A,     NO_DATA);
    instrMapCB.emplace_back(0x58, &CoreSM83::BIT3, 2, "BIT3", B,     NO_DATA);
    instrMapCB.emplace_back(0x59, &CoreSM83::BIT3, 2, "BIT3", C,     NO_DATA);
    instrMapCB.emplace_back(0x5a, &CoreSM83::BIT3, 2, "BIT3", D,     NO_DATA);
    instrMapCB.emplace_back(0x5b, &CoreSM83::BIT3, 2, "BIT3", E,     NO_DATA);
    instrMapCB.emplace_back(0x5c, &CoreSM83::BIT3, 2, "BIT3", H,     NO_DATA);
    instrMapCB.emplace_back(0x5d, &CoreSM83::BIT3, 2, "BIT3", L,     NO_DATA);
    instrMapCB.emplace_back(0x5e, &CoreSM83::BIT3, 4, "BIT3", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x5f, &CoreSM83::BIT3, 2, "BIT3", A,     NO_DATA);

    instrMapCB.emplace_back(0x60, &CoreSM83::BIT4, 2, "BIT4", B,     NO_DATA);
    instrMapCB.emplace_back(0x61, &CoreSM83::BIT4, 2, "BIT4", C,     NO_DATA);
    instrMapCB.emplace_back(0x62, &CoreSM83::BIT4, 2, "BIT4", D,     NO_DATA);
    instrMapCB.emplace_back(0x63, &CoreSM83::BIT4, 2, "BIT4", E,     NO_DATA);
    instrMapCB.emplace_back(0x64, &CoreSM83::BIT4, 2, "BIT4", H,     NO_DATA);
    instrMapCB.emplace_back(0x65, &CoreSM83::BIT4, 2, "BIT4", L,     NO_DATA);
    instrMapCB.emplace_back(0x66, &CoreSM83::BIT4, 4, "BIT4", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x67, &CoreSM83::BIT4, 2, "BIT4", A,     NO_DATA);
    instrMapCB.emplace_back(0x68, &CoreSM83::BIT5, 2, "BIT5", B,     NO_DATA);
    instrMapCB.emplace_back(0x69, &CoreSM83::BIT5, 2, "BIT5", C,     NO_DATA);
    instrMapCB.emplace_back(0x6a, &CoreSM83::BIT5, 2, "BIT5", D,     NO_DATA);
    instrMapCB.emplace_back(0x6b, &CoreSM83::BIT5, 2, "BIT5", E,     NO_DATA);
    instrMapCB.emplace_back(0x6c, &CoreSM83::BIT5, 2, "BIT5", H,     NO_DATA);
    instrMapCB.emplace_back(0x6d, &CoreSM83::BIT5, 2, "BIT5", L,     NO_DATA);
    instrMapCB.emplace_back(0x6e, &CoreSM83::BIT5, 4, "BIT5", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x6f, &CoreSM83::BIT5, 2, "BIT5", A,     NO_DATA);

    instrMapCB.emplace_back(0x70, &CoreSM83::BIT6, 2, "BIT6", B,     NO_DATA);
    instrMapCB.emplace_back(0x71, &CoreSM83::BIT6, 2, "BIT6", C,     NO_DATA);
    instrMapCB.emplace_back(0x72, &CoreSM83::BIT6, 2, "BIT6", D,     NO_DATA);
    instrMapCB.emplace_back(0x73, &CoreSM83::BIT6, 2, "BIT6", E,     NO_DATA);
    instrMapCB.emplace_back(0x74, &CoreSM83::BIT6, 2, "BIT6", H,     NO_DATA);
    instrMapCB.emplace_back(0x75, &CoreSM83::BIT6, 2, "BIT6", L,     NO_DATA);
    instrMapCB.emplace_back(0x76, &CoreSM83::BIT6, 4, "BIT6", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x77, &CoreSM83::BIT6, 2, "BIT6", A,     NO_DATA);
    instrMapCB.emplace_back(0x78, &CoreSM83::BIT7, 2, "BIT7", B,     NO_DATA);
    instrMapCB.emplace_back(0x79, &CoreSM83::BIT7, 2, "BIT7", C,     NO_DATA);
    instrMapCB.emplace_back(0x7a, &CoreSM83::BIT7, 2, "BIT7", D,     NO_DATA);
    instrMapCB.emplace_back(0x7b, &CoreSM83::BIT7, 2, "BIT7", E,     NO_DATA);
    instrMapCB.emplace_back(0x7c, &CoreSM83::BIT7, 2, "BIT7", H,     NO_DATA);
    instrMapCB.emplace_back(0x7d, &CoreSM83::BIT7, 2, "BIT7", L,     NO_DATA);
    instrMapCB.emplace_back(0x7e, &CoreSM83::BIT7, 4, "BIT7", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x7f, &CoreSM83::BIT7, 2, "BIT7", A,     NO_DATA);

    instrMapCB.emplace_back(0x80, &CoreSM83::RES0, 2, "RES0", B,     NO_DATA);
    instrMapCB.emplace_back(0x81, &CoreSM83::RES0, 2, "RES0", C,     NO_DATA);
    instrMapCB.emplace_back(0x82, &CoreSM83::RES0, 2, "RES0", D,     NO_DATA);
    instrMapCB.emplace_back(0x83, &CoreSM83::RES0, 2, "RES0", E,     NO_DATA);
    instrMapCB.emplace_back(0x84, &CoreSM83::RES0, 2, "RES0", H,     NO_DATA);
    instrMapCB.emplace_back(0x85, &CoreSM83::RES0, 2, "RES0", L,     NO_DATA);
    instrMapCB.emplace_back(0x86, &CoreSM83::RES0, 4, "RES0", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x87, &CoreSM83::RES0, 2, "RES0", A,     NO_DATA);
    instrMapCB.emplace_back(0x88, &CoreSM83::RES1, 2, "RES1", B,     NO_DATA);
    instrMapCB.emplace_back(0x89, &CoreSM83::RES1, 2, "RES1", C,     NO_DATA);
    instrMapCB.emplace_back(0x8a, &CoreSM83::RES1, 2, "RES1", D,     NO_DATA);
    instrMapCB.emplace_back(0x8b, &CoreSM83::RES1, 2, "RES1", E,     NO_DATA);
    instrMapCB.emplace_back(0x8c, &CoreSM83::RES1, 2, "RES1", H,     NO_DATA);
    instrMapCB.emplace_back(0x8d, &CoreSM83::RES1, 2, "RES1", L,     NO_DATA);
    instrMapCB.emplace_back(0x8e, &CoreSM83::RES1, 4, "RES1", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x8f, &CoreSM83::RES1, 2, "RES1", A,     NO_DATA);

    instrMapCB.emplace_back(0x90, &CoreSM83::RES2, 2, "RES2", B,     NO_DATA);
    instrMapCB.emplace_back(0x91, &CoreSM83::RES2, 2, "RES2", C,     NO_DATA);
    instrMapCB.emplace_back(0x92, &CoreSM83::RES2, 2, "RES2", D,     NO_DATA);
    instrMapCB.emplace_back(0x93, &CoreSM83::RES2, 2, "RES2", E,     NO_DATA);
    instrMapCB.emplace_back(0x94, &CoreSM83::RES2, 2, "RES2", H,     NO_DATA);
    instrMapCB.emplace_back(0x95, &CoreSM83::RES2, 2, "RES2", L,     NO_DATA);
    instrMapCB.emplace_back(0x96, &CoreSM83::RES2, 4, "RES2", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x97, &CoreSM83::RES2, 2, "RES2", A,     NO_DATA);
    instrMapCB.emplace_back(0x98, &CoreSM83::RES3, 2, "RES3", B,     NO_DATA);
    instrMapCB.emplace_back(0x99, &CoreSM83::RES3, 2, "RES3", C,     NO_DATA);
    instrMapCB.emplace_back(0x9a, &CoreSM83::RES3, 2, "RES3", D,     NO_DATA);
    instrMapCB.emplace_back(0x9b, &CoreSM83::RES3, 2, "RES3", E,     NO_DATA);
    instrMapCB.emplace_back(0x9c, &CoreSM83::RES3, 2, "RES3", H,     NO_DATA);
    instrMapCB.emplace_back(0x9d, &CoreSM83::RES3, 2, "RES3", L,     NO_DATA);
    instrMapCB.emplace_back(0x9e, &CoreSM83::RES3, 4, "RES3", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x9f, &CoreSM83::RES3, 2, "RES3", A,     NO_DATA);

    instrMapCB.emplace_back(0xa0, &CoreSM83::RES4, 2, "RES4", B,     NO_DATA);
    instrMapCB.emplace_back(0xa1, &CoreSM83::RES4, 2, "RES4", C,     NO_DATA);
    instrMapCB.emplace_back(0xa2, &CoreSM83::RES4, 2, "RES4", D,     NO_DATA);
    instrMapCB.emplace_back(0xa3, &CoreSM83::RES4, 2, "RES4", E,     NO_DATA);
    instrMapCB.emplace_back(0xa4, &CoreSM83::RES4, 2, "RES4", H,     NO_DATA);
    instrMapCB.emplace_back(0xa5, &CoreSM83::RES4, 2, "RES4", L,     NO_DATA);
    instrMapCB.emplace_back(0xa6, &CoreSM83::RES4, 4, "RES4", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xa7, &CoreSM83::RES4, 2, "RES4", A,     NO_DATA);
    instrMapCB.emplace_back(0xa8, &CoreSM83::RES5, 2, "RES5", B,     NO_DATA);
    instrMapCB.emplace_back(0xa9, &CoreSM83::RES5, 2, "RES5", C,     NO_DATA);
    instrMapCB.emplace_back(0xaa, &CoreSM83::RES5, 2, "RES5", D,     NO_DATA);
    instrMapCB.emplace_back(0xab, &CoreSM83::RES5, 2, "RES5", E,     NO_DATA);
    instrMapCB.emplace_back(0xac, &CoreSM83::RES5, 2, "RES5", H,     NO_DATA);
    instrMapCB.emplace_back(0xad, &CoreSM83::RES5, 2, "RES5", L,     NO_DATA);
    instrMapCB.emplace_back(0xae, &CoreSM83::RES5, 4, "RES5", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xaf, &CoreSM83::RES5, 2, "RES5", A,     NO_DATA);

    instrMapCB.emplace_back(0xb0, &CoreSM83::RES6, 2, "RES6", B,     NO_DATA);
    instrMapCB.emplace_back(0xb1, &CoreSM83::RES6, 2, "RES6", C,     NO_DATA);
    instrMapCB.emplace_back(0xb2, &CoreSM83::RES6, 2, "RES6", D,     NO_DATA);
    instrMapCB.emplace_back(0xb3, &CoreSM83::RES6, 2, "RES6", E,     NO_DATA);
    instrMapCB.emplace_back(0xb4, &CoreSM83::RES6, 2, "RES6", H,     NO_DATA);
    instrMapCB.emplace_back(0xb5, &CoreSM83::RES6, 2, "RES6", L,     NO_DATA);
    instrMapCB.emplace_back(0xb6, &CoreSM83::RES6, 4, "RES6", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xb7, &CoreSM83::RES6, 2, "RES6", A,     NO_DATA);
    instrMapCB.emplace_back(0xb8, &CoreSM83::RES7, 2, "RES7", B,     NO_DATA);
    instrMapCB.emplace_back(0xb9, &CoreSM83::RES7, 2, "RES7", C,     NO_DATA);
    instrMapCB.emplace_back(0xba, &CoreSM83::RES7, 2, "RES7", D,     NO_DATA);
    instrMapCB.emplace_back(0xbb, &CoreSM83::RES7, 2, "RES7", E,     NO_DATA);
    instrMapCB.emplace_back(0xbc, &CoreSM83::RES7, 2, "RES7", H,     NO_DATA);
    instrMapCB.emplace_back(0xbd, &CoreSM83::RES7, 2, "RES7", L,     NO_DATA);
    instrMapCB.emplace_back(0xbe, &CoreSM83::RES7, 4, "RES7", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xbf, &CoreSM83::RES7, 2, "RES7", A,     NO_DATA);

    instrMapCB.emplace_back(0xc0, &CoreSM83::SET0, 2, "SET0", B,     NO_DATA);
    instrMapCB.emplace_back(0xc1, &CoreSM83::SET0, 2, "SET0", C,     NO_DATA);
    instrMapCB.emplace_back(0xc2, &CoreSM83::SET0, 2, "SET0", D,     NO_DATA);
    instrMapCB.emplace_back(0xc3, &CoreSM83::SET0, 2, "SET0", E,     NO_DATA);
    instrMapCB.emplace_back(0xc4, &CoreSM83::SET0, 2, "SET0", H,     NO_DATA);
    instrMapCB.emplace_back(0xc5, &CoreSM83::SET0, 2, "SET0", L,     NO_DATA);
    instrMapCB.emplace_back(0xc6, &CoreSM83::SET0, 4, "SET0", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xc7, &CoreSM83::SET0, 2, "SET0", A,     NO_DATA);
    instrMapCB.emplace_back(0xc8, &CoreSM83::SET1, 2, "SET1", B,     NO_DATA);
    instrMapCB.emplace_back(0xc9, &CoreSM83::SET1, 2, "SET1", C,     NO_DATA);
    instrMapCB.emplace_back(0xca, &CoreSM83::SET1, 2, "SET1", D,     NO_DATA);
    instrMapCB.emplace_back(0xcb, &CoreSM83::SET1, 2, "SET1", E,     NO_DATA);
    instrMapCB.emplace_back(0xcc, &CoreSM83::SET1, 2, "SET1", H,     NO_DATA);
    instrMapCB.emplace_back(0xcd, &CoreSM83::SET1, 2, "SET1", L,     NO_DATA);
    instrMapCB.emplace_back(0xce, &CoreSM83::SET1, 4, "SET1", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xcf, &CoreSM83::SET1, 2, "SET1", A,     NO_DATA);

    instrMapCB.emplace_back(0xd0, &CoreSM83::SET2, 2, "SET2", B,     NO_DATA);
    instrMapCB.emplace_back(0xd1, &CoreSM83::SET2, 2, "SET2", C,     NO_DATA);
    instrMapCB.emplace_back(0xd2, &CoreSM83::SET2, 2, "SET2", D,     NO_DATA);
    instrMapCB.emplace_back(0xd3, &CoreSM83::SET2, 2, "SET2", E,     NO_DATA);
    instrMapCB.emplace_back(0xd4, &CoreSM83::SET2, 2, "SET2", H,     NO_DATA);
    instrMapCB.emplace_back(0xd5, &CoreSM83::SET2, 2, "SET2", L,     NO_DATA);
    instrMapCB.emplace_back(0xd6, &CoreSM83::SET2, 4, "SET2", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xd7, &CoreSM83::SET2, 2, "SET2", A,     NO_DATA);
    instrMapCB.emplace_back(0xd8, &CoreSM83::SET3, 2, "SET3", B,     NO_DATA);
    instrMapCB.emplace_back(0xd9, &CoreSM83::SET3, 2, "SET3", C,     NO_DATA);
    instrMapCB.emplace_back(0xda, &CoreSM83::SET3, 2, "SET3", D,     NO_DATA);
    instrMapCB.emplace_back(0xdb, &CoreSM83::SET3, 2, "SET3", E,     NO_DATA);
    instrMapCB.emplace_back(0xdc, &CoreSM83::SET3, 2, "SET3", H,     NO_DATA);
    instrMapCB.emplace_back(0xdd, &CoreSM83::SET3, 2, "SET3", L,     NO_DATA);
    instrMapCB.emplace_back(0xde, &CoreSM83::SET3, 4, "SET3", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xdf, &CoreSM83::SET3, 2, "SET3", A,     NO_DATA);

    instrMapCB.emplace_back(0xe0, &CoreSM83::SET4, 2, "SET4", B,     NO_DATA);
    instrMapCB.emplace_back(0xe1, &CoreSM83::SET4, 2, "SET4", C,     NO_DATA);
    instrMapCB.emplace_back(0xe2, &CoreSM83::SET4, 2, "SET4", D,     NO_DATA);
    instrMapCB.emplace_back(0xe3, &CoreSM83::SET4, 2, "SET4", E,     NO_DATA);
    instrMapCB.emplace_back(0xe4, &CoreSM83::SET4, 2, "SET4", H,     NO_DATA);
    instrMapCB.emplace_back(0xe5, &CoreSM83::SET4, 2, "SET4", L,     NO_DATA);
    instrMapCB.emplace_back(0xe6, &CoreSM83::SET4, 4, "SET4", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xe7, &CoreSM83::SET4, 2, "SET4", A,     NO_DATA);
    instrMapCB.emplace_back(0xe8, &CoreSM83::SET5, 2, "SET5", B,     NO_DATA);
    instrMapCB.emplace_back(0xe9, &CoreSM83::SET5, 2, "SET5", C,     NO_DATA);
    instrMapCB.emplace_back(0xea, &CoreSM83::SET5, 2, "SET5", D,     NO_DATA);
    instrMapCB.emplace_back(0xeb, &CoreSM83::SET5, 2, "SET5", E,     NO_DATA);
    instrMapCB.emplace_back(0xec, &CoreSM83::SET5, 2, "SET5", H,     NO_DATA);
    instrMapCB.emplace_back(0xed, &CoreSM83::SET5, 2, "SET5", L,     NO_DATA);
    instrMapCB.emplace_back(0xee, &CoreSM83::SET5, 4, "SET5", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xef, &CoreSM83::SET5, 2, "SET5", A,     NO_DATA);

    instrMapCB.emplace_back(0xf0, &CoreSM83::SET6, 2, "SET6", B,     NO_DATA);
    instrMapCB.emplace_back(0xf1, &CoreSM83::SET6, 2, "SET6", C,     NO_DATA);
    instrMapCB.emplace_back(0xf2, &CoreSM83::SET6, 2, "SET6", D,     NO_DATA);
    instrMapCB.emplace_back(0xf3, &CoreSM83::SET6, 2, "SET6", E,     NO_DATA);
    instrMapCB.emplace_back(0xf4, &CoreSM83::SET6, 2, "SET6", H,     NO_DATA);
    instrMapCB.emplace_back(0xf5, &CoreSM83::SET6, 2, "SET6", L,     NO_DATA);
    instrMapCB.emplace_back(0xf6, &CoreSM83::SET6, 4, "SET6", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xf7, &CoreSM83::SET6, 2, "SET6", A,     NO_DATA);
    instrMapCB.emplace_back(0xf8, &CoreSM83::SET7, 2, "SET7", B,     NO_DATA);
    instrMapCB.emplace_back(0xf9, &CoreSM83::SET7, 2, "SET7", C,     NO_DATA);
    instrMapCB.emplace_back(0xfa, &CoreSM83::SET7, 2, "SET7", D,     NO_DATA);
    instrMapCB.emplace_back(0xfb, &CoreSM83::SET7, 2, "SET7", E,     NO_DATA);
    instrMapCB.emplace_back(0xfc, &CoreSM83::SET7, 2, "SET7", H,     NO_DATA);
    instrMapCB.emplace_back(0xfd, &CoreSM83::SET7, 2, "SET7", L,     NO_DATA);
    instrMapCB.emplace_back(0xfe, &CoreSM83::SET7, 4, "SET7", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xff, &CoreSM83::SET7, 2, "SET7", A,     NO_DATA);
}

/* ***********************************************************************************************************
*
*   GAMEBOY (COLOR) CORESM83 CB INSTRUCTION SET
*
*********************************************************************************************************** */
// rotate left
void CoreSM83::RLC() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        Regs.F |= (data & MSB ? FLAG_CARRY : 0x00);
        data = (data << 1) & 0xFF;
        data |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
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
void CoreSM83::RRC() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        Regs.F |= (data & LSB ? FLAG_CARRY : 0x00);
        data = (data >> 1) & 0xFF;
        data |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
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
void CoreSM83::RL() {
    static bool carry = Regs.F & FLAG_CARRY;
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
        data = mmu_instance->Read8Bit(Regs.HL);
        Regs.F |= (data & MSB ? FLAG_CARRY : 0x00);
        data = (data << 1) & 0xFF;
        data |= (carry ? LSB : 0x00);
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
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
void CoreSM83::RR() {
    static bool carry = Regs.F & FLAG_CARRY;
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
        data = mmu_instance->Read8Bit(Regs.HL);
        Regs.F |= (data & LSB ? FLAG_CARRY : 0x00);
        data = (data >> 1) & 0xFF;
        data |= (carry ? MSB : 0x00);
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
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
void CoreSM83::SLA() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        Regs.F |= (data & MSB ? FLAG_CARRY : 0x00);
        data = (data << 1) & 0xFF;
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.F |= (Regs.A & MSB ? FLAG_CARRY : 0x00);
        Regs.A <<= 1;
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// shift right arithmetic
void CoreSM83::SRA() {
    RESET_ALL_FLAGS(Regs.F);
    static bool msb;

    switch (opcode & 0x07) {
    case 0x00:
        msb = (Regs.BC_.B & MSB);
        Regs.BC_.B >>= 1;
        Regs.BC_.B |= (msb ? MSB : 0x00);
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x01:
        msb = (Regs.BC_.C & MSB);
        Regs.BC_.C >>= 1;
        Regs.BC_.C |= (msb ? MSB : 0x00);
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x02:
        msb = (Regs.DE_.D & MSB);
        Regs.DE_.D >>= 1;
        Regs.DE_.D |= (msb ? MSB : 0x00);
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x03:
        msb = (Regs.DE_.E & MSB);
        Regs.DE_.E >>= 1;
        Regs.DE_.E |= (msb ? MSB : 0x00);
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x04:
        msb = (Regs.HL_.H & MSB);
        Regs.HL_.H >>= 1;
        Regs.HL_.H |= (msb ? MSB : 0x00);
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x05:
        msb = (Regs.HL_.L & MSB);
        Regs.HL_.L >>= 1;
        Regs.HL_.L |= (msb ? MSB : 0x00);
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        msb = (data & MSB);
        data = (data >> 1) & 0xFF;
        data |= (msb ? MSB : 0x00);
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        msb = (Regs.A & MSB);
        Regs.A >>= 1;
        Regs.A |= (msb ? MSB : 0x00);
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// swap lo<->hi nibble
void CoreSM83::SWAP() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        data = (data >> 4) | ((data << 4) & 0xF0);
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A = (Regs.A >> 4) | (Regs.A << 4);
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// shift right logical
void CoreSM83::SRL() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        Regs.F |= (data & LSB ? FLAG_CARRY : 0x00);
        data = (data >> 1) & 0xFF;
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.F |= (Regs.A & LSB ? FLAG_CARRY : 0x00);
        Regs.A >>= 1;
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// test bit 0
void CoreSM83::BIT0() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x01, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x01, Regs.F);
        break;
    }
}

// test bit 1
void CoreSM83::BIT1() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x02, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x02, Regs.F);
        break;
    }
}

// test bit 2
void CoreSM83::BIT2() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x04, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x04, Regs.F);
        break;
    }
}

// test bit 3
void CoreSM83::BIT3() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x08, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x08, Regs.F);
        break;
    }
}

// test bit 4
void CoreSM83::BIT4() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x10, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x10, Regs.F);
        break;
    }
}

// test bit 5
void CoreSM83::BIT5() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x20, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x20, Regs.F);
        break;
    }
}

// test bit 6
void CoreSM83::BIT6() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x40, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x40, Regs.F);
        break;
    }
}

// test bit 7
void CoreSM83::BIT7() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x80, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x80, Regs.F);
        break;
    }
}

// reset bit 0
void CoreSM83::RES0() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        data &= ~0x01;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x01;
        break;
    }
}

// reset bit 1
void CoreSM83::RES1() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        data &= ~0x02;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x02;
        break;
    }
}

// reset bit 2
void CoreSM83::RES2() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        data &= ~0x04;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x04;
        break;
    }
}

// reset bit 3
void CoreSM83::RES3() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        data &= ~0x08;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x08;
        break;
    }
}

// reset bit 4
void CoreSM83::RES4() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        data &= ~0x10;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x10;
        break;
    }
}

// reset bit 5
void CoreSM83::RES5() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        data &= ~0x20;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x20;
        break;
    }
}

// reset bit 6
void CoreSM83::RES6() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        data &= ~0x40;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x40;
        break;
    }
}

// reset bit 7
void CoreSM83::RES7() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        data &= ~0x80;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x80;
        break;
    }
}

// set bit 0
void CoreSM83::SET0() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        data |= 0x01;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x01;
        break;
    }
}

// set bit 1
void CoreSM83::SET1() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        data |= 0x02;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x02;
        break;
    }
}

// set bit 2
void CoreSM83::SET2() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        data |= 0x04;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x04;
        break;
    }
}

// set bit 3
void CoreSM83::SET3() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        data |= 0x08;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x08;
        break;
    }
}

// set bit 4
void CoreSM83::SET4() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        data |= 0x10;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x10;
        break;
    }
}

// set bit 5
void CoreSM83::SET5() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        data |= 0x20;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x20;
        break;
    }
}

// set bit 6
void CoreSM83::SET6() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        data |= 0x40;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x40;
        break;
    }
}

// set bit 7
void CoreSM83::SET7() {
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
        data = mmu_instance->Read8Bit(Regs.HL);
        data |= 0x80;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x80;
        break;
    }
}

/* ***********************************************************************************************************
*
*   GAMEBOY (COLOR) CORESM83 FUNCTIONALITY
*
*********************************************************************************************************** */

