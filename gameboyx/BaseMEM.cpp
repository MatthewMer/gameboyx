#include "BaseMEM.h"
#include "GameboyMEM.h"

namespace Emulation {
    BaseMEM* BaseMEM::instance = nullptr;

    BaseMEM* BaseMEM::getInstance(BaseCartridge* _cartridge) {
        if (instance == nullptr) {
            switch (_cartridge->console) {
            case GB:
            case GBC:
                instance = new Gameboy::GameboyMEM(_cartridge);
                break;
            }
        }
        return instance;
    }

    BaseMEM* BaseMEM::getInstance() {
        if (instance == nullptr) {
            LOG_ERROR("[emu] MEM instance is nullptr");
        }
        return instance;
    }

    void BaseMEM::resetInstance() {
        if (instance != nullptr) {
            delete instance;
            instance = nullptr;
        }
    }

    std::vector<memory_type_tables>& BaseMEM::GetMemoryTables() {
        return memoryTables;
    }
}