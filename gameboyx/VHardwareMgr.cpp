#include "VHardwareMgr.h"

#include "logger.h"

using namespace std;

namespace Emulation {
    /* ***********************************************************************************************************
        (DE)INIT
    *********************************************************************************************************** */
    std::weak_ptr<VHardwareMgr> VHardwareMgr::m_Instance;

    std::shared_ptr<VHardwareMgr> VHardwareMgr::s_GetInstance() {
        std::shared_ptr<VHardwareMgr> ptr = m_Instance.lock();

        if (!ptr) {
            struct shared_enabler : public VHardwareMgr {};       // workaround for private constructor/destructor
            ptr = std::make_shared<shared_enabler>();
            m_Instance = ptr;
        }

        return ptr;
    }

    void VHardwareMgr::s_ResetInstance() {
        if (m_Instance.lock()) {
            m_Instance.reset();
        }
    }

    VHardwareMgr::~VHardwareMgr() {
        BaseCPU::s_ResetInstance();
        BaseCTRL::s_ResetInstance();
        BaseGPU::s_ResetInstance();
        BaseAPU::s_ResetInstance();
    }

    /* ***********************************************************************************************************
        RUN HARDWARE
    *********************************************************************************************************** */
    u8 VHardwareMgr::InitHardware(emulation_settings& _emu_settings) {
        errors = Errors::NONE;
        initialized = false;

        if (_emu_settings.reset) {
            ShutdownHardware();
        }

        if (_emu_settings.cartridge == nullptr) {
            errors |= Errors::CART_NULL;
        } else {
            m_Cartridge = _emu_settings.cartridge;
            if (m_Cartridge->ReadRom()) {
                m_CoreInstance = BaseCPU::s_GetInstance(m_Cartridge);
                m_MmuInstance = BaseMMU::s_GetInstance(m_Cartridge);
                m_MemInstance = BaseMEM::s_GetInstance(m_Cartridge);
                m_GraphicsInstance = BaseGPU::s_GetInstance(m_Cartridge);
                m_SoundInstance = BaseAPU::s_GetInstance(m_Cartridge);
                m_ControlInstance = BaseCTRL::s_GetInstance(m_Cartridge);

                if (m_CoreInstance != nullptr &&
                    m_MemInstance != nullptr &&
                    m_MmuInstance != nullptr &&
                    m_GraphicsInstance != nullptr &&
                    m_SoundInstance != nullptr &&
                    m_ControlInstance != nullptr) {

                    m_CoreInstance->Init();
                    m_MmuInstance->Init();
                    m_MemInstance->Init();
                    m_GraphicsInstance->Init();
                    m_SoundInstance->Init();
                    m_ControlInstance->Init();

                    // returns the time per frame in ns
                    timePerFrame = std::chrono::microseconds(m_GraphicsInstance->GetDelayTime());

                    InitMembers(_emu_settings);

                    dbgCallback = _emu_settings.callback;
                    dbgData = {};
                    m_CoreInstance->UpdateDebugData(&dbgData);
                    dbgCallback(dbgData);

                    LOG_INFO("[emu] hardware for ", m_Cartridge->title, " initialized");
                    initialized = true;
                } else {
                    errors |= Errors::INIT_HW;
                }
            } else {
                errors |= Errors::READ_ROM;
            }
            m_Cartridge->ClearRom();
        }

        if (errors) {
            string title;
            if (errors | Errors::CART_NULL) {
                title = N_A;
            } else {
                title = m_Cartridge->title;
            }
            LOG_ERROR("[emu] initializing hardware for ", title);
            s_ResetInstance();
        }

        return errors;
    }

    u8 VHardwareMgr::StartHardware() {
        errors = Errors::NONE;

        if (initialized) {
            hardwareThread = thread([this]() -> void { ProcessHardware(); });
            if (!hardwareThread.joinable()) {
                errors |= Errors::INIT_THREAD;
            } else {
                LOG_INFO("[emu] hardware for ", m_Cartridge->title, " started");
            }
        } else {
            errors |= Errors::HW_NOT_INIT;
        }

        if (errors) {
            string title;
            if (errors | Errors::CART_NULL) {
                title = N_A;
            } else {
                title = m_Cartridge->title;
            }
            LOG_ERROR("[emu] starting hardware for ", title);
            s_ResetInstance();
        }

        return errors;
    }

    void VHardwareMgr::ShutdownHardware() {
        running.store(false);
        if (hardwareThread.joinable()) {
            hardwareThread.join();
        }

        m_GraphicsInstance.reset();
        m_SoundInstance.reset();
        m_CoreInstance.reset();
        m_ControlInstance.reset();
        m_MmuInstance.reset();
        m_MemInstance.reset();

        string title;
        if (m_Cartridge == nullptr) {
            title = N_A;
        } else {
            title = m_Cartridge->title;
        }

        LOG_INFO("[emu] hardware for ", title, " stopped");
    }

    void VHardwareMgr::ProcessHardware() {
        unique_lock<mutex> lock_hardware(mutHardware, defer_lock);

        while (running.load()) {
            if (debugEnable.load()) {
                m_CoreInstance->UpdateDebugData(&dbgData);
                dbgCallback(dbgData);

                if (proceedExecution.load()) {
                    lock_hardware.lock();
                    m_CoreInstance->RunCycle();
                    lock_hardware.unlock();

                    proceedExecution.store(false);
                }
            } else {
                for (int i = 0; i < emulationSpeed.load(); i++) {
                    lock_hardware.lock();
                    m_CoreInstance->RunCycles();
                    lock_hardware.unlock();
                }
                Delay();
            }

            CheckFpsAndClock();
        }
    }

    void VHardwareMgr::InitMembers(emulation_settings& _settings) {
        timeSecondPrev = steady_clock::now();
        timeSecondCur = steady_clock::now();
        timePointPrev = steady_clock::now();
        timePointCur = steady_clock::now();

        running.store(true);
        debugEnable.store(_settings.debug_enabled);
        emulationSpeed.store(_settings.emulation_speed);
    }

    // TODO: revise this section
    bool VHardwareMgr::CheckFpsAndClock() {
        timeSecondCur = steady_clock::now();
        accumulatedTime += (u32)duration_cast<microseconds>(timeSecondCur - timeSecondPrev).count();
        timeSecondPrev = timeSecondCur;

        if (accumulatedTime > 999999) {
            clockCount = m_CoreInstance->GetClockCycles();
            frameCount = m_GraphicsInstance->GetFrameCount();

            currentFrequency.store((float)clockCount / accumulatedTime);
            currentFramerate.store((float)frameCount / (accumulatedTime / (float)pow(10, 6)));

            accumulatedTime = 0;
            m_CoreInstance->ResetClockCycles();
            m_GraphicsInstance->ResetFrameCount();
            return true;
        }
        return false;
    }

    void VHardwareMgr::Delay() {
        timePointCur = steady_clock::now();
        std::chrono::microseconds c_time_diff = duration_cast<microseconds>(timePointCur - timePointPrev);

        std::unique_lock<mutex> lock_timedelta(mutTimeDelta);
        notifyTimeDelta.wait_for(lock_timedelta, timePerFrame - c_time_diff);

        timePointPrev = steady_clock::now();
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
        m_ControlInstance->SetKey(_player, _key);
    }

    void VHardwareMgr::EventButtonUp(const int& _player, const SDL_GameControllerButton& _key) {
        unique_lock<mutex> lock_hardware(mutHardware);
        m_ControlInstance->ResetKey(_player, _key);
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

    assembly_tables& VHardwareMgr::GetAssemblyTables() {
        return m_CoreInstance->GetAssemblyTables();
    }

    void VHardwareMgr::GenerateTemporaryAssemblyTable(assembly_tables& _table) {
        m_CoreInstance->GenerateTemporaryAssemblyTable(_table);
    }

    void VHardwareMgr::GetInstrDebugFlags(std::vector<reg_entry>& _reg_values, std::vector<reg_entry>& _flag_values, std::vector<reg_entry>& _misc_values) {
        //unique_lock<mutex> lock_hardware(mutHardware);
        m_CoreInstance->GetInstrDebugFlags(_reg_values, _flag_values, _misc_values);
    }

    void VHardwareMgr::GetHardwareInfo(std::vector<data_entry>& _hardware_info) {
        //unique_lock<mutex> lock_hardware(mutHardware);
        m_CoreInstance->GetHardwareInfo(_hardware_info);
    }

    std::vector<memory_type_tables>& VHardwareMgr::GetMemoryTables() {
        return m_MemInstance->GetMemoryTables();
    }

    void VHardwareMgr::GetGraphicsDebugSettings(std::vector<std::tuple<int, std::string, bool>>& _settings) {
        _settings = m_GraphicsInstance->GetGraphicsDebugSettings();
    }

    void VHardwareMgr::SetGraphicsDebugSetting(const bool& _val, const int& _id) {
        m_GraphicsInstance->SetGraphicsDebugSetting(_val, _id);
    }

    int VHardwareMgr::GetPlayerCount() const {
        return m_CoreInstance->GetPlayerCount();
    }

    void VHardwareMgr::GetMemoryTypes(std::map<int, std::string>& _map) const {
        m_CoreInstance->GetMemoryTypes(_map);
    }
}