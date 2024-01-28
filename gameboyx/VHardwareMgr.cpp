#include "VHardwareMgr.h"

#include "logger.h"
#include <thread>

using namespace std;

/* ***********************************************************************************************************
    CONSTRUCTOR AND (DE)INIT
*********************************************************************************************************** */
VHardwareMgr* VHardwareMgr::instance = nullptr;
u8 VHardwareMgr::errors = 0x00;

VHardwareMgr* VHardwareMgr::getInstance(machine_information& _machine_info, GraphicsMgr* _graphics_mgr, graphics_information& _graphics_info, AudioMgr* _audio_mgr, audio_information& _audio_info) {
    VHardwareMgr::resetInstance();

    instance = new VHardwareMgr(_machine_info, _graphics_mgr, _graphics_info, _audio_mgr, _audio_info);
    return instance;
}

void VHardwareMgr::resetInstance() {
    if (instance != nullptr) {
        BaseCPU::resetInstance();
        BaseCTRL::resetInstance();
        BaseGPU::resetInstance();
        BaseAPU::resetInstance();
        delete instance;
        instance = nullptr;
    }
}

VHardwareMgr::VHardwareMgr(machine_information& _machine_info, GraphicsMgr* _graphics_mgr, graphics_information& _graphics_info, AudioMgr* _audio_mgr, audio_information& _audio_info) : machineInfo(_machine_info) {
    errors = 0x00;
    
    if (machineInfo.cartridge->ReadRom()) {
        // initialize cpu first -> initializes required memory
        core_instance = BaseCPU::getInstance(_machine_info);
        // afterwards initialize other hardware -> needs initialzed memory
        graphics_instance = BaseGPU::getInstance(_graphics_info, _graphics_mgr);
        sound_instance = BaseAPU::getInstance(_audio_info, _audio_mgr);
        control_instance = BaseCTRL::getInstance(_machine_info);
        // cpu needs references to other hardware to finish setting up, could differ between different emulated platforms
        core_instance->SetHardwareInstances();

        // returns the time per frame in ns
        timePerFrame = graphics_instance->GetDelayTime();

        core_instance->GetCurrentHardwareState();
        core_instance->GetCurrentRegisterValues();
        core_instance->GetCurrentMiscValues();
        core_instance->GetCurrentFlagsAndISR();
        core_instance->GetCurrentProgramCounter();

        core_instance->InitMessageBufferProgram();

        LOG_INFO("[emu] hardware for ", machineInfo.cartridge->title, " initialized");
    } else {
        errors |= VHWMGR_ERR_READ_ROM;
    }
}

/* ***********************************************************************************************************
    FUNCTIONALITY
*********************************************************************************************************** */
void VHardwareMgr::ProcessHardware() {

    if (machineInfo.instruction_debug_enabled) {
        if (!machineInfo.pause_execution) {
            core_instance->RunCycle();
            machineInfo.pause_execution = true;
        }
    }
    else {
        if (CheckDelay()) {
            for (int j = 0; j < machineInfo.emulation_speed; j++) {
                core_instance->RunCycles();
            }
        }
    }

    CheckFPSandClock();

    // get current hardware state
    if (machineInfo.track_hardware_info) {
        core_instance->GetCurrentHardwareState();
    }

    if (machineInfo.instruction_debug_enabled) {
        core_instance->GetCurrentRegisterValues();
        core_instance->GetCurrentMiscValues();
        core_instance->GetCurrentFlagsAndISR();
        core_instance->GetCurrentProgramCounter();
    }
}

bool VHardwareMgr::CheckDelay() {
    timeFrameCur = high_resolution_clock::now();
    currentTimePerFrame = (u32)duration_cast<nanoseconds>(timeFrameCur - timeFramePrev).count();

    if (currentTimePerFrame >= timePerFrame) {
        timeFramePrev = timeFrameCur;
        currentTimePerFrame = 0;

        return true;
    }
    else {
        return false;
    }
}

// get core frequency once per second to stabilize output
void VHardwareMgr::CheckFPSandClock() {
    timeSecondCur = high_resolution_clock::now();
    accumulatedTime += duration_cast<nanoseconds>(timeSecondCur - timeSecondPrev).count();
    timeSecondPrev = timeSecondCur;

    if (accumulatedTime > 999999999) {
        machineInfo.current_frequency = ((float)core_instance->GetCurrentClockCycles() / (accumulatedTime / pow(10,3)));

        machineInfo.current_framerate = graphics_instance->GetFrames() / (accumulatedTime / pow(10,9));

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
bool VHardwareMgr::EventKeyDown(const SDL_Keycode& _key) {
    return control_instance->SetKey(_key);
}

bool VHardwareMgr::EventKeyUp(const SDL_Keycode& _key) {
    return control_instance->ResetKey(_key);
}