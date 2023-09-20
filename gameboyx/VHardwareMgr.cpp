#include "VHardwareMgr.h"

#include "logger.h"
#include <thread>

using namespace std;

/* ***********************************************************************************************************
    CONSTRUCTOR AND (DE)INIT
*********************************************************************************************************** */
VHardwareMgr* VHardwareMgr::instance = nullptr;

VHardwareMgr* VHardwareMgr::getInstance(const game_info& _game_ctx, message_buffer& _msg_buffer) {
    VHardwareMgr::resetInstance();

    instance = new VHardwareMgr(_game_ctx, _msg_buffer);
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

VHardwareMgr::VHardwareMgr(const game_info& _game_ctx, message_buffer& _msg_buffer) : msgBuffer(_msg_buffer){
    cart_instance = Cartridge::getInstance(_game_ctx);
    if (cart_instance == nullptr) {
        LOG_ERROR("Couldn't create virtual cartridge");
        return;
    }

    core_instance = CoreBase::getInstance(_msg_buffer);
    graphics_instance = GraphicsUnitBase::getInstance();

    // sets the machine cycle threshold for core and returns the time per frame in ns
    timePerFrame = core_instance->GetDelayTime();
    displayFrequency = core_instance->GetDisplayFrequency();

    core_instance->GetStartupHardwareInfo(msgBuffer);

    this_thread::sleep_for(std::chrono::milliseconds(100));
    LOG_INFO(_game_ctx.title, " started");
}

/* ***********************************************************************************************************
    FUNCTIONALITY
*********************************************************************************************************** */
void VHardwareMgr::ProcessNext() {
    // simulate delay
    SimulateDelay();

    // run cpu for 1/Display frequency
    core_instance->RunCycles();
    // if machine cycles per frame passed -> render frame
    if (core_instance->CheckMachineCycles()) {
        graphics_instance->NextFrame();
    }
    
    // get current hardware state
    if (msgBuffer.track_hardware_info) {
        core_instance->GetCurrentHardwareState(msgBuffer);
    }
}

void VHardwareMgr::SimulateDelay() {
    while (currentTimePerFrame < timePerFrame) {
        cur = high_resolution_clock::now();
        currentTimePerFrame = duration_cast<nanoseconds>(cur - prev).count();
    }
    prev = cur;

    if (msgBuffer.track_hardware_info) {
        GetCurrentCoreFrequency();
    }

    currentTimePerFrame = 0;
}

// calculate current core frequency
void VHardwareMgr::GetCurrentCoreFrequency() {
    timeDelta += currentTimePerFrame;
    timeDeltaCounter++;

    if (timeDeltaCounter == displayFrequency) {
        msgBuffer.current_frequency = (((float)core_instance->GetPassedClockCycles() / ((float)timeDelta / timeDeltaCounter)) / displayFrequency) * 1000;
        timeDelta = 0;
        timeDeltaCounter = 0;
    }
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