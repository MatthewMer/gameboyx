#pragma once
/*
*	different defines for random usage and speed up writing
*/

#include <stdint.h>
#include <string>

#define N_A                         "N/A"

#define PARAMETER_TRUE              "true"
#define PARAMETER_FALSE             "false"

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

struct Vec2 {
    int x;
    int y;

    Vec2(int x, int y) : x(x), y(y) {};
};

enum instr_entry_types {
	INSTR_HEX,
	INSTR_ASM
};
using instr_entry = std::pair<std::string, std::string>;

enum reg_entry_types {
	REG_NAME,
	REG_DATA
};
using reg_entry = std::pair<std::string, std::string>;

enum memory_data_types {
	MEM_ENTRY_ADDR,
	MEM_ENTRY_LEN,
	MEM_ENTRY_REF
};
using memory_entry = std::tuple<std::string, int, u8*>;