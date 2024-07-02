#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	This class manages all the emulated hardware and synchronizes it as well as initialization. It takes the game_info struct instance
*	passed by the GUI object and passes it to the constructors of the hardware classes. 
*/

#include "HardwareMgr.h"

#include "BaseCPU.h"
#include "BaseCTRL.h"
#include "BaseAPU.h"
#include "BaseGPU.h"
#include "BaseCartridge.h"
#include "defs.h"
#include "general_config.h"
#include "VHardwareTypes.h"

#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
using namespace std::chrono;
using namespace std::chrono_literals;

namespace Emulation {
    struct emulation_settings {
        bool debug_enabled = false;
        int emulation_speed = 1;
    };

#define VHWMGR_ERR_READ_ROM			0x01
#define VHWMGR_ERR_INIT_THREAD		0x02
#define VHWMGR_ERR_INIT_HW			0x04
#define VHWMGR_ERR_CART_NULL		0x08
#define VHWMGR_ERR_THREAD_RUNNING	0x10
#define VHWMGR_ERR_HW_NOT_INIT      0x20

    class BaseCartridge;
    //class GuiMgr;

    class VHardwareMgr {
    public:
        static VHardwareMgr* getInstance();
        static void resetInstance();

        u8 InitHardware(BaseCartridge* _cartridge, emulation_settings& _emu_settings, const bool& _reset, std::function<void(debug_data&)> _callback);
        u8 StartHardware();
        void ShutdownHardware();

        // members for running hardware
        void ProcessHardware();

        // SDL
        void EventButtonDown(const int& _player, const SDL_GameControllerButton& _key);
        void EventButtonUp(const int& _player, const SDL_GameControllerButton& _key);

        void SetDebugEnabled(const bool& _debug_enabled);
        void SetProceedExecution(const bool& _proceed_execution);
        void SetEmulationSpeed(const int& _emulation_speed);

        void GetFpsAndClock(int& _fps, float& _clock);

        assembly_tables& GetAssemblyTables();
        void GenerateTemporaryAssemblyTable(assembly_tables& _table);
        void GetInstrDebugFlags(std::vector<reg_entry>& _reg_values, std::vector<reg_entry>& _flag_values, std::vector<reg_entry>& _misc_values);
        void GetHardwareInfo(std::vector<data_entry>& _hardware_info);
        std::vector<memory_type_tables>& GetMemoryTables();

        void GetGraphicsDebugSettings(std::vector<std::tuple<int, std::string, bool>>& _settings);
        void SetGraphicsDebugSetting(const bool& _val, const int& _id);
        int GetPlayerCount() const;
        void GetMemoryTypes(std::map<int, std::string>& _map) const;

    private:
        VHardwareMgr() = default;
        ~VHardwareMgr();
        static VHardwareMgr* instance;

        // hardware instances
        BaseCPU* core_instance;
        BaseMMU* mmu_instance;
        BaseMEM* memory_instance;
        BaseAPU* sound_instance;
        BaseGPU* graphics_instance;
        BaseCTRL* control_instance;
        BaseCartridge* cart_instance;

        // execution time (e.g. 60FPS -> 1/60th of a second)
        std::chrono::microseconds timePerFrame;
        void Delay();

        int frameCount = 0;
        int clockCount = 0;

        // timestamps for core virtualFrequency and virtualFramerate calculation
        steady_clock::time_point timeSecondPrev;
        steady_clock::time_point timeSecondCur;
        steady_clock::time_point timePointPrev;
        steady_clock::time_point timePointCur;
        u32 accumulatedTime = 0;
        u32 accumulatedTimeTmp = 0;

        u8 errors;
        bool initialized = false;

        alignas(64) std::atomic<float> currentFrequency = 0;
        alignas(64) std::atomic<float> currentFramerate = 0;

        std::thread hardwareThread;
        std::mutex mutHardware;

        std::mutex mutTimeDelta;
        std::condition_variable  notifyTimeDelta;

        alignas(64) std::atomic<bool> running;
        alignas(64) std::atomic<bool> debugEnable;
        alignas(64) std::atomic<bool> proceedExecution;
        alignas(64) std::atomic<bool> autoRun;
        alignas(64) std::atomic<int> emulationSpeed;

        bool CheckFpsAndClock();
        void InitMembers(emulation_settings& _settings);

        std::function<void(debug_data&)> dbgCallback;
        debug_data dbgData;
    };
}