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
    return instance;
}

void VHardwareMgr::resetInstance() {
    if (instance != nullptr) {
        CoreBase::resetInstance();
        GraphicsUnitBase::resetInstance();
        Cartridge::resetInstance();
        delete instance;
        instance = nullptr;
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
void VHardwareMgr::ProcessNext() {
    core_instance->RunCycles();
}

/* ***********************************************************************************************************
    VHARDWAREMANAGER SDL FUNCTIONS
*********************************************************************************************************** */
void VHardwareMgr::KeyDown(const SDL_Keycode& _key) {
    switch (_key) {
    default:
        break;
    }
}

void VHardwareMgr::KeyUp(const SDL_Keycode& _key) {
    switch (_key) {
    default:
        break;
    }
}