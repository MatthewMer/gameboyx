#pragma once

#include "general_config.h"
#include <vector>
#include <string>

enum bufferingMethod {
    V_DOUBLE_BUFFERING = 2,
    V_TRIPPLE_BUFFERING = 3
};

struct virtual_graphics_settings {
    bufferingMethod buffering = V_DOUBLE_BUFFERING;
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