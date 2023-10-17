#include "VHardwareMgr.h"

#include "logger.h"
#include <thread>

using namespace std;

/* ***********************************************************************************************************
    CONSTRUCTOR AND (DE)INIT
*********************************************************************************************************** */
VHardwareMgr* VHardwareMgr::instance = nullptr;

VHardwareMgr* VHardwareMgr::getInstance(const game_info& _game_ctx, machine_information& _machine_info) {
    VHardwareMgr::resetInstance();

    instance = new VHardwareMgr(_game_ctx, _machine_info);
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

VHardwareMgr::VHardwareMgr(const game_info& _game_ctx, machine_information& _machine_info) : machineInfo(_machine_info) {
    cart_instance = Cartridge::getInstance(_game_ctx);
    if (cart_instance == nullptr) {
        LOG_ERROR("Couldn't create virtual cartridge");
        return;
    }

    core_instance = CoreBase::getInstance(_machine_info);
    graphics_instance = GraphicsUnitBase::getInstance();

    // returns the time per frame in ns
    timePerFrame = core_instance->GetDelayTime();

    core_instance->GetStartupHardwareInfo();
    core_instance->GetCurrentRegisterValues();
    core_instance->GetCurrentFlagsAndISR();
    core_instance->GetCurrentProgramCounter();

    core_instance->InitMessageBufferProgram();

    LOG_INFO(_game_ctx.title, " started");
}

/* ***********************************************************************************************************
    FUNCTIONALITY
*********************************************************************************************************** */
void VHardwareMgr::ProcessNext() {
    // run cpu for 1/Display frequency
    core_instance->RunCycles();
    // if machine cycles per frame passed -> render frame
    if (core_instance->CheckNextFrame()) {
        SimulateDelay();
        graphics_instance->NextFrame();
    }

    // get current hardware state
    if (machineInfo.track_hardware_info) {
        core_instance->GetCurrentHardwareState();
        CheckCoreFrequency();
    }

    if (machineInfo.instruction_debug_enabled) {
        core_instance->GetCurrentRegisterValues();
        core_instance->GetCurrentFlagsAndISR();
        core_instance->GetCurrentProgramCounter();
    }
}

void VHardwareMgr::SimulateDelay() {
    while (currentTimePerFrame < timePerFrame) {
        timeFrameCur = high_resolution_clock::now();
        currentTimePerFrame = (u32)duration_cast<nanoseconds>(timeFrameCur - timeFramePrev).count();
    }
    timeFramePrev = timeFrameCur;

    currentTimePerFrame = 0;
}

// get core frequency once per second to stabilize output
void VHardwareMgr::CheckCoreFrequency() {
    timeSecondCur = high_resolution_clock::now();
    accumulatedTime += duration_cast<microseconds>(timeSecondCur - timeSecondPrev).count();
    timeSecondPrev = timeSecondCur;

    if (accumulatedTime > msPerSecondThreshold) {
        core_instance->GetCurrentCoreFrequency();
        accumulatedTime = 0;
    }
}

// init time points for waits/delays
void VHardwareMgr::InitTime() {
    timeFramePrev = high_resolution_clock::now();
    timeFrameCur = high_resolution_clock::now();
    timeSecondPrev = high_resolution_clock::now();
    timeSecondCur = high_resolution_clock::now();
}

/* ***********************************************************************************************************
    VHARDWAREMANAGER SDL FUNCTIONS
*********************************************************************************************************** */
void VHardwareMgr::EventKeyDown(const SDL_Keycode& _key) {
    switch (_key) {
    default:
        break;
    }
}

void VHardwareMgr::EventKeyUp(const SDL_Keycode& _key) {
    switch (_key) {
    default:
        break;
    }
}