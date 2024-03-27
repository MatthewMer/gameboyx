#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	This class manages all the emulated hardware and synchronizes it as well as initialization. It takes the game_info struct instance
*	passed by the GUI object and passes it to the constructors of the hardware classes. 
*/

#include "BaseCPU.h"
#include "BaseCTRL.h"
#include "BaseAPU.h"
#include "BaseGPU.h"
#include "BaseCartridge.h"
#include "defs.h"
#include "general_config.h"
#include "VHardwareStructs.h"

#include <SDL.h>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
using namespace std::chrono;

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
class GuiMgr;
typedef void (GuiMgr::* debug_callback)(const int&, const int&);

class VHardwareMgr
{
public:
	static VHardwareMgr* getInstance();
	static void resetInstance();

    u8 InitHardware(BaseCartridge* _cartridge, virtual_graphics_settings& _virt_graphics_settings, emulation_settings& _emu_settings, const bool& _reset, GuiMgr* _guimgr, debug_callback _callback);
    u8 StartHardware();
    void ShutdownHardware();

	// members for running hardware
	void ProcessHardware();

	// SDL
    void EventKeyDown(SDL_Keycode& _key);
    void EventKeyUp(SDL_Keycode& _key);

	void SetDebugEnabled(const bool& _debug_enabled);
	void SetProceedExecution(const bool& _proceed_execution);
	void SetEmulationSpeed(const int& _emulation_speed);
	
	void GetFpsAndClock(int& _fps, float& _clock);
    void GetCurrentPCandBank(int& _pc, int& _bank);

    void GetInstrDebugTable(Table<instr_entry>& _table);
    void GetInstrDebugTableTmp(Table<instr_entry>& _table);
    void GetInstrDebugFlags(std::vector<reg_entry>& _reg_values, std::vector<reg_entry>& _flag_values, std::vector<reg_entry>& _misc_values);
    void GetHardwareInfo(std::vector<data_entry>& _hardware_info);
    void GetMemoryDebugTables(std::vector<Table<memory_entry>>& _tables);

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
    u32 timePerFrame;
    u32 currentTimePerFrame;
    steady_clock::time_point timeFramePrev;
    steady_clock::time_point timeFrameCur;

    void Delay();

    int frameCount = 0;
    int clockCount = 0;

    // timestamps for core virtualFrequency and virtualFramerate calculation
    steady_clock::time_point timeSecondPrev;
    steady_clock::time_point timeSecondCur;
    u32 accumulatedTime = 0;
    u32 accumulatedTimeTmp = 0;

    u8 errors;
    bool initialized = false;

    alignas(64) std::atomic<float> currentFrequency = 0;
    alignas(64) std::atomic<float> currentFramerate = 0;

    std::thread hardwareThread;
    std::mutex mutHardware;

    alignas(64) std::atomic<bool> running;
    alignas(64) std::atomic<bool> debugEnable;
    alignas(64) std::atomic<bool> proceedExecution;
    alignas(64) std::atomic<bool> autoRun;
    alignas(64) std::atomic<int> emulationSpeed;

    void CheckFpsAndClock();
    void InitMembers(emulation_settings& _settings);

    GuiMgr* guimgr;
    debug_callback dbgCallback;
};

