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
u8 VHardwareMgr::InitHardware(BaseCartridge* _cartridge, emulation_settings& _emu_settings, const bool& _reset, std::function<void(debug_data&)> _callback) {
    errors = 0x00;
    initialized = false;

    if (_reset) {
        ShutdownHardware();
    }

    if (_cartridge == nullptr) {
        errors |= VHWMGR_ERR_CART_NULL;
    } else {
        if (_cartridge->ReadRom()) {
            cart_instance = _cartridge;

            core_instance = BaseCPU::getInstance(cart_instance);
            graphics_instance = BaseGPU::getInstance(cart_instance);
            sound_instance = BaseAPU::getInstance(cart_instance);
            control_instance = BaseCTRL::getInstance(cart_instance);

            if (cart_instance != nullptr &&
                core_instance != nullptr &&
                graphics_instance != nullptr &&
                sound_instance != nullptr &&
                control_instance != nullptr) {

                core_instance->SetInstances();
                mmu_instance = BaseMMU::getInstance();
                memory_instance = BaseMEM::getInstance();

                // returns the time per frame in ns
                timePerFrame = graphics_instance->GetDelayTime();

                InitMembers(_emu_settings);

                dbgCallback = _callback;
                dbgData = {};
                core_instance->UpdateDebugData(&dbgData);
                dbgCallback(dbgData);

                LOG_INFO("[emu] hardware for ", cart_instance->title, " initialized");
                initialized = true;
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
        LOG_ERROR("[emu] initializing hardware for ", title);
        resetInstance();
    }

    return errors;
}

u8 VHardwareMgr::StartHardware() {
    errors = 0x00;

    if (initialized) {
        hardwareThread = thread([this]() -> void { ProcessHardware(); });
        if (!hardwareThread.joinable()) {
            errors |= VHWMGR_ERR_INIT_THREAD;
        } else {
            LOG_INFO("[emu] hardware for ", cart_instance->title, " started");
        }
    } else {
        errors |= VHWMGR_ERR_HW_NOT_INIT;
    }

    if (errors) {
        string title;
        if (errors | VHWMGR_ERR_CART_NULL) {
            title = N_A;
        } else {
            title = cart_instance->title;
        }
        LOG_ERROR("[emu] starting hardware for ", title);
        resetInstance();
    }

    return errors;
}

void VHardwareMgr::ShutdownHardware() {
    running.store(false);
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

void VHardwareMgr::ProcessHardware() {
    unique_lock<mutex> lock_hardware(mutHardware, defer_lock);

    while (running.load()) {
        if (debugEnable.load()) {
            core_instance->UpdateDebugData(&dbgData);
            dbgCallback(dbgData);

            if (proceedExecution.load()) {
                lock_hardware.lock();
                core_instance->RunCycle();
                lock_hardware.unlock();

                proceedExecution.store(false);
            }
        } else {
            for (int i = 0; i < emulationSpeed.load(); i++) {
                lock_hardware.lock();
                core_instance->RunCycles();
                lock_hardware.unlock();
            }
            Delay();
        }

        CheckFpsAndClock();
    }
}

void VHardwareMgr::InitMembers(emulation_settings& _settings) {
    timeFramePrev = high_resolution_clock::now();
    timeFrameCur = high_resolution_clock::now();
    timeSecondPrev = high_resolution_clock::now();
    timeSecondCur = high_resolution_clock::now();

    running.store(true);
    debugEnable.store(_settings.debug_enabled);
    emulationSpeed.store(_settings.emulation_speed);
}

bool VHardwareMgr::CheckFpsAndClock() {
    timeSecondCur = high_resolution_clock::now();
    accumulatedTime += (u32)duration_cast<microseconds>(timeSecondCur - timeSecondPrev).count();
    timeSecondPrev = timeSecondCur;

    if (accumulatedTime > 999999) {
        clockCount = core_instance->GetClockCycles();
        frameCount = graphics_instance->GetFrameCount();

        currentFrequency.store((float)clockCount / accumulatedTime);
        currentFramerate.store((float)frameCount / (accumulatedTime / (float)pow(10, 6)));

        accumulatedTime = 0;
        core_instance->ResetClockCycles();
        graphics_instance->ResetFrameCount();
        return true;
    }
    return false;
}

void VHardwareMgr::Delay() {
    while (currentTimePerFrame < timePerFrame) {
        timeFrameCur = high_resolution_clock::now();
        currentTimePerFrame = (u32)duration_cast<microseconds>(timeFrameCur - timeFramePrev).count();
    }

    timeFramePrev = timeFrameCur;
    currentTimePerFrame = 0;
}

/* ***********************************************************************************************************
    External interaction (Getters/Setters)
*********************************************************************************************************** */
void VHardwareMgr::GetFpsAndClock(int& _fps, float& _clock) {
    //unique_lock<mutex> lock_hardware(mutHardware);
    _clock = currentFrequency.load();
    _fps = (int)currentFramerate.load();
}
   
void VHardwareMgr::EventButtonDown(const int& _player, const SDL_GameControllerButton& _key) {
    unique_lock<mutex> lock_hardware(mutHardware);
    control_instance->SetKey(_player, _key);
}

void VHardwareMgr::EventButtonUp(const int& _player, const SDL_GameControllerButton& _key) {
    unique_lock<mutex> lock_hardware(mutHardware);
    control_instance->ResetKey(_player, _key);
}

void VHardwareMgr::SetDebugEnabled(const bool& _debug_enabled) {
    debugEnable.store(_debug_enabled);
}

void VHardwareMgr::SetProceedExecution(const bool& _proceed_execution) {
    proceedExecution.store(_proceed_execution);
}

void VHardwareMgr::SetEmulationSpeed(const int& _emulation_speed) {
    emulationSpeed.store(_emulation_speed);
}

void VHardwareMgr::GetInstrDebugTable(Table<instr_entry>& _table) {
    //unique_lock<mutex> lock_hardware(mutHardware);
    core_instance->GetInstrDebugTable(_table);
}

void VHardwareMgr::GetInstrDebugTableTmp(Table<instr_entry>& _table) {
    //unique_lock<mutex> lock_hardware(mutHardware);
    core_instance->GetInstrDebugTableTmp(_table);
}

void VHardwareMgr::GetInstrDebugFlags(std::vector<reg_entry>& _reg_values, std::vector<reg_entry>& _flag_values, std::vector<reg_entry>& _misc_values) {
    //unique_lock<mutex> lock_hardware(mutHardware);
    core_instance->GetInstrDebugFlags(_reg_values, _flag_values, _misc_values);
}

void VHardwareMgr::GetHardwareInfo(std::vector<data_entry>& _hardware_info) {
    //unique_lock<mutex> lock_hardware(mutHardware);
    core_instance->GetHardwareInfo(_hardware_info);
}

void VHardwareMgr::GetMemoryDebugTables(std::vector<Table<memory_entry>>& _tables) {
    //unique_lock<mutex> lock_hardware(mutHardware);
    memory_instance->GetMemoryDebugTables(_tables);
}

void VHardwareMgr::GetGraphicsDebugSettings(std::vector<std::tuple<int, std::string, bool>>& _settings) {
    _settings = graphics_instance->GetGraphicsDebugSettings();
}

void VHardwareMgr::SetGraphicsDebugSetting(const bool& _val, const int& _id) {
    graphics_instance->SetGraphicsDebugSetting(_val, _id);
}

int VHardwareMgr::GetPlayerCount() const {
    return core_instance->GetPlayerCount();
}

void VHardwareMgr::GetMemoryTypes(std::map<int, std::string>& _map) const {
    core_instance->GetMemoryTypes(_map);
}