#include "BaseMEM.h"
#include "GameboyMEM.h"

BaseMEM* BaseMEM::instance = nullptr;

BaseMEM* BaseMEM::getInstance(BaseCartridge* _cartridge) {
    if (instance == nullptr) {
        switch (_cartridge->console) {
        case GB:
        case GBC:
            instance = new GameboyMEM(_cartridge);
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