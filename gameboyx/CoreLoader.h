#pragma once

#include "VHardwareTypes.h"

#include <string.h>
#include <vector>

namespace Emulation {
    inline const size_t FUNC_NUM = 1;

    // generic function pointer type
    typedef void(__stdcall* func_ptr)();

    typedef std::vector<console_ids>(__stdcall* GetConsoleIDs_ptr)();

    struct function_ptrs {
        std::vector<console_ids>(__stdcall* GetConsoleIDs_ptr)();
    };

    union functions {
        function_ptrs by_type;
        func_ptr ptrs[FUNC_NUM];
    };

    class CoreLoader {
    public:
        static void LoadCores(std::string _dll_path);

    private:
        CoreLoader() = delete;
        ~CoreLoader() = delete;

        static std::vector<std::pair<console_ids, functions>> loadedCores;
    };
}