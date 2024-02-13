#include "VHardwareMgr.h"

#include "logger.h"


using namespace std;

/* ***********************************************************************************************************
    (DE)INIT
*********************************************************************************************************** */
VHardwareMgr* VHardwareMgr::instance = nullptr;

VHardwareMgr* VHardwareMgr::getInstance() {
    if (instance == nullptr) {
        instance = new VHardwareMgr();
    }

    return instance;
}

void VHardwareMgr::resetInstance() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

VHardwareMgr::~VHardwareMgr() {
    BaseCPU::resetInstance();
    BaseCTRL::resetInstance();
    BaseGPU::resetInstance();
    BaseAPU::resetInstance();
}

/* ***********************************************************************************************************
    RUN HARDWARE
*********************************************************************************************************** */
u8 VHardwareMgr::InitHardware(BaseCartridge* _cartridge) {
    errors = 0x00;

    if (_cartridge == nullptr) {
        errors |= VHWMGR_ERR_CART_NULL;
    } else {
        if (_cartridge->ReadRom()) {
            cart_instance = _cartridge;

            // initialize cpu first -> initializes required memory
            core_instance = BaseCPU::getInstance(cart_instance);
            // afterwards initialize other hardware -> needs initialzed memory
            graphics_instance = BaseGPU::getInstance(cart_instance);
            sound_instance = BaseAPU::getInstance(cart_instance);
            control_instance = BaseCTRL::getInstance(cart_instance);
            // cpu needs references to other hardware to finish setting up, could differ between different emulated platforms

            if (cart_instance != nullptr &&
                core_instance != nullptr &&
                graphics_instance != nullptr &&
                sound_instance != nullptr &&
                control_instance != nullptr) {

                // returns the time per frame in ns
                timePerFrame = graphics_instance->GetDelayTime();

                InitMembers();

                hardwareThread = thread([this] { ProcessHardware(); });
                if (!hardwareThread.joinable()) {
                    errors |= VHWMGR_ERR_INIT_THREAD;
                } else {
                    LOG_INFO("[emu] hardware for ", cart_instance->title, " initialized");
                }
            } else {
                errors |= VHWMGR_ERR_INIT_HW;
            }
        } else {
            errors |= VHWMGR_ERR_READ_ROM;
            _cartridge->ClearRom();
        }
    }

    if (errors) {
        string title;
        if (errors | VHWMGR_ERR_CART_NULL) {
            title = N_A;
        } else {
            title = cart_instance->title;
        }
        LOG_ERROR("[emu] starting game ", title);
        resetInstance();
    }

    return errors;
}

void VHardwareMgr::ShutdownHardware() {
    unique_lock<mutex> lock_run(mutRun);
    running = false;
    lock_run.unlock();

    if (hardwareThread.joinable()) {
        hardwareThread.join();
    }

    BaseCTRL::resetInstance();
    BaseAPU::resetInstance();
    BaseGPU::resetInstance();
    BaseCPU::resetInstance();
    
    string title;
    if (cart_instance == nullptr) {
        title = N_A;
    } else {
        title = cart_instance->title;
    }

    LOG_INFO("[emu] hardware for ", title, " stopped");
}

u8 VHardwareMgr::ResetHardware() {
    errors = 0x00;



    return errors;
}

void VHardwareMgr::ProcessHardware() {
    unique_lock<mutex> lock_run(mutRun, defer_lock);
    unique_lock<mutex> lock_debug(mutDebug, defer_lock);
    unique_lock<mutex> lock_pause(mutPause, defer_lock);
    unique_lock<mutex> lock_speed(mutSpeed, defer_lock);
    unique_lock<mutex> lock_hardware(mutHardware, defer_lock);
    
    lock_run.lock();
    while (running) {
        lock_run.unlock();

        lock_debug.lock();
        if (debugEnable) {
            lock_debug.unlock();

            lock_pause.lock();
            if (!pauseExecution) {

                lock_hardware.lock();
                core_instance->RunCycle();
                CheckFpsAndClock();
                lock_hardware.unlock();

                pauseExecution = true;
            }
            lock_pause.unlock();
        } else {
            lock_debug.unlock();

            if (CheckDelay()) {

                lock_speed.lock();
                for (int j = 0; j < emulationSpeed; j++) {
                    lock_speed.unlock();

                    lock_hardware.lock();
                    core_instance->RunCycles();
                    CheckFpsAndClock();
                    lock_hardware.unlock();

                    lock_speed.lock();
                }
                lock_speed.unlock();
            }
        }

        lock_run.lock();
    }
}

bool VHardwareMgr::CheckDelay() {
    timeFrameCur = high_resolution_clock::now();
    currentTimePerFrame = (u32)duration_cast<milliseconds>(timeFrameCur - timeFramePrev).count();

    if (currentTimePerFrame >= timePerFrame) {
        timeFramePrev = timeFrameCur;
        currentTimePerFrame = 0;

        return true;
    } else {
        return false;
    }
}

void VHardwareMgr::InitMembers() {
    timeFramePrev = high_resolution_clock::now();
    timeFrameCur = high_resolution_clock::now();
    timeSecondPrev = high_resolution_clock::now();
    timeSecondCur = high_resolution_clock::now();

    running = true;
    debugEnable = false;
    pauseExecution = false;
    emulationSpeed = 1;
}

void VHardwareMgr::CheckFpsAndClock() {
    timeSecondCur = high_resolution_clock::now();
    accumulatedTime += (u32)duration_cast<microseconds>(timeSecondCur - timeSecondPrev).count();
    timeSecondPrev = timeSecondCur;

    if (accumulatedTime > 999999) {
        currentFrequency = ((float)core_instance->GetCurrentClockCycles() / accumulatedTime);
        currentFramerate = graphics_instance->GetFrames() / (accumulatedTime / (float)pow(10, 9));

        accumulatedTime = 0;
    }
}

/* ***********************************************************************************************************
    External interaction (Getters/Setters)
*********************************************************************************************************** */
u8 VHardwareMgr::SetInitialValues(const bool& _debug_enable, const bool& _pause_execution, const int& _emulation_speed) {
    errors = 0x00;

    if (hardwareThread.joinable()) {
        errors |= VHWMGR_ERR_THREAD_RUNNING;
    } else {
        debugEnable = _debug_enable;
        pauseExecution = _pause_execution;
        emulationSpeed = _emulation_speed;
    }

    return errors;
}

void VHardwareMgr::GetFpsAndClock(float& _clock, float& _fps) {
    unique_lock<mutex> lock_hardware(mutHardware);
    _clock = currentFrequency;
    _fps = currentFramerate;
}

void VHardwareMgr::GetCurrentPCandBank(u32& _pc, int& _bank) {
    unique_lock<mutex> lock_hardware(mutHardware);
    core_instance->GetBankAndPC(_bank, _pc);
}

bool VHardwareMgr::EventKeyDown(const SDL_Keycode& _key) {
    unique_lock<mutex> lock_hardware(mutHardware);
    return control_instance->SetKey(_key);
}

bool VHardwareMgr::EventKeyUp(const SDL_Keycode& _key) {
    unique_lock<mutex> lock_hardware(mutHardware);
    return control_instance->ResetKey(_key);
}

void VHardwareMgr::SetDebugEnabled(const bool& _debug_enabled) {
    unique_lock<mutex> lock_debug(mutDebug);
    debugEnable = _debug_enabled;
}

void VHardwareMgr::SetPauseExecution(const bool& _pause_execution) {
    unique_lock<mutex> lock_pause(mutPause);
    pauseExecution = _pause_execution;
}

void VHardwareMgr::SetEmulationSpeed(const int& _emulation_speed) {
    unique_lock<mutex> lock_speed(mutSpeed);
    emulationSpeed = _emulation_speed;
}