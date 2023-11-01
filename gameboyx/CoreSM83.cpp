#include "CoreSM83.h"

/* ***********************************************************************************************************
    INCLUDES
*********************************************************************************************************** */
#include "gameboy_defines.h"
#include <format>
#include "logger.h"
#include "MemorySM83.h"
#include "ScrollableTable.h"


#include <iostream>

using namespace std;

/* ***********************************************************************************************************
    MISC LOOKUPS
*********************************************************************************************************** */
enum instr_lookup_enums {
    INSTR_OPCODE,
    INSTR_FUNC,
    INSTR_MC,
    INSTR_MNEMONIC,
    INSTR_ARG_1,
    INSTR_ARG_2
};

const vector<pair<cgb_flag_types, string>> flag_names{
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

inline string get_flag_and_isr_name(const cgb_flag_types& _type) {
    for (const auto& [type, val] : flag_names) {
        if (type == _type) { return val; }
    }

    return "";
}

const vector<pair<cgb_data_types, string>> register_names{
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

inline string get_register_name(const cgb_data_types& _type) {
    for (const auto& [type, val] : register_names) {
        if (type == _type) { return val; }
    }

    return "";
}

const vector<pair<cgb_data_types, string>> data_names{
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

inline string get_data_name(const cgb_data_types& _type) {
    for (const auto& [type, val] : data_names) {
        if (type == _type) { return val; }
    }

    return "";
}

/*
*   Resolve enums to strings for log output (to compare actual instruction execution to debug window)
*   probably no reason to keep the log output
*/
inline string get_arg_name(const cgb_data_types& _type) {
    string result = get_register_name(_type);
    if (result.compare("") == 0) {
        result = get_data_name(_type);
    }
    return result;
}

/*
*   Resolve enums to strings for program debugger
*/
inline string resolve_data_enum(const cgb_data_types& _type, const int& _addr, const u16& _data) {
    string result = "";
    u8 data;

    switch (_type) {
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
        result = get_register_name(_type);
        break;
    case d8:
    case a8:
        result = format("{:02x}", (u8)(_data & 0xFF));
        break;
    case d16:
    case a16:
        result = format("{:04x}", _data);
        break;
    case r8:
        data = (u8)(_data & 0xFF);
        result = format("{:04x}", _addr + *(i8*)&data);
        break;
    case a8_ref:
        result = format("(FF00+{:02x})", (u8)(_data & 0xFF));
        break;
    case a16_ref:
        result = format("({:04x})", _data);
        break;
    case SP_r8:
        data = (u8)(_data & 0xFF);
        result = format("SP+{:02x}", *(i8*)&data);
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

inline void instruction_args_to_string(u16& _addr, const vector<u8>& _bank, u16& _data, string& _raw_data, const cgb_data_types& _type) {
    switch (_type) {
    case d8:
    case a8:
    case a8_ref:
        _data = _bank[_addr++];
        _raw_data += format("{:02x} ", (u8)_data); 
        break;
    case r8:
        _data = _bank[_addr++];
        _raw_data += format("{:02x} ", (u8)_data + 0xFF00);
        break;
    case d16:
    case a16:
    case a16_ref:
        _data = _bank[_addr++];
        _data |= ((u16)_bank[_addr++]) << 8;
        _raw_data += format("{:02x} ", (u8)(_data & 0xFF));
        _raw_data += format("{:02x} ", (u8)((_data & 0xFF00) >> 8));
        break;
    default:
        break;
    }
}

inline void data_enums_to_string(const int& _bank, u16 _addr, const u16& _base_ptr, const u16& _data, string& _args, const cgb_data_types& _type_1, const cgb_data_types& _type_2) {
    _addr += _base_ptr;

    _args = "";
    if (_type_1 != NO_DATA) {
        _args = resolve_data_enum(_type_1, _addr, _data);
    }
    if (_type_2 != NO_DATA) {
        _args += ", " + resolve_data_enum(_type_2, _addr, _data);
    }
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
    CONSTRUCTOR
*********************************************************************************************************** */
CoreSM83::CoreSM83(machine_information& _machine_info) : CoreBase(_machine_info) {
    mem_instance = MemorySM83::getInstance();
    machine_ctx = mem_instance->GetMachineContext();

    InitCpu();

    setupLookupTable();
    setupLookupTableCB();
}

/* ***********************************************************************************************************
    INIT CPU
*********************************************************************************************************** */
// initial cpu state
void CoreSM83::InitCpu() {
    InitRegisterStates();

    machineCyclesPerScanline = (((BASE_CLOCK_CPU / 4) * pow(10, 6)) / DISPLAY_FREQUENCY) / LCD_SCANLINES_TOTAL;
}

// initial register states
void CoreSM83::InitRegisterStates() {
    Regs = gbc_registers();

    Regs.A = (INIT_CGB_AF & 0xFF00) >> 8;
    Regs.F = INIT_CGB_AF & 0xFF;

    Regs.BC = INIT_CGB_BC;
    Regs.SP = INIT_CGB_SP;
    Regs.PC = INIT_CGB_PC;

    if (machine_ctx->isCgb) {
        Regs.DE = INIT_CGB_CGB_DE;
        Regs.HL = INIT_CGB_CGB_HL;
    }
    else {
        Regs.DE = INIT_CGB_DMG_DE;
        Regs.HL = INIT_CGB_DMG_HL;
    }
}


/* ***********************************************************************************************************
    RUN CPU
*********************************************************************************************************** */
void CoreSM83::RunCycles() {
    if (stopped) {
        // check button press
        if (mem_instance->GetIOValue(IF_ADDR) & IRQ_JOYPAD) {
            stopped = false;
        }
    }
    else if (halted) {
        TickTimers();
        // check pending and enabled interrupts
        if (machine_ctx->IE & mem_instance->GetIOValue(IF_ADDR)) {
            halted = false;
            CheckInterrupts();
        }
    }
    else {
        while ((machineCycleScanlineCounter < (machineCyclesPerScanline * machine_ctx->currentSpeed)) && !halted && !stopped) {
            RunCpu();
        }
    }
}

void CoreSM83::RunCycle() {
    if (machineInfo.pause_execution) {
        return;
    }
    else if (stopped) {
        // check button press
        if (mem_instance->GetIOValue(IF_ADDR) & IRQ_JOYPAD) {
            stopped = false;
        }
    }
    else if (halted) {
        TickTimers();
        // check pending and enabled interrupts
        if (machine_ctx->IE & mem_instance->GetIOValue(IF_ADDR)) {
            halted = false;
            CheckInterrupts();
        }
    }
    else {
        RunCpu();
        machineInfo.pause_execution = true;
        if (machineInfo.instruction_logging) { GetCurrentInstruction(); }
    }
}

void CoreSM83::RunCpu() {
    currentMachineCycles = 0;

    CheckInterrupts();
    ExecuteInstruction();

    machineCycleScanlineCounter += currentMachineCycles;
    machineCycleClockCounter += currentMachineCycles;


    /*
    * // Output for blarggs mem_timing_2 test, doesn't write to serial port. result written to RAM (see readme of test)
    int check = 0;
    if (Regs.PC == 0x2bdd) {
        for (int i = 0; i < 4; i++) {
            check |= (mmu_instance->Read8Bit(RAM_N_OFFSET + i) << ((3 - i) * 8));
        }

        if (check == 0x00deb061) {
            string result = "";

            char c;
            for (int i = 0; (c = mmu_instance->Read8Bit(RAM_N_OFFSET + 4 + i)) != 0x00; i++) {
                result += c;
            }

            printf(result.c_str());
        }
        else {
            LOG_WARN("Error");
        }
    }
    */
}

void CoreSM83::ExecuteInstruction() {
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

void CoreSM83::CheckInterrupts() {
    if (ime) {
        u8 isr_requested = mem_instance->GetIOValue(IF_ADDR);
        if ((isr_requested & IRQ_VBLANK) && (machine_ctx->IE & IRQ_VBLANK)) {
            ime = false;

            isr_push(ISR_VBLANK_HANDLER_ADDR);
            isr_requested &= ~IRQ_VBLANK;

            mem_instance->SetIOValue(isr_requested, IF_ADDR);
        }
        else if ((isr_requested & IRQ_LCD_STAT) && (machine_ctx->IE & IRQ_LCD_STAT)) {
            ime = false;

            isr_push(ISR_LCD_STAT_HANDLER_ADDR);
            isr_requested &= ~IRQ_LCD_STAT;

            mem_instance->SetIOValue(isr_requested, IF_ADDR);
        }
        else if ((isr_requested & IRQ_TIMER) && (machine_ctx->IE & IRQ_TIMER)) {
            ime = false;

            isr_push(ISR_TIMER_HANDLER_ADDR);
            isr_requested &= ~IRQ_TIMER;

            mem_instance->SetIOValue(isr_requested, IF_ADDR);
        }
        /*if (machine_ctx->IF & IRQ_SERIAL) {
            // not implemented
        }*/
        else if ((isr_requested & IRQ_JOYPAD) && (machine_ctx->IE & IRQ_JOYPAD)) {
            ime = false;

            isr_push(ISR_JOYPAD_HANDLER_ADDR);
            isr_requested &= ~IRQ_JOYPAD;

            mem_instance->SetIOValue(isr_requested, IF_ADDR);
        }
    }
}

void CoreSM83::TickTimers() {
    bool div_low_byte_selected = machine_ctx->timaDivMask < 0x100;
    u8 div = mem_instance->GetIOValue(DIV_ADDR);
    bool tima_enabled = mem_instance->GetIOValue(TAC_ADDR) & TAC_CLOCK_ENABLE;

    if (machine_ctx->tima_reload_cycle) {
        mem_instance->SetIOValue(mem_instance->GetIOValue(TMA_ADDR), TIMA_ADDR);
        if (!machine_ctx->tima_reload_if_write) {
            mem_instance->RequestInterrupts(IRQ_TIMER);
        }
        else {
            machine_ctx->tima_reload_if_write = false;
        }
        machine_ctx->tima_reload_cycle = false;
    }
    else if (machine_ctx->tima_overflow_cycle) {
        machine_ctx->tima_reload_cycle = true;
        machine_ctx->tima_overflow_cycle = false;
    }

    for (int i = 0; i < 4; i++) {
        if (machine_ctx->div_low_byte == 0xFF) {
            machine_ctx->div_low_byte = 0x00;

            if (div == 0xFF) {
                div = 0x00;
            }
            else {
                div++;
            }
            mem_instance->SetIOValue(div, DIV_ADDR);
        }
        else {
            machine_ctx->div_low_byte++;
        }

        if (div_low_byte_selected) {
            timaEnAndDivOverflowCur = tima_enabled && (machine_ctx->div_low_byte & machine_ctx->timaDivMask ? true : false);
        }
        else {
            timaEnAndDivOverflowCur = tima_enabled && (div & (machine_ctx->timaDivMask >> 8) ? true : false);
        }

        if (!timaEnAndDivOverflowCur && timaEnAndDivOverflowPrev) {
            IncrementTIMA();
        }
        timaEnAndDivOverflowPrev = timaEnAndDivOverflowCur;
    }

    currentMachineCycles++;
}

void CoreSM83::IncrementTIMA() {
    u8 tima = mem_instance->GetIOValue(TIMA_ADDR);
    if (tima == 0xFF) {
        tima = 0x00;
        machine_ctx->tima_overflow_cycle = true;
    }
    else {
        tima++;
    }
    mem_instance->SetIOValue(tima, TIMA_ADDR);
}

void CoreSM83::FetchOpCode() {
    opcode = mmu_instance->Read8Bit(Regs.PC);
    Regs.PC++;
    TickTimers();
}

void CoreSM83::Fetch8Bit() {
    data = mmu_instance->Read8Bit(Regs.PC);
    Regs.PC++;
    TickTimers();
}

void CoreSM83::Fetch16Bit() {
    data = mmu_instance->Read8Bit(Regs.PC);
    Regs.PC++;
    TickTimers();

    data |= (((u16)mmu_instance->Read8Bit(Regs.PC)) << 8);
    Regs.PC++;
    TickTimers();
}

void CoreSM83::Write8Bit(const u8& _data, const u16& _addr) {
    mmu_instance->Write8Bit(_data, _addr);
    TickTimers();
}

void CoreSM83::Write16Bit(const u16& _data, const u16& _addr) {
    mmu_instance->Write8Bit((_data >> 8) & 0xFF, _addr + 1);
    TickTimers();
    mmu_instance->Write8Bit(_data & 0xFF, _addr);
    TickTimers();
}

u8 CoreSM83::Read8Bit(const u16& _addr) {
    u8 data = mmu_instance->Read8Bit(_addr);
    TickTimers();
    return data;
}

u16 CoreSM83::Read16Bit(const u16& _addr) {
    u16 data = mmu_instance->Read8Bit(_addr);
    TickTimers();
    data |= (((u16)mmu_instance->Read8Bit(_addr + 1)) << 8);
    TickTimers();
    return data;
}

/* ***********************************************************************************************************
    ACCESS CPU STATUS
*********************************************************************************************************** */
// return delta t per frame in nanoseconds
int CoreSM83::GetDelayTime() {
    return (1.f / DISPLAY_FREQUENCY) * pow(10, 9);
}

int CoreSM83::GetStepsPerFrame() {
    return LCD_SCANLINES_TOTAL;
}

// check if cpu executed machinecycles per frame
bool CoreSM83::CheckStep() {
    if (machineCycleScanlineCounter >= machineCyclesPerScanline * machine_ctx->currentSpeed) {
        machineCycleScanlineCounter -= machineCyclesPerScanline * machine_ctx->currentSpeed;
        return true;
    }
    else {
        return false;
    }
}

void CoreSM83::ResetStep() {
    machineCycleScanlineCounter -= machineCyclesPerScanline * machine_ctx->currentSpeed;
}

// return clock cycles per second
void CoreSM83::GetCurrentCoreFrequency() {
    u32 result = machineCycleClockCounter * 4;
    machineCycleClockCounter = 0;
    machineInfo.current_frequency = (float)result / pow(10, 6);
}

void CoreSM83::GetCurrentProgramCounter() {
    machineInfo.current_pc = (int)Regs.PC;

    if (Regs.PC < ROM_N_OFFSET) {
        machineInfo.current_rom_bank = 0;
    }
    else if (Regs.PC < VRAM_N_OFFSET) {
        machineInfo.current_rom_bank = machine_ctx->rom_bank_selected + 1;
    }
    else {
        InitMessageBufferProgramTmp();
    }
}

/* ***********************************************************************************************************
    ACCESS HARDWARE STATUS
*********************************************************************************************************** */
// get hardware info startup
void CoreSM83::GetStartupHardwareInfo() const {
    machineInfo.rom_bank_num = machine_ctx->rom_bank_num;
    machineInfo.ram_bank_num = machine_ctx->ram_bank_num;
    machineInfo.wram_bank_num = machine_ctx->wram_bank_num;
    machineInfo.vram_bank_num = machine_ctx->vram_bank_num;
}

// get current hardware status (currently mapped memory banks)
void CoreSM83::GetCurrentHardwareState() const {
    machineInfo.rom_bank_selected = machine_ctx->rom_bank_selected + 1;
    machineInfo.ram_bank_selected = machine_ctx->ram_bank_selected;
    machineInfo.wram_bank_selected = machine_ctx->wram_bank_selected + 1;
    machineInfo.vram_bank_selected = machine_ctx->vram_bank_selected;
}

void CoreSM83::GetCurrentRegisterValues() const {
    machineInfo.register_values.clear();

    machineInfo.register_values.emplace_back(get_register_name(A) + get_register_name(F), format("A:{:02x} F:{:02x}", Regs.A, Regs.F));
    machineInfo.register_values.emplace_back(get_register_name(BC), format("{:04x}", Regs.BC));
    machineInfo.register_values.emplace_back(get_register_name(DE), format("{:04x}", Regs.DE));
    machineInfo.register_values.emplace_back(get_register_name(HL), format("{:04x}", Regs.HL));
    machineInfo.register_values.emplace_back(get_register_name(SP), format("{:04x}", Regs.SP));
    machineInfo.register_values.emplace_back(get_register_name(PC), format("{:04x}", Regs.PC));
    machineInfo.register_values.emplace_back(get_register_name(IE), format("{:02x}", machine_ctx->IE));
    machineInfo.register_values.emplace_back(get_register_name(IF), format("{:02x}", mem_instance->GetIOValue(IF_ADDR)));
}

void CoreSM83::GetCurrentFlagsAndISR() const {
    machineInfo.flag_values.clear();

    machineInfo.flag_values.emplace_back(get_flag_and_isr_name(FLAG_C), format("{:01b}", (Regs.F & FLAG_CARRY) >> 4));
    machineInfo.flag_values.emplace_back(get_flag_and_isr_name(FLAG_H), format("{:01b}", (Regs.F & FLAG_HCARRY) >> 5));
    machineInfo.flag_values.emplace_back(get_flag_and_isr_name(FLAG_N), format("{:01b}", (Regs.F & FLAG_SUB) >> 6));
    machineInfo.flag_values.emplace_back(get_flag_and_isr_name(FLAG_Z), format("{:01b}", (Regs.F & FLAG_ZERO) >> 7));
    machineInfo.flag_values.emplace_back(get_flag_and_isr_name(FLAG_IME), format("{:01b}", ime ? 1 : 0));
    u8 isr_requested = mem_instance->GetIOValue(IF_ADDR);
    machineInfo.flag_values.emplace_back(get_flag_and_isr_name(INT_VBLANK), format("{:01b}", (isr_requested & IRQ_VBLANK)));
    machineInfo.flag_values.emplace_back(get_flag_and_isr_name(INT_STAT), format("{:01b}", (isr_requested & IRQ_LCD_STAT) >> 1));
    machineInfo.flag_values.emplace_back(get_flag_and_isr_name(INT_TIMER), format("{:01b}", (isr_requested & IRQ_TIMER) >> 2));
    machineInfo.flag_values.emplace_back(get_flag_and_isr_name(INT_SERIAL), format("{:01b}", (isr_requested & IRQ_SERIAL) >> 3));
    machineInfo.flag_values.emplace_back(get_flag_and_isr_name(INT_JOYPAD), format("{:01b}", (isr_requested & IRQ_JOYPAD) >> 4));
    
}

void CoreSM83::GetCurrentInstruction() const {
    machineInfo.current_instruction =
        get_register_name(A) + get_register_name(F) + format("({:04x})", (((u16)Regs.A) << 8) | Regs.F) +
        get_register_name(BC) + format("({:04x}) ", Regs.BC) +
        get_register_name(DE) + format("({:04x}) ", Regs.DE) +
        get_register_name(HL) + format("({:04x}) ", Regs.HL) +
        get_register_name(SP) + format("({:04x}) ", Regs.SP);

    machineInfo.current_instruction += " -> ";

    machineInfo.current_instruction += get_register_name(PC) + format("({:04x}): ", machineInfo.current_pc) +
        get<INSTR_MNEMONIC>(*instrPtr) + format("({:02x}) ", opcode);

    string arg = get_arg_name(get<INSTR_ARG_1>(*instrPtr));
    if (arg.compare("") != 0) { machineInfo.current_instruction += " " + arg; }

    arg = get_arg_name(get<INSTR_ARG_2>(*instrPtr));
    if (arg.compare("") != 0) { machineInfo.current_instruction += ", " + arg; }
}

/* ***********************************************************************************************************
    PREPARE DEBUG DATA (DISASSEMBLED INSTRUCTIONS)
*********************************************************************************************************** */
inline void CoreSM83::DecodeRomBankContent(ScrollableTableBuffer<debug_instr_data>& _program_buffer, const pair<int, vector<u8>>& _bank_data, const int& _bank_num) {
    u16 data = 0;
    bool cb = false;

    _program_buffer.clear();

    const auto& base_ptr = _bank_data.first;
    const auto& rom_bank = _bank_data.second;

    auto current_entry = ScrollableTableEntry<debug_instr_data>();

    for (u16 addr = 0, i = 0; addr < rom_bank.size(); i++) {

        current_entry = ScrollableTableEntry<debug_instr_data>();

        // print rom header info
        if (addr == ROM_HEAD_LOGO && _bank_num == 0) {
            get<ST_ENTRY_ADDRESS>(current_entry) = addr;
            get<ST_ENTRY_DATA>(current_entry).first = "ROM" + to_string(_bank_num) + ": " + format("{:04x}  ", addr);
            get<ST_ENTRY_DATA>(current_entry).second = "- HEADER INFO -";
            addr = ROM_HEAD_END + 1;
            _program_buffer.emplace_back(current_entry);
        }
        else {
            u8 opcode = rom_bank[addr];
            get<ST_ENTRY_ADDRESS>(current_entry) = addr + base_ptr;

            instr_tuple* instr_ptr;

            if (cb) { instr_ptr = &(instrMapCB[opcode]); }
            else { instr_ptr = &(instrMap[opcode]); }
            cb = opcode == 0xCB;

            string raw_data;
            raw_data = "ROM" + to_string(_bank_num) + ": " + format("{:04x}  ", addr + base_ptr);
            addr++;

            raw_data += format("{:02x} ", opcode);

            // arguments
            instruction_args_to_string(addr, rom_bank, data, raw_data, get<INSTR_ARG_1>(*instr_ptr));
            instruction_args_to_string(addr, rom_bank, data, raw_data, get<INSTR_ARG_2>(*instr_ptr));
            get<ST_ENTRY_DATA>(current_entry).first = raw_data;

            // instruction to assembly
            string args;
            data_enums_to_string(_bank_num, addr, base_ptr, data, args, get<INSTR_ARG_1>(*instr_ptr), get<INSTR_ARG_2>(*instr_ptr));

            get<ST_ENTRY_DATA>(current_entry).second = get<INSTR_MNEMONIC>(*instr_ptr);
            if (args.compare("") != 0) {
                get<ST_ENTRY_DATA>(current_entry).second += " " + args;
            }

            _program_buffer.emplace_back(current_entry);
        }
    }
}

void CoreSM83::InitMessageBufferProgram() {
    const auto rom_data = mem_instance->GetProgramData();
    machineInfo.program_buffer = ScrollableTable<debug_instr_data>(DEBUG_INSTR_LINES);

    auto next_bank_table = ScrollableTableBuffer<debug_instr_data>();

    for (int bank_num = 0; const auto & bank_data : rom_data) {
        next_bank_table = ScrollableTableBuffer<debug_instr_data>();

        DecodeRomBankContent(next_bank_table, bank_data, bank_num++);

        machineInfo.program_buffer.AddMemoryArea(next_bank_table);
    }
}

inline void CoreSM83::DecodeBankContent(ScrollableTableBuffer<debug_instr_data>& _program_buffer, const pair<int, vector<u8>>& _bank_data, const int& _bank_num, const string& _bank_name) {
    u16 data = 0;
    bool cb = false;

    _program_buffer.clear();

    const auto& base_ptr = _bank_data.first;
    const auto& bank = _bank_data.second;

    auto current_entry = ScrollableTableEntry<debug_instr_data>();

    for (u16 addr = 0, i = 0; addr < bank.size(); i++) {

        current_entry = ScrollableTableEntry<debug_instr_data>();

        u8 opcode = bank[addr];
        get<ST_ENTRY_ADDRESS>(current_entry) = addr + base_ptr;

        instr_tuple* instr_ptr;

        if (cb) { instr_ptr = &(instrMapCB[opcode]); }
        else { instr_ptr = &(instrMap[opcode]); }
        cb = opcode == 0xCB;

        string raw_data;
        raw_data = _bank_name + to_string(_bank_num) + ": " + format("{:04x}  ", addr + base_ptr);
        addr++;

        raw_data += format("{:02x} ", opcode);

        // arguments
        instruction_args_to_string(addr, bank, data, raw_data, get<INSTR_ARG_1>(*instr_ptr));
        instruction_args_to_string(addr, bank, data, raw_data, get<INSTR_ARG_2>(*instr_ptr));
        get<ST_ENTRY_DATA>(current_entry).first = raw_data;

        // instruction to assembly
        string args;
        data_enums_to_string(_bank_num, addr, base_ptr, data, args, get<INSTR_ARG_1>(*instr_ptr), get<INSTR_ARG_2>(*instr_ptr));

        get<ST_ENTRY_DATA>(current_entry).second = get<INSTR_MNEMONIC>(*instr_ptr);
        if (args.compare("") != 0) {
            get<ST_ENTRY_DATA>(current_entry).second += " " + args;
        }

        _program_buffer.emplace_back(current_entry);
    }
}

void CoreSM83::InitMessageBufferProgramTmp() {
    auto bank_table = ScrollableTableBuffer<debug_instr_data>();
    int bank_num;

    if (Regs.PC >= VRAM_N_OFFSET && Regs.PC < RAM_N_OFFSET) {
        if (machineInfo.current_rom_bank != -1) {
            machineInfo.program_buffer_tmp = ScrollableTable<debug_instr_data>(DEBUG_INSTR_LINES);

            graphics_context* graphics_ctx = mem_instance->GetGraphicsContext();
            bank_num = mem_instance->GetIOValue(CGB_VRAM_SELECT_ADDR);
            DecodeBankContent(bank_table, pair(VRAM_N_OFFSET, graphics_ctx->VRAM_N[bank_num]), bank_num, "VRAM");
            machineInfo.current_rom_bank = -1;

            machineInfo.program_buffer_tmp.AddMemoryArea(bank_table);
        }
    }
    else if (Regs.PC >= RAM_N_OFFSET && Regs.PC < WRAM_0_OFFSET) {
        if (machineInfo.current_rom_bank != -2) {
            machineInfo.program_buffer_tmp = ScrollableTable<debug_instr_data>(DEBUG_INSTR_LINES);

            bank_num = machine_ctx->ram_bank_selected;
            DecodeBankContent(bank_table, pair(RAM_N_OFFSET, mem_instance->RAM_N[bank_num]), bank_num, "RAM");
            machineInfo.current_rom_bank = -2;

            machineInfo.program_buffer_tmp.AddMemoryArea(bank_table);
        }
    }
    else if (Regs.PC >= WRAM_0_OFFSET && Regs.PC < WRAM_N_OFFSET) {
        if (machineInfo.current_rom_bank != -3) {
            machineInfo.program_buffer_tmp = ScrollableTable<debug_instr_data>(DEBUG_INSTR_LINES);

            bank_num = 0;
            DecodeBankContent(bank_table, pair(WRAM_0_OFFSET, mem_instance->WRAM_0), bank_num, "WRAM");
            machineInfo.current_rom_bank = -3;

            machineInfo.program_buffer_tmp.AddMemoryArea(bank_table);
        }
    }
    else if (Regs.PC >= WRAM_N_OFFSET && Regs.PC < MIRROR_WRAM_OFFSET) {
        if (machineInfo.current_rom_bank != -4) {
            machineInfo.program_buffer_tmp = ScrollableTable<debug_instr_data>(DEBUG_INSTR_LINES);

            bank_num = machine_ctx->wram_bank_selected;
            DecodeBankContent(bank_table, pair(WRAM_N_OFFSET, mem_instance->WRAM_N[bank_num]), bank_num + 1, "WRAM");
            machineInfo.current_rom_bank = -4;

            machineInfo.program_buffer_tmp.AddMemoryArea(bank_table);
        }
    }
    else if (Regs.PC >= HRAM_OFFSET && Regs.PC < IE_OFFSET) {
        if (machineInfo.current_rom_bank != -5) {
            machineInfo.program_buffer_tmp = ScrollableTable<debug_instr_data>(DEBUG_INSTR_LINES);

            bank_num = 0;
            DecodeBankContent(bank_table, pair(HRAM_OFFSET, mem_instance->HRAM), bank_num, "HRAM");
            machineInfo.current_rom_bank = -5;

            machineInfo.program_buffer_tmp.AddMemoryArea(bank_table);
        }
    }
    else {
        // TODO
    }
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
    instrMap.emplace_back(0x10, &CoreSM83::STOP, 0, "STOP", d8, NO_DATA);
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
    instrMap.emplace_back(0x76, &CoreSM83::HALT, 0, "HALT", NO_DATA, NO_DATA);
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
    instrMap.emplace_back(0xcb, &CoreSM83::CB, 0, "CB", NO_DATA, NO_DATA);
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
void CoreSM83::NoInstruction() {
    return;
}

// no operation
void CoreSM83::NOP() {
    return;
}

// stopped
void CoreSM83::STOP() {
    u8 isr_requested = mem_instance->GetIOValue(IF_ADDR);

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
                    LOG_ERROR("STOP Glitch encountered");
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
            LOG_ERROR("Wrong usage of STOP instruction at: ", format("{:x}", Regs.PC - 1));
            return;
        }
    }

    if (div_reset) {
        mmu_instance->Write8Bit(0x00, DIV_ADDR);
    }

    if (machine_ctx->speed_switch_requested) {
        machine_ctx->currentSpeed ^= 3;
        machine_ctx->speed_switch_requested = false;
    }
}

// halted
void CoreSM83::HALT() {
    halted = true;
}

// flip c
void CoreSM83::CCF() {
    RESET_FLAGS(FLAG_HCARRY | FLAG_SUB, Regs.F);
    Regs.F ^= FLAG_CARRY;
}

// 1's complement of A
void CoreSM83::CPL() {
    SET_FLAGS(FLAG_SUB | FLAG_HCARRY, Regs.F);
    Regs.A = ~(Regs.A);
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
    // TODO: check for problems, the cb prefixed instruction gets actually executed the same cycle it gets fetched
    //TickTimers();
    return;
}

/* ***********************************************************************************************************
    LOAD
*********************************************************************************************************** */
// load 8/16 bit
void CoreSM83::LDfromAtoRef() {
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

void CoreSM83::LDtoAfromRef() {
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

void CoreSM83::LDd8() {
    Fetch8Bit();

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
        Write8Bit(data, Regs.HL);
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

void CoreSM83::LDCref() {
    switch (opcode) {
    case 0xE2:
        Write8Bit(Regs.A, Regs.BC_.C | 0xFF00);
        break;
    case 0xF2:
        Regs.A = Read8Bit(Regs.BC_.C | 0xFF00);
        break;
    }
}

void CoreSM83::LDH() {
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

void CoreSM83::LDHa16() {
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

void CoreSM83::LDSPa16() {
    Fetch16Bit();
    Write16Bit(Regs.SP, data);
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
        Regs.BC_.B = Read8Bit(Regs.HL);
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
        Regs.BC_.C = Read8Bit(Regs.HL);
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
        Regs.DE_.D = Read8Bit(Regs.HL);
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
        Regs.DE_.E = Read8Bit(Regs.HL);
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
        Regs.HL_.H = Read8Bit(Regs.HL);
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
        Regs.HL_.L = Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.HL_.L = Regs.A;
        break;
    }
}

void CoreSM83::LDtoHLref() {
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
        Regs.A = Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.A = Regs.A;
        break;
    }
}

// ld SP to HL and add signed 8 bit immediate data
void CoreSM83::LDHLSPr8() {
    Fetch8Bit();

    Regs.HL = Regs.SP;
    RESET_ALL_FLAGS(Regs.F);
    ADD_8_FLAGS(Regs.HL, data, Regs.F);
    Regs.HL += *(i8*)&data;

    TickTimers();
}

void CoreSM83::LDSPHL() {
    Regs.SP = Regs.HL;
    TickTimers();
}

// push PC to Stack
void CoreSM83::PUSH() {
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

void CoreSM83::stack_push(const u16& _data) {
    Regs.SP -= 2;
    TickTimers();

    Write16Bit(_data, Regs.SP);
}

void CoreSM83::isr_push(const u16& _isr_handler) {
    // simulate 2 NOPs
    TickTimers();
    TickTimers();

    stack_push(Regs.PC);
    Regs.PC = _isr_handler;
}

// pop PC from Stack
void CoreSM83::POP() {
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

u16 CoreSM83::stack_pop() {
    u16 data = Read16Bit(Regs.SP);
    Regs.SP += 2;
    return data;
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
        data = Read8Bit(Regs.HL);
        ADD_8_HC(data, 1, Regs.F);
        data = ((data + 1) & 0xFF);
        ZERO_FLAG(data, Regs.F);
        Write8Bit(data, Regs.HL);
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
    TickTimers();
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
        data = Read8Bit(Regs.HL);
        SUB_8_HC(data, 1, Regs.F);
        data -= 1;
        ZERO_FLAG(data, Regs.F);
        Write8Bit(data, Regs.HL);
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
    TickTimers();
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

    TickTimers();
}

// add for SP + signed immediate data r8
void CoreSM83::ADDSPr8() {
    Fetch8Bit();

    RESET_ALL_FLAGS(Regs.F);
    ADD_8_FLAGS(Regs.SP, data, Regs.F);
    Regs.SP += *(i8*)&data;

    TickTimers();
    TickTimers();
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
    bool carry = Regs.F & FLAG_CARRY;
    RESET_ALL_FLAGS(Regs.F);
    Regs.F |= (Regs.A & MSB ? FLAG_CARRY : 0x00);
    Regs.A <<= 1;
    Regs.A |= (carry ? LSB : 0x00);
}

// rotate A right through carry
void CoreSM83::RRA() {
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
void CoreSM83::JP() {
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

void CoreSM83::jump_jp() {
    Regs.PC = data;
    if (opcode != 0xe9) { TickTimers(); }
}

// jump relative to memory lecation
void CoreSM83::JR() {
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

void CoreSM83::jump_jr() {
    Regs.PC += *(i8*)&data;
    TickTimers();
}

// call routine at memory location
void CoreSM83::CALL() {
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
    stack_push(Regs.PC);
    Regs.PC = data;
}

// return from routine
void CoreSM83::RET() {
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
void CoreSM83::RETI() {
    ret();
    ime = true;
}

void CoreSM83::ret() {
    Regs.PC = stack_pop();
    TickTimers();
}

/* ***********************************************************************************************************
*
*   GAMEBOY (COLOR) CORESM83 CB INSTRUCTION LOOKUP TABLE
*
*********************************************************************************************************** */
void CoreSM83::setupLookupTableCB() {
    instrMapCB.clear();

    // Elements: opcode, instruction function, machine cycles
    instrMapCB.emplace_back(0x00, &CoreSM83::RLC, 2, "RLC", B, NO_DATA);
    instrMapCB.emplace_back(0x01, &CoreSM83::RLC, 2, "RLC", C, NO_DATA);
    instrMapCB.emplace_back(0x02, &CoreSM83::RLC, 2, "RLC", D, NO_DATA);
    instrMapCB.emplace_back(0x03, &CoreSM83::RLC, 2, "RLC", E, NO_DATA);
    instrMapCB.emplace_back(0x04, &CoreSM83::RLC, 2, "RLC", H, NO_DATA);
    instrMapCB.emplace_back(0x05, &CoreSM83::RLC, 2, "RLC", L, NO_DATA);
    instrMapCB.emplace_back(0x06, &CoreSM83::RLC, 4, "RLC", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x07, &CoreSM83::RLC, 2, "RLC", A, NO_DATA);
    instrMapCB.emplace_back(0x08, &CoreSM83::RRC, 2, "RRC", B, NO_DATA);
    instrMapCB.emplace_back(0x09, &CoreSM83::RRC, 2, "RRC", C, NO_DATA);
    instrMapCB.emplace_back(0x0a, &CoreSM83::RRC, 2, "RRC", D, NO_DATA);
    instrMapCB.emplace_back(0x0b, &CoreSM83::RRC, 2, "RRC", E, NO_DATA);
    instrMapCB.emplace_back(0x0c, &CoreSM83::RRC, 2, "RRC", H, NO_DATA);
    instrMapCB.emplace_back(0x0d, &CoreSM83::RRC, 2, "RRC", L, NO_DATA);
    instrMapCB.emplace_back(0x0e, &CoreSM83::RRC, 4, "RRC", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x0f, &CoreSM83::RRC, 2, "RRC", A, NO_DATA);

    instrMapCB.emplace_back(0x10, &CoreSM83::RL, 2, "RL", B, NO_DATA);
    instrMapCB.emplace_back(0x11, &CoreSM83::RL, 2, "RL", C, NO_DATA);
    instrMapCB.emplace_back(0x12, &CoreSM83::RL, 2, "RL", D, NO_DATA);
    instrMapCB.emplace_back(0x13, &CoreSM83::RL, 2, "RL", E, NO_DATA);
    instrMapCB.emplace_back(0x14, &CoreSM83::RL, 2, "RL", H, NO_DATA);
    instrMapCB.emplace_back(0x15, &CoreSM83::RL, 2, "RL", L, NO_DATA);
    instrMapCB.emplace_back(0x16, &CoreSM83::RL, 4, "RL", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x17, &CoreSM83::RL, 2, "RL", A, NO_DATA);
    instrMapCB.emplace_back(0x18, &CoreSM83::RR, 2, "RR", B, NO_DATA);
    instrMapCB.emplace_back(0x19, &CoreSM83::RR, 2, "RR", C, NO_DATA);
    instrMapCB.emplace_back(0x1a, &CoreSM83::RR, 2, "RR", D, NO_DATA);
    instrMapCB.emplace_back(0x1b, &CoreSM83::RR, 2, "RR", E, NO_DATA);
    instrMapCB.emplace_back(0x1c, &CoreSM83::RR, 2, "RR", H, NO_DATA);
    instrMapCB.emplace_back(0x1d, &CoreSM83::RR, 2, "RR", L, NO_DATA);
    instrMapCB.emplace_back(0x1e, &CoreSM83::RR, 4, "RR", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x1f, &CoreSM83::RR, 2, "RR", A, NO_DATA);

    instrMapCB.emplace_back(0x20, &CoreSM83::SLA, 2, "SLA", B, NO_DATA);
    instrMapCB.emplace_back(0x21, &CoreSM83::SLA, 2, "SLA", C, NO_DATA);
    instrMapCB.emplace_back(0x22, &CoreSM83::SLA, 2, "SLA", D, NO_DATA);
    instrMapCB.emplace_back(0x23, &CoreSM83::SLA, 2, "SLA", E, NO_DATA);
    instrMapCB.emplace_back(0x24, &CoreSM83::SLA, 2, "SLA", H, NO_DATA);
    instrMapCB.emplace_back(0x25, &CoreSM83::SLA, 2, "SLA", L, NO_DATA);
    instrMapCB.emplace_back(0x26, &CoreSM83::SLA, 4, "SLA", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x27, &CoreSM83::SLA, 2, "SLA", A, NO_DATA);
    instrMapCB.emplace_back(0x28, &CoreSM83::SRA, 2, "SRA", B, NO_DATA);
    instrMapCB.emplace_back(0x29, &CoreSM83::SRA, 2, "SRA", C, NO_DATA);
    instrMapCB.emplace_back(0x2a, &CoreSM83::SRA, 2, "SRA", D, NO_DATA);
    instrMapCB.emplace_back(0x2b, &CoreSM83::SRA, 2, "SRA", E, NO_DATA);
    instrMapCB.emplace_back(0x2c, &CoreSM83::SRA, 2, "SRA", H, NO_DATA);
    instrMapCB.emplace_back(0x2d, &CoreSM83::SRA, 2, "SRA", L, NO_DATA);
    instrMapCB.emplace_back(0x2e, &CoreSM83::SRA, 4, "SRA", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x2f, &CoreSM83::SRA, 2, "SRA", A, NO_DATA);

    instrMapCB.emplace_back(0x30, &CoreSM83::SWAP, 2, "SWAP", B, NO_DATA);
    instrMapCB.emplace_back(0x31, &CoreSM83::SWAP, 2, "SWAP", C, NO_DATA);
    instrMapCB.emplace_back(0x32, &CoreSM83::SWAP, 2, "SWAP", D, NO_DATA);
    instrMapCB.emplace_back(0x33, &CoreSM83::SWAP, 2, "SWAP", E, NO_DATA);
    instrMapCB.emplace_back(0x34, &CoreSM83::SWAP, 2, "SWAP", H, NO_DATA);
    instrMapCB.emplace_back(0x35, &CoreSM83::SWAP, 2, "SWAP", L, NO_DATA);
    instrMapCB.emplace_back(0x36, &CoreSM83::SWAP, 4, "SWAP", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x37, &CoreSM83::SWAP, 2, "SWAP", A, NO_DATA);
    instrMapCB.emplace_back(0x38, &CoreSM83::SRL, 2, "SRL", B, NO_DATA);
    instrMapCB.emplace_back(0x39, &CoreSM83::SRL, 2, "SRL", C, NO_DATA);
    instrMapCB.emplace_back(0x3a, &CoreSM83::SRL, 2, "SRL", D, NO_DATA);
    instrMapCB.emplace_back(0x3b, &CoreSM83::SRL, 2, "SRL", E, NO_DATA);
    instrMapCB.emplace_back(0x3c, &CoreSM83::SRL, 2, "SRL", H, NO_DATA);
    instrMapCB.emplace_back(0x3d, &CoreSM83::SRL, 2, "SRL", L, NO_DATA);
    instrMapCB.emplace_back(0x3e, &CoreSM83::SRL, 4, "SRL", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x3f, &CoreSM83::SRL, 2, "SRL", A, NO_DATA);

    instrMapCB.emplace_back(0x40, &CoreSM83::BIT0, 2, "BIT0", B, NO_DATA);
    instrMapCB.emplace_back(0x41, &CoreSM83::BIT0, 2, "BIT0", C, NO_DATA);
    instrMapCB.emplace_back(0x42, &CoreSM83::BIT0, 2, "BIT0", D, NO_DATA);
    instrMapCB.emplace_back(0x43, &CoreSM83::BIT0, 2, "BIT0", E, NO_DATA);
    instrMapCB.emplace_back(0x44, &CoreSM83::BIT0, 2, "BIT0", H, NO_DATA);
    instrMapCB.emplace_back(0x45, &CoreSM83::BIT0, 2, "BIT0", L, NO_DATA);
    instrMapCB.emplace_back(0x46, &CoreSM83::BIT0, 4, "BIT0", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x47, &CoreSM83::BIT0, 2, "BIT0", A, NO_DATA);
    instrMapCB.emplace_back(0x48, &CoreSM83::BIT1, 2, "BIT1", B, NO_DATA);
    instrMapCB.emplace_back(0x49, &CoreSM83::BIT1, 2, "BIT1", C, NO_DATA);
    instrMapCB.emplace_back(0x4a, &CoreSM83::BIT1, 2, "BIT1", D, NO_DATA);
    instrMapCB.emplace_back(0x4b, &CoreSM83::BIT1, 2, "BIT1", E, NO_DATA);
    instrMapCB.emplace_back(0x4c, &CoreSM83::BIT1, 2, "BIT1", H, NO_DATA);
    instrMapCB.emplace_back(0x4d, &CoreSM83::BIT1, 2, "BIT1", L, NO_DATA);
    instrMapCB.emplace_back(0x4e, &CoreSM83::BIT1, 4, "BIT1", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x4f, &CoreSM83::BIT1, 2, "BIT1", A, NO_DATA);

    instrMapCB.emplace_back(0x50, &CoreSM83::BIT2, 2, "BIT2", B, NO_DATA);
    instrMapCB.emplace_back(0x51, &CoreSM83::BIT2, 2, "BIT2", C, NO_DATA);
    instrMapCB.emplace_back(0x52, &CoreSM83::BIT2, 2, "BIT2", D, NO_DATA);
    instrMapCB.emplace_back(0x53, &CoreSM83::BIT2, 2, "BIT2", E, NO_DATA);
    instrMapCB.emplace_back(0x54, &CoreSM83::BIT2, 2, "BIT2", H, NO_DATA);
    instrMapCB.emplace_back(0x55, &CoreSM83::BIT2, 2, "BIT2", L, NO_DATA);
    instrMapCB.emplace_back(0x56, &CoreSM83::BIT2, 4, "BIT2", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x57, &CoreSM83::BIT2, 2, "BIT2", A, NO_DATA);
    instrMapCB.emplace_back(0x58, &CoreSM83::BIT3, 2, "BIT3", B, NO_DATA);
    instrMapCB.emplace_back(0x59, &CoreSM83::BIT3, 2, "BIT3", C, NO_DATA);
    instrMapCB.emplace_back(0x5a, &CoreSM83::BIT3, 2, "BIT3", D, NO_DATA);
    instrMapCB.emplace_back(0x5b, &CoreSM83::BIT3, 2, "BIT3", E, NO_DATA);
    instrMapCB.emplace_back(0x5c, &CoreSM83::BIT3, 2, "BIT3", H, NO_DATA);
    instrMapCB.emplace_back(0x5d, &CoreSM83::BIT3, 2, "BIT3", L, NO_DATA);
    instrMapCB.emplace_back(0x5e, &CoreSM83::BIT3, 4, "BIT3", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x5f, &CoreSM83::BIT3, 2, "BIT3", A, NO_DATA);

    instrMapCB.emplace_back(0x60, &CoreSM83::BIT4, 2, "BIT4", B, NO_DATA);
    instrMapCB.emplace_back(0x61, &CoreSM83::BIT4, 2, "BIT4", C, NO_DATA);
    instrMapCB.emplace_back(0x62, &CoreSM83::BIT4, 2, "BIT4", D, NO_DATA);
    instrMapCB.emplace_back(0x63, &CoreSM83::BIT4, 2, "BIT4", E, NO_DATA);
    instrMapCB.emplace_back(0x64, &CoreSM83::BIT4, 2, "BIT4", H, NO_DATA);
    instrMapCB.emplace_back(0x65, &CoreSM83::BIT4, 2, "BIT4", L, NO_DATA);
    instrMapCB.emplace_back(0x66, &CoreSM83::BIT4, 4, "BIT4", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x67, &CoreSM83::BIT4, 2, "BIT4", A, NO_DATA);
    instrMapCB.emplace_back(0x68, &CoreSM83::BIT5, 2, "BIT5", B, NO_DATA);
    instrMapCB.emplace_back(0x69, &CoreSM83::BIT5, 2, "BIT5", C, NO_DATA);
    instrMapCB.emplace_back(0x6a, &CoreSM83::BIT5, 2, "BIT5", D, NO_DATA);
    instrMapCB.emplace_back(0x6b, &CoreSM83::BIT5, 2, "BIT5", E, NO_DATA);
    instrMapCB.emplace_back(0x6c, &CoreSM83::BIT5, 2, "BIT5", H, NO_DATA);
    instrMapCB.emplace_back(0x6d, &CoreSM83::BIT5, 2, "BIT5", L, NO_DATA);
    instrMapCB.emplace_back(0x6e, &CoreSM83::BIT5, 4, "BIT5", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x6f, &CoreSM83::BIT5, 2, "BIT5", A, NO_DATA);

    instrMapCB.emplace_back(0x70, &CoreSM83::BIT6, 2, "BIT6", B, NO_DATA);
    instrMapCB.emplace_back(0x71, &CoreSM83::BIT6, 2, "BIT6", C, NO_DATA);
    instrMapCB.emplace_back(0x72, &CoreSM83::BIT6, 2, "BIT6", D, NO_DATA);
    instrMapCB.emplace_back(0x73, &CoreSM83::BIT6, 2, "BIT6", E, NO_DATA);
    instrMapCB.emplace_back(0x74, &CoreSM83::BIT6, 2, "BIT6", H, NO_DATA);
    instrMapCB.emplace_back(0x75, &CoreSM83::BIT6, 2, "BIT6", L, NO_DATA);
    instrMapCB.emplace_back(0x76, &CoreSM83::BIT6, 4, "BIT6", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x77, &CoreSM83::BIT6, 2, "BIT6", A, NO_DATA);
    instrMapCB.emplace_back(0x78, &CoreSM83::BIT7, 2, "BIT7", B, NO_DATA);
    instrMapCB.emplace_back(0x79, &CoreSM83::BIT7, 2, "BIT7", C, NO_DATA);
    instrMapCB.emplace_back(0x7a, &CoreSM83::BIT7, 2, "BIT7", D, NO_DATA);
    instrMapCB.emplace_back(0x7b, &CoreSM83::BIT7, 2, "BIT7", E, NO_DATA);
    instrMapCB.emplace_back(0x7c, &CoreSM83::BIT7, 2, "BIT7", H, NO_DATA);
    instrMapCB.emplace_back(0x7d, &CoreSM83::BIT7, 2, "BIT7", L, NO_DATA);
    instrMapCB.emplace_back(0x7e, &CoreSM83::BIT7, 4, "BIT7", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x7f, &CoreSM83::BIT7, 2, "BIT7", A, NO_DATA);

    instrMapCB.emplace_back(0x80, &CoreSM83::RES0, 2, "RES0", B, NO_DATA);
    instrMapCB.emplace_back(0x81, &CoreSM83::RES0, 2, "RES0", C, NO_DATA);
    instrMapCB.emplace_back(0x82, &CoreSM83::RES0, 2, "RES0", D, NO_DATA);
    instrMapCB.emplace_back(0x83, &CoreSM83::RES0, 2, "RES0", E, NO_DATA);
    instrMapCB.emplace_back(0x84, &CoreSM83::RES0, 2, "RES0", H, NO_DATA);
    instrMapCB.emplace_back(0x85, &CoreSM83::RES0, 2, "RES0", L, NO_DATA);
    instrMapCB.emplace_back(0x86, &CoreSM83::RES0, 4, "RES0", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x87, &CoreSM83::RES0, 2, "RES0", A, NO_DATA);
    instrMapCB.emplace_back(0x88, &CoreSM83::RES1, 2, "RES1", B, NO_DATA);
    instrMapCB.emplace_back(0x89, &CoreSM83::RES1, 2, "RES1", C, NO_DATA);
    instrMapCB.emplace_back(0x8a, &CoreSM83::RES1, 2, "RES1", D, NO_DATA);
    instrMapCB.emplace_back(0x8b, &CoreSM83::RES1, 2, "RES1", E, NO_DATA);
    instrMapCB.emplace_back(0x8c, &CoreSM83::RES1, 2, "RES1", H, NO_DATA);
    instrMapCB.emplace_back(0x8d, &CoreSM83::RES1, 2, "RES1", L, NO_DATA);
    instrMapCB.emplace_back(0x8e, &CoreSM83::RES1, 4, "RES1", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x8f, &CoreSM83::RES1, 2, "RES1", A, NO_DATA);

    instrMapCB.emplace_back(0x90, &CoreSM83::RES2, 2, "RES2", B, NO_DATA);
    instrMapCB.emplace_back(0x91, &CoreSM83::RES2, 2, "RES2", C, NO_DATA);
    instrMapCB.emplace_back(0x92, &CoreSM83::RES2, 2, "RES2", D, NO_DATA);
    instrMapCB.emplace_back(0x93, &CoreSM83::RES2, 2, "RES2", E, NO_DATA);
    instrMapCB.emplace_back(0x94, &CoreSM83::RES2, 2, "RES2", H, NO_DATA);
    instrMapCB.emplace_back(0x95, &CoreSM83::RES2, 2, "RES2", L, NO_DATA);
    instrMapCB.emplace_back(0x96, &CoreSM83::RES2, 4, "RES2", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x97, &CoreSM83::RES2, 2, "RES2", A, NO_DATA);
    instrMapCB.emplace_back(0x98, &CoreSM83::RES3, 2, "RES3", B, NO_DATA);
    instrMapCB.emplace_back(0x99, &CoreSM83::RES3, 2, "RES3", C, NO_DATA);
    instrMapCB.emplace_back(0x9a, &CoreSM83::RES3, 2, "RES3", D, NO_DATA);
    instrMapCB.emplace_back(0x9b, &CoreSM83::RES3, 2, "RES3", E, NO_DATA);
    instrMapCB.emplace_back(0x9c, &CoreSM83::RES3, 2, "RES3", H, NO_DATA);
    instrMapCB.emplace_back(0x9d, &CoreSM83::RES3, 2, "RES3", L, NO_DATA);
    instrMapCB.emplace_back(0x9e, &CoreSM83::RES3, 4, "RES3", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0x9f, &CoreSM83::RES3, 2, "RES3", A, NO_DATA);

    instrMapCB.emplace_back(0xa0, &CoreSM83::RES4, 2, "RES4", B, NO_DATA);
    instrMapCB.emplace_back(0xa1, &CoreSM83::RES4, 2, "RES4", C, NO_DATA);
    instrMapCB.emplace_back(0xa2, &CoreSM83::RES4, 2, "RES4", D, NO_DATA);
    instrMapCB.emplace_back(0xa3, &CoreSM83::RES4, 2, "RES4", E, NO_DATA);
    instrMapCB.emplace_back(0xa4, &CoreSM83::RES4, 2, "RES4", H, NO_DATA);
    instrMapCB.emplace_back(0xa5, &CoreSM83::RES4, 2, "RES4", L, NO_DATA);
    instrMapCB.emplace_back(0xa6, &CoreSM83::RES4, 4, "RES4", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xa7, &CoreSM83::RES4, 2, "RES4", A, NO_DATA);
    instrMapCB.emplace_back(0xa8, &CoreSM83::RES5, 2, "RES5", B, NO_DATA);
    instrMapCB.emplace_back(0xa9, &CoreSM83::RES5, 2, "RES5", C, NO_DATA);
    instrMapCB.emplace_back(0xaa, &CoreSM83::RES5, 2, "RES5", D, NO_DATA);
    instrMapCB.emplace_back(0xab, &CoreSM83::RES5, 2, "RES5", E, NO_DATA);
    instrMapCB.emplace_back(0xac, &CoreSM83::RES5, 2, "RES5", H, NO_DATA);
    instrMapCB.emplace_back(0xad, &CoreSM83::RES5, 2, "RES5", L, NO_DATA);
    instrMapCB.emplace_back(0xae, &CoreSM83::RES5, 4, "RES5", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xaf, &CoreSM83::RES5, 2, "RES5", A, NO_DATA);

    instrMapCB.emplace_back(0xb0, &CoreSM83::RES6, 2, "RES6", B, NO_DATA);
    instrMapCB.emplace_back(0xb1, &CoreSM83::RES6, 2, "RES6", C, NO_DATA);
    instrMapCB.emplace_back(0xb2, &CoreSM83::RES6, 2, "RES6", D, NO_DATA);
    instrMapCB.emplace_back(0xb3, &CoreSM83::RES6, 2, "RES6", E, NO_DATA);
    instrMapCB.emplace_back(0xb4, &CoreSM83::RES6, 2, "RES6", H, NO_DATA);
    instrMapCB.emplace_back(0xb5, &CoreSM83::RES6, 2, "RES6", L, NO_DATA);
    instrMapCB.emplace_back(0xb6, &CoreSM83::RES6, 4, "RES6", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xb7, &CoreSM83::RES6, 2, "RES6", A, NO_DATA);
    instrMapCB.emplace_back(0xb8, &CoreSM83::RES7, 2, "RES7", B, NO_DATA);
    instrMapCB.emplace_back(0xb9, &CoreSM83::RES7, 2, "RES7", C, NO_DATA);
    instrMapCB.emplace_back(0xba, &CoreSM83::RES7, 2, "RES7", D, NO_DATA);
    instrMapCB.emplace_back(0xbb, &CoreSM83::RES7, 2, "RES7", E, NO_DATA);
    instrMapCB.emplace_back(0xbc, &CoreSM83::RES7, 2, "RES7", H, NO_DATA);
    instrMapCB.emplace_back(0xbd, &CoreSM83::RES7, 2, "RES7", L, NO_DATA);
    instrMapCB.emplace_back(0xbe, &CoreSM83::RES7, 4, "RES7", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xbf, &CoreSM83::RES7, 2, "RES7", A, NO_DATA);

    instrMapCB.emplace_back(0xc0, &CoreSM83::SET0, 2, "SET0", B, NO_DATA);
    instrMapCB.emplace_back(0xc1, &CoreSM83::SET0, 2, "SET0", C, NO_DATA);
    instrMapCB.emplace_back(0xc2, &CoreSM83::SET0, 2, "SET0", D, NO_DATA);
    instrMapCB.emplace_back(0xc3, &CoreSM83::SET0, 2, "SET0", E, NO_DATA);
    instrMapCB.emplace_back(0xc4, &CoreSM83::SET0, 2, "SET0", H, NO_DATA);
    instrMapCB.emplace_back(0xc5, &CoreSM83::SET0, 2, "SET0", L, NO_DATA);
    instrMapCB.emplace_back(0xc6, &CoreSM83::SET0, 4, "SET0", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xc7, &CoreSM83::SET0, 2, "SET0", A, NO_DATA);
    instrMapCB.emplace_back(0xc8, &CoreSM83::SET1, 2, "SET1", B, NO_DATA);
    instrMapCB.emplace_back(0xc9, &CoreSM83::SET1, 2, "SET1", C, NO_DATA);
    instrMapCB.emplace_back(0xca, &CoreSM83::SET1, 2, "SET1", D, NO_DATA);
    instrMapCB.emplace_back(0xcb, &CoreSM83::SET1, 2, "SET1", E, NO_DATA);
    instrMapCB.emplace_back(0xcc, &CoreSM83::SET1, 2, "SET1", H, NO_DATA);
    instrMapCB.emplace_back(0xcd, &CoreSM83::SET1, 2, "SET1", L, NO_DATA);
    instrMapCB.emplace_back(0xce, &CoreSM83::SET1, 4, "SET1", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xcf, &CoreSM83::SET1, 2, "SET1", A, NO_DATA);

    instrMapCB.emplace_back(0xd0, &CoreSM83::SET2, 2, "SET2", B, NO_DATA);
    instrMapCB.emplace_back(0xd1, &CoreSM83::SET2, 2, "SET2", C, NO_DATA);
    instrMapCB.emplace_back(0xd2, &CoreSM83::SET2, 2, "SET2", D, NO_DATA);
    instrMapCB.emplace_back(0xd3, &CoreSM83::SET2, 2, "SET2", E, NO_DATA);
    instrMapCB.emplace_back(0xd4, &CoreSM83::SET2, 2, "SET2", H, NO_DATA);
    instrMapCB.emplace_back(0xd5, &CoreSM83::SET2, 2, "SET2", L, NO_DATA);
    instrMapCB.emplace_back(0xd6, &CoreSM83::SET2, 4, "SET2", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xd7, &CoreSM83::SET2, 2, "SET2", A, NO_DATA);
    instrMapCB.emplace_back(0xd8, &CoreSM83::SET3, 2, "SET3", B, NO_DATA);
    instrMapCB.emplace_back(0xd9, &CoreSM83::SET3, 2, "SET3", C, NO_DATA);
    instrMapCB.emplace_back(0xda, &CoreSM83::SET3, 2, "SET3", D, NO_DATA);
    instrMapCB.emplace_back(0xdb, &CoreSM83::SET3, 2, "SET3", E, NO_DATA);
    instrMapCB.emplace_back(0xdc, &CoreSM83::SET3, 2, "SET3", H, NO_DATA);
    instrMapCB.emplace_back(0xdd, &CoreSM83::SET3, 2, "SET3", L, NO_DATA);
    instrMapCB.emplace_back(0xde, &CoreSM83::SET3, 4, "SET3", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xdf, &CoreSM83::SET3, 2, "SET3", A, NO_DATA);

    instrMapCB.emplace_back(0xe0, &CoreSM83::SET4, 2, "SET4", B, NO_DATA);
    instrMapCB.emplace_back(0xe1, &CoreSM83::SET4, 2, "SET4", C, NO_DATA);
    instrMapCB.emplace_back(0xe2, &CoreSM83::SET4, 2, "SET4", D, NO_DATA);
    instrMapCB.emplace_back(0xe3, &CoreSM83::SET4, 2, "SET4", E, NO_DATA);
    instrMapCB.emplace_back(0xe4, &CoreSM83::SET4, 2, "SET4", H, NO_DATA);
    instrMapCB.emplace_back(0xe5, &CoreSM83::SET4, 2, "SET4", L, NO_DATA);
    instrMapCB.emplace_back(0xe6, &CoreSM83::SET4, 4, "SET4", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xe7, &CoreSM83::SET4, 2, "SET4", A, NO_DATA);
    instrMapCB.emplace_back(0xe8, &CoreSM83::SET5, 2, "SET5", B, NO_DATA);
    instrMapCB.emplace_back(0xe9, &CoreSM83::SET5, 2, "SET5", C, NO_DATA);
    instrMapCB.emplace_back(0xea, &CoreSM83::SET5, 2, "SET5", D, NO_DATA);
    instrMapCB.emplace_back(0xeb, &CoreSM83::SET5, 2, "SET5", E, NO_DATA);
    instrMapCB.emplace_back(0xec, &CoreSM83::SET5, 2, "SET5", H, NO_DATA);
    instrMapCB.emplace_back(0xed, &CoreSM83::SET5, 2, "SET5", L, NO_DATA);
    instrMapCB.emplace_back(0xee, &CoreSM83::SET5, 4, "SET5", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xef, &CoreSM83::SET5, 2, "SET5", A, NO_DATA);

    instrMapCB.emplace_back(0xf0, &CoreSM83::SET6, 2, "SET6", B, NO_DATA);
    instrMapCB.emplace_back(0xf1, &CoreSM83::SET6, 2, "SET6", C, NO_DATA);
    instrMapCB.emplace_back(0xf2, &CoreSM83::SET6, 2, "SET6", D, NO_DATA);
    instrMapCB.emplace_back(0xf3, &CoreSM83::SET6, 2, "SET6", E, NO_DATA);
    instrMapCB.emplace_back(0xf4, &CoreSM83::SET6, 2, "SET6", H, NO_DATA);
    instrMapCB.emplace_back(0xf5, &CoreSM83::SET6, 2, "SET6", L, NO_DATA);
    instrMapCB.emplace_back(0xf6, &CoreSM83::SET6, 4, "SET6", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xf7, &CoreSM83::SET6, 2, "SET6", A, NO_DATA);
    instrMapCB.emplace_back(0xf8, &CoreSM83::SET7, 2, "SET7", B, NO_DATA);
    instrMapCB.emplace_back(0xf9, &CoreSM83::SET7, 2, "SET7", C, NO_DATA);
    instrMapCB.emplace_back(0xfa, &CoreSM83::SET7, 2, "SET7", D, NO_DATA);
    instrMapCB.emplace_back(0xfb, &CoreSM83::SET7, 2, "SET7", E, NO_DATA);
    instrMapCB.emplace_back(0xfc, &CoreSM83::SET7, 2, "SET7", H, NO_DATA);
    instrMapCB.emplace_back(0xfd, &CoreSM83::SET7, 2, "SET7", L, NO_DATA);
    instrMapCB.emplace_back(0xfe, &CoreSM83::SET7, 4, "SET7", HL_ref, NO_DATA);
    instrMapCB.emplace_back(0xff, &CoreSM83::SET7, 2, "SET7", A, NO_DATA);
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
        data = Read8Bit(Regs.HL);
        Regs.F |= (data & MSB ? FLAG_CARRY : 0x00);
        data = (data << 1) & 0xFF;
        data |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
        ZERO_FLAG(data, Regs.F);
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
        Regs.F |= (data & LSB ? FLAG_CARRY : 0x00);
        data = (data >> 1) & 0xFF;
        data |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
        ZERO_FLAG(data, Regs.F);
        Write8Bit(data, Regs.HL);
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
        Write8Bit(data, Regs.HL);
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
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
        Regs.F |= (data & MSB ? FLAG_CARRY : 0x00);
        data = (data << 1) & 0xFF;
        ZERO_FLAG(data, Regs.F);
        Write8Bit(data, Regs.HL);
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
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
        data = (data >> 4) | ((data << 4) & 0xF0);
        ZERO_FLAG(data, Regs.F);
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
        Regs.F |= (data & LSB ? FLAG_CARRY : 0x00);
        data = (data >> 1) & 0xFF;
        ZERO_FLAG(data, Regs.F);
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
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
        data = Read8Bit(Regs.HL);
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
        data = Read8Bit(Regs.HL);
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
        data = Read8Bit(Regs.HL);
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
        data = Read8Bit(Regs.HL);
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
        data = Read8Bit(Regs.HL);
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
        data = Read8Bit(Regs.HL);
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
        data = Read8Bit(Regs.HL);
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
        data = Read8Bit(Regs.HL);
        data &= ~0x01;
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
        data &= ~0x02;
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
        data &= ~0x04;
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
        data &= ~0x08;
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
        data &= ~0x10;
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
        data &= ~0x20;
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
        data &= ~0x40;
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
        data &= ~0x80;
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
        data |= 0x01;
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
        data |= 0x02;
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
        data |= 0x04;
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
        data |= 0x08;
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
        data |= 0x10;
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
        data |= 0x20;
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
        data |= 0x40;
        Write8Bit(data, Regs.HL);
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
        data = Read8Bit(Regs.HL);
        data |= 0x80;
        Write8Bit(data, Regs.HL);
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

