#pragma once

#include "general_config.h"
#include <vector>
#include <string>

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