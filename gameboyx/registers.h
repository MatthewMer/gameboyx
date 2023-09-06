#pragma once

#include "defs.h"

struct registers_gbc {

	union {						// accumulator and flags
		u16 AF;
		struct {
			u8 F;
			u8 A;
		}AF_;
	};

	union {
		u16 BC;
		struct {
			u8 C;
			u8 B;
		}BC_;
	};

	union {
		u16 DE;
		struct {
			u8 E;
			u8 D;
		}DE_;
	};

	union {
		u16 HL;
		struct {
			u8 L;
			u8 H;
		}HL_;
	};

	u16 SP;						// stack pointer
	u16 PC;						// program counter
};