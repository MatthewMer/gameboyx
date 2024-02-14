#pragma once

#include "general_config.h"

enum bufferingMethod {
    V_DOUBLE_BUFFERING = 2,
    V_TRIPPLE_BUFFERING = 3
};

struct virtual_graphics_settings {
    bufferingMethod buffering = V_DOUBLE_BUFFERING;
};