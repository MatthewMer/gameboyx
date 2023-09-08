#pragma once

#include "defs.h"

// unions not allow for type-punning by the C++ standard but works fine with g++, Clang and MSVC (MSVC even uses it in its headers)
struct gbc_registers {

	u8 F;
	u8 A;

	union {
		u16 BC = 0;
		struct {
			u8 C;
			u8 B;
		}BC_;
	};

	union {
		u16 DE = 0;
		struct {
			u8 E;
			u8 D;
		}DE_;
	};

	union {
		u16 HL = 0;
		struct {
			u8 L;
			u8 H;
		}HL_;
	};

	u16 SP = 0;						// stack pointer
	u16 PC = 0;						// program counter
};