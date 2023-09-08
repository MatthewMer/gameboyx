#include "core_sm83_instructionset.h"

/* ***********************************************************************************************************
    INCLUDES
*********************************************************************************************************** */
#include "gameboy_config.h"

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

#define SUB_8_FLAGS(d, s, f) {f &= ~(FLAG_CARRY | FLAG_HCARRY); f |= ((u16)(d & 0xFF) - (u8)(s & 0xFF) > 0xFF ? FLAG_CARRY : 0); f |= ((d & 0xF) - (u8)(s & 0xF) > 0xF ? FLAG_HCARRY : 0);}
#define SUB_8_C(d, s, f) {f &= ~FLAG_CARRY; f |= ((u16)(d & 0xFF) - (u8)(s & 0xFF) > 0xFF ? FLAG_CARRY : 0);}
#define SUB_8_HC(d, s, f) {f &= ~FLAG_HCARRY; f|= ((d & 0xF) - (u8)(s & 0xF) > 0xF ? FLAG_HCARRY : 0);}

#define SBC_8_C(d, s, c, f) {f &= ~FLAG_CARRY; f |= ((u16)(d & 0xFF) - ((u16)(s & 0xFF) + c) > 0xFF ? FLAG_CARRY : 0);}
#define SBC_8_HC(d, s, c, f) {f &= ~FLAG_HCARRY; f |= ((d & 0xF) - ((s & 0xF) + c) > 0xF ? FLAG_HCARRY : 0);}

#define ZERO_FLAG(x, f) {f &= ~FLAG_ZERO; f |= (x == 0 ? FLAG_ZERO : 0);}

/* ***********************************************************************************************************
    HELPER PROTOTYPES
*********************************************************************************************************** */
constexpr void jump_jp(coresm83_cpu_context& _cpu_ctx);
constexpr void jump_jr(coresm83_cpu_context& _cpu_ctx);
constexpr void call(coresm83_cpu_context& _cpu_ctx);
constexpr void ret(coresm83_cpu_context& _cpu_ctx);

/* ***********************************************************************************************************
* 
*   GAMEBOY (COLOR) CORESM83 BASIC INSTRUCTIONSET
*
*********************************************************************************************************** */

/* ***********************************************************************************************************
    CPU CONTROL
*********************************************************************************************************** */
// no operation
void coresm83_instr_nop(coresm83_cpu_context& _cpu_ctx) {}

// stop instruction
void coresm83_instr_stop(coresm83_cpu_context& _cpu_ctx) {
    // TODO
}

// set halt state
void coresm83_instr_halt(coresm83_cpu_context& _cpu_ctx) {
    _cpu_ctx.halt = true;
}

// flip c
void coresm83_instr_ccf(coresm83_cpu_context& _cpu_ctx) {
    RESET_FLAGS(FLAG_HCARRY | FLAG_SUB, *_cpu_ctx.F);
    *_cpu_ctx.F ^= FLAG_CARRY;
}

// set c
void coresm83_instr_scf(coresm83_cpu_context& _cpu_ctx) {
    RESET_FLAGS(FLAG_HCARRY | FLAG_SUB, *_cpu_ctx.F);
    *_cpu_ctx.F |= FLAG_CARRY;
}

// disable interrupts
void coresm83_instr_di(coresm83_cpu_context& _cpu_ctx) {
    _cpu_ctx.ime = false;
}

// eneable interrupts
void coresm83_instr_ei(coresm83_cpu_context& _cpu_ctx) {
    _cpu_ctx.ime = true;
}

// enable CB instructions for next opcode
void coresm83_instr_cb(coresm83_cpu_context& _cpu_ctx) {
    _cpu_ctx.cb_instruction = true;
}

/* ***********************************************************************************************************
    LOAD
*********************************************************************************************************** */
// load 8/16 bit (ldh: address gets ORed with 0xFF00; ldi: load from/to HL/DE/BC reference (+ inc/dec HL))
void coresm83_instr_ld(coresm83_cpu_context& _cpu_ctx) {}

// ld SP to HL and add signed 8 bit immediate data
void coresm83_instr_ld_HLSPr8(coresm83_cpu_context& _cpu_ctx) {
    _cpu_ctx.reg->HL = _cpu_ctx.reg->SP;
    RESET_ALL_FLAGS(*_cpu_ctx.F);
    ADD_8_FLAGS(_cpu_ctx.reg->HL, _cpu_ctx.data, *_cpu_ctx.F);
    _cpu_ctx.reg->HL += *(i8*) & _cpu_ctx.data;
}

// push PC to Stack
void coresm83_instr_push(coresm83_cpu_context& _cpu_ctx) {
    *_cpu_ctx.SP -= 2;
    _cpu_ctx.stack[*_cpu_ctx.SP - STACK_OFFSET] = _cpu_ctx.data;
}

// pop PC from Stack
void coresm83_instr_pop(coresm83_cpu_context& _cpu_ctx) {
    _cpu_ctx.data = _cpu_ctx.stack[*_cpu_ctx.SP - STACK_OFFSET];
    *_cpu_ctx.SP += 2;
}

/* ***********************************************************************************************************
    ARITHMETIC/LOGIC INSTRUCTIONS
*********************************************************************************************************** */
// increment 8/16 bit
void coresm83_instr_inc(coresm83_cpu_context& _cpu_ctx) {
    switch (_cpu_ctx.arg_type) {
    case BIT8:
        RESET_FLAGS(FLAG_ZERO | FLAG_SUB | FLAG_HCARRY, *_cpu_ctx.F);
        ADD_8_HC(_cpu_ctx.data, 1, *_cpu_ctx.F);
        _cpu_ctx.data += 1;
        ZERO_FLAG(_cpu_ctx.data, *_cpu_ctx.F);
        break;
    case BIT16:
        _cpu_ctx.data += 1;
        break;
    default:
        break;
    }
}

// decrement 8/16 bit
void coresm83_instr_dec(coresm83_cpu_context& _cpu_ctx) {
    switch (_cpu_ctx.arg_type) {
    case BIT8:
        RESET_FLAGS(FLAG_ZERO | FLAG_HCARRY, *_cpu_ctx.F);
        SET_FLAGS(FLAG_SUB, *_cpu_ctx.F);
        SUB_8_HC(_cpu_ctx.data, 1, *_cpu_ctx.F);
        _cpu_ctx.data -= 1;
        ZERO_FLAG(_cpu_ctx.data, *_cpu_ctx.F);
        break;
    case BIT16:
        _cpu_ctx.data -= 1;
        break;
    default:
        break;
    }
}

// add with carry + halfcarry
void coresm83_instr_add(coresm83_cpu_context& _cpu_ctx) {
    switch (_cpu_ctx.arg_type) {
    case BIT8:
        RESET_ALL_FLAGS(*_cpu_ctx.F);
        ADD_8_FLAGS(*_cpu_ctx.A, _cpu_ctx.data, *_cpu_ctx.F);
        *_cpu_ctx.A += _cpu_ctx.data;
        ZERO_FLAG(*_cpu_ctx.A, *_cpu_ctx.F);
        break;
    case BIT16:
        RESET_FLAGS(FLAG_SUB | FLAG_HCARRY | FLAG_CARRY, *_cpu_ctx.F);
        ADD_16_FLAGS(_cpu_ctx.reg->HL, _cpu_ctx.data, *_cpu_ctx.F);
        _cpu_ctx.reg->HL += _cpu_ctx.data;
        break;
    default:
        break;
    }
}

// add for SP + signed immediate data r8
void coresm83_instr_add_SPr8(coresm83_cpu_context& _cpu_ctx) {
    RESET_ALL_FLAGS(*_cpu_ctx.F);
    ADD_8_FLAGS(_cpu_ctx.reg->SP, _cpu_ctx.data, *_cpu_ctx.F);
    _cpu_ctx.reg->SP += *(i8*)&_cpu_ctx.data;
}

// adc for A + (register or immediate unsigned data d8) + carry
void coresm83_instr_adc(coresm83_cpu_context& _cpu_ctx) {
    u8 carry = (*_cpu_ctx.F & FLAG_CARRY ? 1 : 0);
    RESET_ALL_FLAGS(*_cpu_ctx.F);
    ADC_8_HC(*_cpu_ctx.A, _cpu_ctx.data, carry, *_cpu_ctx.F);
    ADC_8_C(*_cpu_ctx.A, _cpu_ctx.data, carry, *_cpu_ctx.F);
    *_cpu_ctx.A += _cpu_ctx.data + carry;
    ZERO_FLAG(*_cpu_ctx.A, *_cpu_ctx.F);
}

// sub with carry + halfcarry
void coresm83_instr_sub(coresm83_cpu_context& _cpu_ctx) {
    *_cpu_ctx.F = FLAG_SUB;
    SUB_8_FLAGS(*_cpu_ctx.A, _cpu_ctx.data, *_cpu_ctx.F);
    *_cpu_ctx.A -= _cpu_ctx.data;
    ZERO_FLAG(*_cpu_ctx.A, *_cpu_ctx.F);
}

// adc for A + (register or immediate unsigned data d8) + carry
void coresm83_instr_sbc(coresm83_cpu_context& _cpu_ctx) {
    u8 carry = (*_cpu_ctx.F & FLAG_CARRY ? 1 : 0);
    *_cpu_ctx.F = FLAG_SUB;
    SBC_8_HC(*_cpu_ctx.A, _cpu_ctx.data, carry, *_cpu_ctx.F);
    SBC_8_C(*_cpu_ctx.A, _cpu_ctx.data, carry, *_cpu_ctx.F);
    *_cpu_ctx.A -= (_cpu_ctx.data + carry);
    ZERO_FLAG(*_cpu_ctx.A, *_cpu_ctx.F);
}

// bcd code addition and subtraction
void coresm83_instr_daa(coresm83_cpu_context& _cpu_ctx) {
    if (*_cpu_ctx.F & FLAG_SUB) {
        if (*_cpu_ctx.F & FLAG_CARRY) { *_cpu_ctx.A -= 0x60; }
        if (*_cpu_ctx.F & FLAG_HCARRY) { *_cpu_ctx.A -= 0x06; }
    }
    else {
        if (*_cpu_ctx.F & FLAG_CARRY || *_cpu_ctx.A > 0x99) { 
            *_cpu_ctx.A += 0x60; 
            *_cpu_ctx.F |= FLAG_CARRY; 
        }
        if (*_cpu_ctx.F & FLAG_HCARRY || (*_cpu_ctx.A & 0x0F) > 0x09) {
            *_cpu_ctx.A += 0x06;
        }
    }
    ZERO_FLAG(*_cpu_ctx.A, *_cpu_ctx.F);
    *_cpu_ctx.F &= ~FLAG_HCARRY;
}

// and with hc=1 and z
void coresm83_instr_and(coresm83_cpu_context& _cpu_ctx) {
    *_cpu_ctx.F = FLAG_HCARRY;
    *_cpu_ctx.A &= _cpu_ctx.data;
    ZERO_FLAG(*_cpu_ctx.A, *_cpu_ctx.F);
}

// or with z
void coresm83_instr_or(coresm83_cpu_context& _cpu_ctx) {
    RESET_ALL_FLAGS(*_cpu_ctx.F);
    *_cpu_ctx.A |= _cpu_ctx.data;
    ZERO_FLAG(*_cpu_ctx.A, *_cpu_ctx.F);
}

// xor with z
void coresm83_instr_xor(coresm83_cpu_context& _cpu_ctx) {
    RESET_ALL_FLAGS(*_cpu_ctx.F);
    *_cpu_ctx.A ^= _cpu_ctx.data;
    ZERO_FLAG(*_cpu_ctx.A, *_cpu_ctx.F);
}

// compare (subtraction)
void coresm83_instr_cp(coresm83_cpu_context& _cpu_ctx) {
    *_cpu_ctx.F = FLAG_SUB;
    SUB_8_FLAGS(*_cpu_ctx.A, _cpu_ctx.data, *_cpu_ctx.F);
    *_cpu_ctx.A -= _cpu_ctx.data;
    ZERO_FLAG(*_cpu_ctx.A, *_cpu_ctx.F);
}

// 1's complement of A
void coresm83_instr_cpl(coresm83_cpu_context& _cpu_ctx) {
    SET_FLAGS(FLAG_SUB | FLAG_HCARRY, *_cpu_ctx.F);
    *_cpu_ctx.A = ~(*_cpu_ctx.A);
}

/* ***********************************************************************************************************
    ROTATE AND SHIFT
*********************************************************************************************************** */
// rotate A left with carry
void coresm83_instr_rlca(coresm83_cpu_context& _cpu_ctx) {
    RESET_ALL_FLAGS(*_cpu_ctx.F);
    *_cpu_ctx.F |= (*_cpu_ctx.A & MSB ? FLAG_CARRY : 0x00);
    *_cpu_ctx.A <<= 1;
    *_cpu_ctx.A |= (*_cpu_ctx.F & FLAG_CARRY ? LSB : 0x00);
}

// rotate A right with carry
void coresm83_instr_rrca(coresm83_cpu_context& _cpu_ctx) {
    RESET_ALL_FLAGS(*_cpu_ctx.F);
    *_cpu_ctx.F |= (*_cpu_ctx.A & LSB ? FLAG_CARRY : 0x00);
    *_cpu_ctx.A >>= 1;
    *_cpu_ctx.A |= (*_cpu_ctx.F & FLAG_CARRY ? MSB : 0x00);
}

// rotate A left through carry
void coresm83_instr_rla(coresm83_cpu_context& _cpu_ctx) {
    static bool carry = *_cpu_ctx.F & FLAG_CARRY;
    RESET_ALL_FLAGS(*_cpu_ctx.F);
    *_cpu_ctx.F |= (*_cpu_ctx.A & MSB ? FLAG_CARRY : 0x00);
    *_cpu_ctx.A <<= 1;
    *_cpu_ctx.A |= (carry ? LSB : 0x00);
}

// rotate A right through carry
void coresm83_instr_rra(coresm83_cpu_context& _cpu_ctx) {
    static bool carry = *_cpu_ctx.F & FLAG_CARRY;
    RESET_ALL_FLAGS(*_cpu_ctx.F);
    *_cpu_ctx.F |= (*_cpu_ctx.A & LSB ? FLAG_CARRY : 0x00);
    *_cpu_ctx.A >>= 1;
    *_cpu_ctx.A |= (carry ? MSB : 0x00);
}

/* ***********************************************************************************************************
    JUMP INSTRUCTIONS
*********************************************************************************************************** */
// jump to memory location
void coresm83_instr_jp(coresm83_cpu_context& _cpu_ctx) {
    static bool carry;
    static bool zero;

    switch (_cpu_ctx.jump_cond) {
    case ZERO:
        zero = *_cpu_ctx.F & FLAG_ZERO;
        if (zero) { jump_jp(_cpu_ctx); }
        break;
    case NOT_ZERO:
        zero = *_cpu_ctx.F & FLAG_ZERO;
        if(!zero) { jump_jp(_cpu_ctx); }
        break;
    case CARRY:
        carry = *_cpu_ctx.F & FLAG_CARRY;
        if(carry) { jump_jp(_cpu_ctx); }
        break;
    case NOT_CARRY:
        carry = *_cpu_ctx.F & FLAG_CARRY;
        if(!carry) { jump_jp(_cpu_ctx); }
        break;
    default:
        jump_jp(_cpu_ctx);
        break;
    }
}

// jump relative to memory lecation
void coresm83_instr_jr(coresm83_cpu_context& _cpu_ctx) {
    static bool carry;
    static bool zero;

    switch (_cpu_ctx.jump_cond) {
    case ZERO:
        zero = *_cpu_ctx.F & FLAG_ZERO;
        if (zero) { jump_jr(_cpu_ctx); }
        break;
    case NOT_ZERO:
        zero = *_cpu_ctx.F & FLAG_ZERO;
        if (!zero) { jump_jr(_cpu_ctx); }
        break;
    case CARRY:
        carry = *_cpu_ctx.F & FLAG_CARRY;
        if (carry) { jump_jr(_cpu_ctx); }
        break;
    case NOT_CARRY:
        carry = *_cpu_ctx.F & FLAG_CARRY;
        if (!carry) { jump_jr(_cpu_ctx); }
        break;
    default:
        jump_jr(_cpu_ctx);
        break;
    }
}

constexpr void jump_jp(coresm83_cpu_context& _cpu_ctx) {
    _cpu_ctx.reg->PC = _cpu_ctx.data;
}

constexpr void jump_jr(coresm83_cpu_context& _cpu_ctx) {
    _cpu_ctx.reg->PC += *(i8*)&_cpu_ctx.data;
}

// call routine at memory location
void coresm83_instr_call(coresm83_cpu_context& _cpu_ctx) {
    static bool carry;
    static bool zero;

    switch (_cpu_ctx.jump_cond) {
    case ZERO:
        zero = *_cpu_ctx.F & FLAG_ZERO;
        if (zero) { call(_cpu_ctx); }
        break;
    case NOT_ZERO:
        zero = *_cpu_ctx.F & FLAG_ZERO;
        if (!zero) { call(_cpu_ctx); }
        break;
    case CARRY:
        carry = *_cpu_ctx.F & FLAG_CARRY;
        if (carry) { call(_cpu_ctx); }
        break;
    case NOT_CARRY:
        carry = *_cpu_ctx.F & FLAG_CARRY;
        if (!carry) { call(_cpu_ctx); }
        break;
    default:
        call(_cpu_ctx);
        break;
    }
}

// call to interrupt vectors
void coresm83_instr_rst(coresm83_cpu_context& _cpu_ctx) {
    call(_cpu_ctx);
}

constexpr void call(coresm83_cpu_context& _cpu_ctx) {
    *_cpu_ctx.SP -= 2;
    _cpu_ctx.stack[*_cpu_ctx.SP - STACK_OFFSET] = *_cpu_ctx.PC;
    *_cpu_ctx.PC = _cpu_ctx.data;
}

// return from routine
void coresm83_instr_ret(coresm83_cpu_context& _cpu_ctx) {
    static bool carry;
    static bool zero;

    switch (_cpu_ctx.jump_cond) {
    case ZERO:
        zero = *_cpu_ctx.F & FLAG_ZERO;
        if (zero) { ret(_cpu_ctx); }
        break;
    case NOT_ZERO:
        zero = *_cpu_ctx.F & FLAG_ZERO;
        if (!zero) { ret(_cpu_ctx); }
        break;
    case CARRY:
        carry = *_cpu_ctx.F & FLAG_CARRY;
        if (carry) { ret(_cpu_ctx); }
        break;
    case NOT_CARRY:
        carry = *_cpu_ctx.F & FLAG_CARRY;
        if (!carry) { ret(_cpu_ctx); }
        break;
    default:
        ret(_cpu_ctx);
        break;
    }
}

// return and enable interrupts
void coresm83_instr_reti(coresm83_cpu_context& _cpu_ctx) {
    ret(_cpu_ctx);
    _cpu_ctx.ime = 1;
}

constexpr void ret(coresm83_cpu_context& _cpu_ctx) {
    *_cpu_ctx.PC = _cpu_ctx.stack[*_cpu_ctx.SP - STACK_OFFSET];
    *_cpu_ctx.SP += 2;
}

/* ***********************************************************************************************************
*
*   GAMEBOY (COLOR) CORESM83 CB INSTRUCTIONSET
*
*********************************************************************************************************** */

