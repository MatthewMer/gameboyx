#include "VHardwareMgr.h"

#include "logger.h"

/* ***********************************************************************************************************
    CONSTRUCTOR AND (DE)INIT
*********************************************************************************************************** */
VHardwareMgr* VHardwareMgr::instance = nullptr;

VHardwareMgr* VHardwareMgr::getInstance(const game_info& _game_ctx, message_fifo& _msg_fifo) {
    VHardwareMgr::resetInstance();

    instance = new VHardwareMgr(_game_ctx, _msg_fifo);
    return instance;
}

void VHardwareMgr::resetInstance() {
    if (instance != nullptr) {
        CoreBase::resetInstance();
        GraphicsUnitBase::resetInstance();
        Cartridge::resetInstance();
        delete instance;
        instance = nullptr;
        LOG_INFO("---=== game stopped ===---");
    }
}

VHardwareMgr::VHardwareMgr(const game_info& _game_ctx, message_fifo& _msg_fifo) : msgFifo(_msg_fifo){
    cart_instance = Cartridge::getInstance(_game_ctx);
    if (cart_instance == nullptr) {
        LOG_ERROR("Couldn't create virtual cartridge");
        return;
    }

    core_instance = CoreBase::getInstance(*cart_instance, _msg_fifo);
    graphics_instance = GraphicsUnitBase::getInstance(*cart_instance);

    // sets the machine cycle threshold for core and returns the time per frame in ns
    timePerFrame = core_instance->GetDelayTime();

    LOG_INFO("---=== ", _game_ctx.title, " started ===---");
}

/* ***********************************************************************************************************
    FUNCTIONALITY
*********************************************************************************************************** */
void VHardwareMgr::ProcessNext() {
    if (!msgFifo.debug_instructions_enabled) { SimulateDelay(); }

    core_instance->RunCycles();
    if (core_instance->CheckMachineCycles()) {
        // run gpu
        core_instance->ResetMachineCycleCounter();
        // TODO: gpu
    }
}

void VHardwareMgr::SimulateDelay() {
    while (duration_cast<nanoseconds>(cur - prev).count() < timePerFrame) {
        cur = high_resolution_clock::now();
    }
    prev = cur;
}

void VHardwareMgr::InitTime() {
    prev = high_resolution_clock::now();
    cur = high_resolution_clock::now();
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