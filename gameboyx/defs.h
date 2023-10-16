#pragma once

#include <stdint.h>
#include <string>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using debug_instr_data = std::pair<std::string, std::string>;
using register_data = std::pair<std::string, std::string>;

enum memory_data_types {
    MEM_ENTRY_ADDR,
    MEM_ENTRY_LEN,
    MEM_ENTRY_REF
};
using memory_data = std::tuple<std::string, int, u8*>;

struct bank_index {
    int bank;
    int index;

    bank_index(int _bank, int _index) : bank(_bank), index(_index) {};

    constexpr bool operator==(const bank_index& n) const {
        return (n.bank == bank) && (n.index == index);
    }
};