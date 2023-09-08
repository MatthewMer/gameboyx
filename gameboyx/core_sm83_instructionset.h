#pragma once

#include "defs.h"
#include "registers.h"

enum argument_types {
    NO_ARG,
    BIT16,
	BIT8
};

enum jump_conditions {
	NO_CONDITION,
	ZERO,
	NOT_ZERO,
	CARRY,
	NOT_CARRY
};

struct coresm83_cpu_context {
	u16 data = 0;

	bool ime = false;
	bool cb_instruction = false;
	bool halt = false;

    argument_types arg_type = NO_ARG;
	jump_conditions jump_cond = NO_CONDITION;

	coresm83_registers* reg;
	u8* F = &reg->AF_.F;
	u8* A = &reg->AF_.A;

	u16* PC = &reg->PC;
	u16* SP = &reg->SP;
	u16* stack;

	explicit constexpr coresm83_cpu_context(coresm83_registers& _regs, u16& _stack) : reg(&_regs), stack(&_stack) {};
};