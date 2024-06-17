#pragma once

#include "general_config.h"
#include <vector>
#include <string>

namespace Emulation {
    struct virtual_graphics_settings {

    };

    struct callstack_data {
        int src_mem_type;
        int src_bank;
        int src_addr;
        int dest_mem_type;
        int dest_bank;
        int dest_addr;
    };

    struct debug_data {
        int pc = 0;
        int bank = 0;

        std::vector<callstack_data> callstack;
    };

    enum {
        DATA_NAME,
        DATA_VAL
    };
    using data_entry = std::pair<std::string, std::string>;

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

    enum console_ids {
        CONSOLE_NONE,
        GB,
        GBC
    };

    inline const std::unordered_map<console_ids, std::pair<std::string, std::string>> FILE_EXTS = {
        { GB, {"Gameboy", "gb"} },
        { GBC, { "Gameboy Color", "gbc"} }
    };
}