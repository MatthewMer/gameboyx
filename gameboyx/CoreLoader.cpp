#include "CoreLoader.h"

#include "HardwareMgr.h"
#include "helper_functions.h"
#include "logger.h"

#include <windows.h>


namespace Emulation {
    std::vector<std::pair<console_ids, functions>> CoreLoader::loadedCores = {};

    inline const std::string DLL_EXT = "dll";

    inline const std::vector<std::string> FUNCTION_NAMES = {
        "GetConsoleIDs"
    };

    std::string get_function_name(std::string _in){
        auto it = std::find_if(FUNCTION_NAMES.begin(), FUNCTION_NAMES.end(), [&_in](const std::string& _e) { return _e.compare(_in) == 0; });
        if (it != FUNCTION_NAMES.end()) {
            return *it;
        } else {
            LOG_ERROR("[DLL Loader] Function not found in core: ", _in);
            return {};
        }
    }

    bool check_function_pointer(const func_ptr _fn_ptr, const std::string& _fn_name) {
        try {
            if (!_fn_ptr) { throw _fn_name; }
            return true;
        } catch (std::string fn_name) {
            LOG_ERROR("[DLL Loader] Could not acquire pointer to function: ", fn_name);
            return false;
        }
    }

    void CoreLoader::LoadCores(std::string _dll_folder) {
        std::vector<std::string> dlls;
        Backend::FileIO::get_files_in_path(dlls, _dll_folder);

        for (auto& path : dlls) {
            if (Helpers::split_string(path, ".").back().compare(DLL_EXT) != 0) {
                continue;
            }

            const size_t c_size = strlen(path.c_str()) + 1;
            wchar_t* lpcw_str = new wchar_t[c_size];
            mbstowcs(lpcw_str, path.c_str(), c_size);

            HINSTANCE hGetProcIDDLL = LoadLibrary(lpcw_str);
            if (!hGetProcIDDLL) {
                LOG_ERROR("[DLL Loader] Could not load core: ", path);
                continue;
            }

            GetConsoleIDs_ptr fn_console_id = (GetConsoleIDs_ptr)GetProcAddress(hGetProcIDDLL, get_function_name("GetConsoleIDs").c_str());
            if (!check_function_pointer((func_ptr)fn_console_id, "GetConsoleIDs")) {
                continue;
            }
            
            auto ids = fn_console_id();
            for (const auto& id : ids) {
                if (std::find_if(loadedCores.begin(), loadedCores.end(), [&id](std::pair<console_ids, functions>& _e) { return _e.first == id; }) == loadedCores.end()) {

                    loadedCores.emplace_back();
                    auto& core_data = loadedCores.back();
                    core_data.first = id;
                    auto& core_fn = core_data.second;
                    for (size_t i = 0; i < FUNCTION_NAMES.size(); i++) {
                        core_fn.ptrs[i] = (func_ptr)GetProcAddress(hGetProcIDDLL, FUNCTION_NAMES[i].c_str());

                        if (!check_function_pointer(core_fn.ptrs[i], FUNCTION_NAMES[i])) {
                            // TODO: further error handling
                            continue;
                        }
                    }

                } else if(FILE_EXTS.find(id) != FILE_EXTS.end()) {
                    LOG_WARN("[DLL Loader] Core has already been loaded: ", FILE_EXTS.at(id).first);
                } else {
                    LOG_WARN("[DLL Loader] Could not find requested core: ", id);
                }
            }
        }
    }
}