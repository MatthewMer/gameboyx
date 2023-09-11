#include "VHardwareMgr.h"

#include "logger.h"

/* ***********************************************************************************************************
    CONSTRUCTOR AND (DE)INIT
*********************************************************************************************************** */
VHardwareMgr* VHardwareMgr::instance = nullptr;

VHardwareMgr* VHardwareMgr::getInstance(const game_info& _game_ctx) {
    if (instance != nullptr) {
        VHardwareMgr::resetInstance();
    }

    instance = new VHardwareMgr(_game_ctx);
    LOG_INFO("Virtual hardware manager created");
    return instance;
}

void VHardwareMgr::resetInstance() {
    if (instance != nullptr) {
        CoreBase::resetInstance();
        GraphicsUnitBase::resetInstance();
        Cartridge::resetInstance();
        delete instance;
        instance = nullptr;
        LOG_INFO("Virtual hardware manager resetted");
    }
}

VHardwareMgr::VHardwareMgr(const game_info& _game_ctx) {
    cart_instance = Cartridge::getInstance(_game_ctx);

    core_instance = CoreBase::getInstance(*cart_instance);
    graphics_instance = GraphicsUnitBase::getInstance(*cart_instance);
}

/* ***********************************************************************************************************
    FUNCTIONALITY
*********************************************************************************************************** */
void VHardwareMgr::RunHardware() {
    core_instance->RunCycles();
}

