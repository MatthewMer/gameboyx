#include "CoreSM83.h"

/* ***********************************************************************************************************
    INCLUDES
*********************************************************************************************************** */
#include "gameboy_config.h"

using namespace std;

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

#define SUB_8_FLAGS(d, s, f) {f &= ~(FLAG_CARRY | FLAG_HCARRY); f |= ((u16)(d & 0xFF) - (s & 0xFF) > 0xFF ? FLAG_CARRY : 0); f |= ((d & 0xF) - (s & 0xF) > 0xF ? FLAG_HCARRY : 0);}
#define SUB_8_C(d, s, f) {f &= ~FLAG_CARRY; f |= ((u16)(d & 0xFF) - (s & 0xFF) > 0xFF ? FLAG_CARRY : 0);}
#define SUB_8_HC(d, s, f) {f &= ~FLAG_HCARRY; f|= ((d & 0xF) - (s & 0xF) > 0xF ? FLAG_HCARRY : 0);}

#define SBC_8_C(d, s, c, f) {f &= ~FLAG_CARRY; f |= ((d & 0xFF) - (((u16)s & 0xFF) + c) > 0xFF ? FLAG_CARRY : 0);}
#define SBC_8_HC(d, s, c, f) {f &= ~FLAG_HCARRY; f |= ((d & 0xF) - ((s & 0xF) + c) > 0xF ? FLAG_HCARRY : 0);}

#define ZERO_FLAG(x, f) {f &= ~FLAG_ZERO; f |= (x == 0 ? FLAG_ZERO : 0);}

/* ***********************************************************************************************************
    CONSTRUCTOR
*********************************************************************************************************** */
CoreSM83::CoreSM83(const Cartridge& _cart_obj) {
	Mmu::resetInstance();
	mmu_instance = Mmu::getInstance(_cart_obj);
}

/* ***********************************************************************************************************
*
*   GAMEBOY (COLOR) CORESM83 BASIC INSTRUCTIONSET
*
*********************************************************************************************************** */

/* ***********************************************************************************************************
    CPU CONTROL
*********************************************************************************************************** */
// no operation
void CoreSM83::InstrNOP() {
    return;
}

// stop sm83_instruction
void CoreSM83::InstrSTOP() {
    // TODO
}

// set halt state
void CoreSM83::InstrHALT() {
    halt = true;
}

// flip c
void CoreSM83::InstrCCF() {
    RESET_FLAGS(FLAG_HCARRY | FLAG_SUB, Regs.F);
    Regs.F ^= FLAG_CARRY;
}

// set c
void CoreSM83::InstrSCF() {
    RESET_FLAGS(FLAG_HCARRY | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_CARRY;
}

// disable interrupts
void CoreSM83::InstrDI() {
    ime = false;
}

// eneable interrupts
void CoreSM83::InstrEI() {
    ime = true;
}

// enable CB sm83_instructions for next opcode
void CoreSM83::InstrCB() {
    opcode_cb = true;
}

/* ***********************************************************************************************************
    LOAD
*********************************************************************************************************** */
// load 8/16 bit
void CoreSM83::InstrLDfromA() {
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
        mmu_instance->Write8Bit(Regs.A, Regs.HL);
        Regs.HL--;
        break;
    }
}

void CoreSM83::InstrLDtoAfromRef() {
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
        Regs.A = mmu_instance->Read8Bit(Regs.HL);
        Regs.HL--;
        break;
    }
}

void CoreSM83::InstrLDd8() {
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

void CoreSM83::InstrLDd16() {
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

void CoreSM83::InstrLDHLref() {
    switch (opcode) {
    case 0x46:
        Regs.BC_.B = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x56:
        Regs.DE_.D = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x66:
        Regs.HL_.H = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x4E:
        Regs.BC_.C = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x5E:
        Regs.DE_.E = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x6E:
        Regs.HL_.L = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x7E:
        Regs.A = mmu_instance->Read8Bit(Regs.HL);
        break;
    }
}

void CoreSM83::InstrLDCref() {
    switch (opcode) {
    case 0xE2:
        mmu_instance->Write8Bit(Regs.A, Regs.BC_.C | 0xFF00);
        break;
    case 0xF2:
        Regs.A = mmu_instance->Read8Bit(Regs.BC_.C | 0xFF00);
        break;
    }
}

void CoreSM83::InstrLDH() {
    switch (opcode) {
    case 0xE0:
        mmu_instance->Write8Bit(Regs.A, (data & 0xFF) | 0xFF00);
        break;
    case 0xF0:
        Regs.A = mmu_instance->Read8Bit((data & 0xFF) | 0xFF00);
        break;
    }
}

void CoreSM83::InstrLDtoB() {
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

void CoreSM83::InstrLDtoC() {
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

void CoreSM83::InstrLDtoD() {
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

void CoreSM83::InstrLDtoE() {
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

void CoreSM83::InstrLDtoH() {
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

void CoreSM83::InstrLDtoL() {
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

void CoreSM83::InstrLDtoHLref() {
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

void CoreSM83::InstrLDtoA() {
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
void CoreSM83::InstrHLSPr8() {
    Regs.HL = Regs.SP;
    RESET_ALL_FLAGS(Regs.F);
    ADD_8_FLAGS(Regs.HL, data, Regs.F);
    Regs.HL += *(i8*)&data;
}

// push PC to Stack
void CoreSM83::InstrPUSH() {
    Regs.SP -= 2;
    mmu_instance->Write8Bit(data, Regs.SP);
}

// pop PC from Stack
void CoreSM83::InstrPOP() {
    data = mmu_instance->Read8Bit(Regs.SP);
    Regs.SP += 2;
}

/* ***********************************************************************************************************
    ARITHMETIC/LOGIC sm83_instrUCTIONS
*********************************************************************************************************** */
// increment 8/16 bit
void CoreSM83::InstrINC8() {
    RESET_FLAGS(FLAG_ZERO | FLAG_SUB | FLAG_HCARRY, Regs.F);
    ADD_8_HC(data, 1, Regs.F);
    data += 1;
    ZERO_FLAG(data, Regs.F);
}

void CoreSM83::InstrINC16() {
    data += 1;
}

// decrement 8/16 bit
void CoreSM83::InstrDEC8() {
    RESET_FLAGS(FLAG_ZERO | FLAG_HCARRY, Regs.F);
    SET_FLAGS(FLAG_SUB, Regs.F);
    SUB_8_HC(data, 1, Regs.F);
    data -= 1;
    ZERO_FLAG(data, Regs.F);
}

void CoreSM83::InstrDEC16() {
    data -= 1;
}

// add with carry + halfcarry
void CoreSM83::InstrADD8() {
    RESET_ALL_FLAGS(Regs.F);
    ADD_8_FLAGS(Regs.A, data, Regs.F);
    Regs.A += data;
    ZERO_FLAG(Regs.A, Regs.F);
}

void CoreSM83::InstrADDHL() {
    RESET_FLAGS(FLAG_SUB | FLAG_HCARRY | FLAG_CARRY, Regs.F);
    ADD_16_FLAGS(Regs.HL, data, Regs.F);
    Regs.HL += data;
}

// add for SP + signed immediate data r8
void CoreSM83::InstrADDSPr8() {
    RESET_ALL_FLAGS(Regs.F);
    ADD_8_FLAGS(Regs.SP, data, Regs.F);
    Regs.SP += *(i8*)&data;
}

// adc for A + (register or immediate unsigned data d8) + carry
void CoreSM83::InstrADC() {
    u8 carry = (Regs.F & FLAG_CARRY ? 1 : 0);
    RESET_ALL_FLAGS(Regs.F);
    ADC_8_HC(Regs.A, data, carry, Regs.F);
    ADC_8_C(Regs.A, data, carry, Regs.F);
    Regs.A += data + carry;
    ZERO_FLAG(Regs.A, Regs.F);
}

// sub with carry + halfcarry
void CoreSM83::InstrSUB() {
    Regs.F = FLAG_SUB;
    SUB_8_FLAGS(Regs.A, data, Regs.F);
    Regs.A -= data;
    ZERO_FLAG(Regs.A, Regs.F);
}

// adc for A + (register or immediate unsigned data d8) + carry
void CoreSM83::InstrSBC() {
    u8 carry = (Regs.F & FLAG_CARRY ? 1 : 0);
    Regs.F = FLAG_SUB;
    SBC_8_HC(Regs.A, data, carry, Regs.F);
    SBC_8_C(Regs.A, data, carry, Regs.F);
    Regs.A -= (data + carry);
    ZERO_FLAG(Regs.A, Regs.F);
}

// bcd code addition and subtraction
void CoreSM83::InstrDAA() {
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
void CoreSM83::InstrAND() {
    Regs.F = FLAG_HCARRY;
    Regs.A &= data;
    ZERO_FLAG(Regs.A, Regs.F);
}

// or with z
void CoreSM83::InstrOR() {
    RESET_ALL_FLAGS(Regs.F);
    Regs.A |= data;
    ZERO_FLAG(Regs.A, Regs.F);
}

// xor with z
void CoreSM83::InstrXOR() {
    RESET_ALL_FLAGS(Regs.F);
    Regs.A ^= data;
    ZERO_FLAG(Regs.A, Regs.F);
}

// compare (subtraction)
void CoreSM83::InstrCP() {
    Regs.F = FLAG_SUB;
    SUB_8_FLAGS(Regs.A, data, Regs.F);
    Regs.A -= data;
    ZERO_FLAG(Regs.A, Regs.F);
}

// 1's complement of A
void CoreSM83::InstrCPL() {
    SET_FLAGS(FLAG_SUB | FLAG_HCARRY, Regs.F);
    Regs.A = ~(Regs.A);
}

/* ***********************************************************************************************************
    ROTATE AND SHIFT
*********************************************************************************************************** */
// rotate A left with carry
void CoreSM83::InstrRLCA() {
    RESET_ALL_FLAGS(Regs.F);
    Regs.F |= (Regs.A & MSB ? FLAG_CARRY : 0x00);
    Regs.A <<= 1;
    Regs.A |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
}

// rotate A right with carry
void CoreSM83::InstrRRCA() {
    RESET_ALL_FLAGS(Regs.F);
    Regs.F |= (Regs.A & LSB ? FLAG_CARRY : 0x00);
    Regs.A >>= 1;
    Regs.A |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
}

// rotate A left through carry
void CoreSM83::InstrRLA() {
    static bool carry = Regs.F & FLAG_CARRY;
    RESET_ALL_FLAGS(Regs.F);
    Regs.F |= (Regs.A & MSB ? FLAG_CARRY : 0x00);
    Regs.A <<= 1;
    Regs.A |= (carry ? LSB : 0x00);
}

// rotate A right through carry
void CoreSM83::InstrRRA() {
    static bool carry = Regs.F & FLAG_CARRY;
    RESET_ALL_FLAGS(Regs.F);
    Regs.F |= (Regs.A & LSB ? FLAG_CARRY : 0x00);
    Regs.A >>= 1;
    Regs.A |= (carry ? MSB : 0x00);
}

/* ***********************************************************************************************************
    JUMP sm83_instrUCTIONS
*********************************************************************************************************** */
// jump to memory location
void CoreSM83::InstrJP() {
    static bool carry;
    static bool zero;

    switch (opcode) {
    case 0xCA:
        zero = Regs.F & FLAG_ZERO;
        if (zero) { jump_jp(); }
        break;
    case 0xC2:
        zero = Regs.F & FLAG_ZERO;
        if (!zero) { jump_jp(); }
        break;
    case 0xDA:
        carry = Regs.F & FLAG_CARRY;
        if (carry) { jump_jp(); }
        break;
    case 0xD2:
        carry = Regs.F & FLAG_CARRY;
        if (!carry) { jump_jp(); }
        break;
    default:
        jump_jp();
        break;
    }
}

// jump relative to memory lecation
void CoreSM83::InstrJR() {
    static bool carry;
    static bool zero;

    switch (opcode) {
    case 0x28:
        zero = Regs.F & FLAG_ZERO;
        if (zero) { jump_jr(); }
        break;
    case 0x20:
        zero = Regs.F & FLAG_ZERO;
        if (!zero) { jump_jr(); }
        break;
    case 0x38:
        carry = Regs.F & FLAG_CARRY;
        if (carry) { jump_jr(); }
        break;
    case 0x30:
        carry = Regs.F & FLAG_CARRY;
        if (!carry) { jump_jr(); }
        break;
    default:
        jump_jr();
        break;
    }
}

void CoreSM83::jump_jp() {
    Regs.PC = data;
}

void CoreSM83::jump_jr() {
    Regs.PC += *(i8*)&data;
}

// call routine at memory location
void CoreSM83::InstrCALL() {
    static bool carry;
    static bool zero;

    switch (opcode) {
    case 0xCC:
        zero = Regs.F & FLAG_ZERO;
        if (zero) { call(); }
        break;
    case 0xC4:
        zero = Regs.F & FLAG_ZERO;
        if (!zero) { call(); }
        break;
    case 0xDC:
        carry = Regs.F & FLAG_CARRY;
        if (carry) { call(); }
        break;
    case 0xD4:
        carry = Regs.F & FLAG_CARRY;
        if (!carry) { call(); }
        break;
    default:
        call();
        break;
    }
}

// call to interrupt vectors
void CoreSM83::InstrRST() {
    // TODO
    call();
}

void CoreSM83::call() {
    Regs.SP -= 2;
    mmu_instance->Write16Bit(Regs.PC, Regs.SP);
    Regs.PC = data;
}

// return from routine
void CoreSM83::InstrRET() {
    static bool carry;
    static bool zero;

    switch (opcode) {
    case 0xC8:
        zero = Regs.F & FLAG_ZERO;
        if (zero) { ret(); }
        break;
    case 0xC0:
        zero = Regs.F & FLAG_ZERO;
        if (!zero) { ret(); }
        break;
    case 0xD8:
        carry = Regs.F & FLAG_CARRY;
        if (carry) { ret(); }
        break;
    case 0xD0:
        carry = Regs.F & FLAG_CARRY;
        if (!carry) { ret(); }
        break;
    default:
        ret();
        break;
    }
}

// return and enable interrupts
void CoreSM83::InstrRETI() {
    ret();
    ime = true;
}

void CoreSM83::ret() {
    Regs.PC = mmu_instance->Read16Bit(Regs.SP);
    Regs.SP += 2;
}

/* ***********************************************************************************************************
*
*   GAMEBOY (COLOR) CORESM83 CB sm83_instrUCTIONSET
*
*********************************************************************************************************** */

