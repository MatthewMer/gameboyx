#include "VHardwareMgr.h"

#include "logger.h"

/* ***********************************************************************************************************
    CONSTRUCTOR AND (DE)INIT
*********************************************************************************************************** */
VHardwareMgr* VHardwareMgr::instance = nullptr;

VHardwareMgr* VHardwareMgr::getInstance(const game_info& _game_ctx, message_buffer& _msg_fifo) {
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
    }
}

VHardwareMgr::VHardwareMgr(const game_info& _game_ctx, message_buffer& _msg_fifo) : msgBuffer(_msg_fifo){
    cart_instance = Cartridge::getInstance(_game_ctx);
    if (cart_instance == nullptr) {
        LOG_ERROR("Couldn't create virtual cartridge");
        return;
    }

    core_instance = CoreBase::getInstance(*cart_instance, _msg_fifo);
    graphics_instance = GraphicsUnitBase::getInstance();

    // sets the machine cycle threshold for core and returns the time per frame in ns
    timePerFrame = core_instance->GetDelayTime();

    LOG_INFO(_game_ctx.title, " started");
}

/* ***********************************************************************************************************
    FUNCTIONALITY
*********************************************************************************************************** */
void VHardwareMgr::ProcessNext() {
    // simulate delay (60Hz display frequency)
    SimulateDelay();
    // run cpu for 1/60Hz
    core_instance->RunCycles();
    // if machine cycles per frame passed -> render frame
    if (core_instance->CheckMachineCycles()) {
        graphics_instance->NextFrame();
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