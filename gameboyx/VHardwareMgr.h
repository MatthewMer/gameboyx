#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	This class manages all the emulated hardware and synchronizes it as well as initialization. It takes the game_info struct instance
*	passed by the GUI object and passes it to the constructors of the hardware classes. 
*/

#include <SDL.h>
#include <thread>
#include <mutex>

#include "BaseCPU.h"
#include "BaseCTRL.h"
#include "BaseAPU.h"
#include "BaseGPU.h"
#include "BaseCartridge.h"
#include "defs.h"
#include "general_config.h"
#include "VHardwareStructs.h"

#define VHWMGR_ERR_READ_ROM			0x01
#define VHWMGR_ERR_INIT_THREAD		0x02
#define VHWMGR_ERR_INIT_HW			0x04
#define VHWMGR_ERR_CART_NULL		0x08
#define VHWMGR_ERR_THREAD_RUNNING	0x10

class BaseCartridge;
class GuiMgr;
typedef void (GuiMgr::* gui_callback)();

class VHardwareMgr
{
public:
	static VHardwareMgr* getInstance();
	static void resetInstance();

    u8 InitHardware(BaseCartridge* _cartridge, virtual_graphics_settings& _virt_graphics_settings);
    void ShutdownHardware();
    u8 ResetHardware();

	// members for running hardware
	void ProcessHardware();

	// SDL
	bool EventKeyDown(const SDL_Keycode& _key);
	bool EventKeyUp(const SDL_Keycode& _key);

	u8 SetInitialValues(const bool& _debug_enable, const bool& _pause_execution, const int& _emulation_speed);
	void SetDebugEnabled(const bool& _debug_enabled);
	void SetPauseExecution(const bool& _pause_execution);
	void SetEmulationSpeed(const int& _emulation_speed);
	
	void GetFpsAndClock(float& _clock, float& _fps);
    void GetCurrentPCandBank(u32& _pc, int& _bank);

    void Next(const bool& _next);

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

    // execution time
    u32 timePerFrame;
    u32 currentTimePerFrame;
    steady_clock::time_point timeFramePrev;
    steady_clock::time_point timeFrameCur;

    // timestamps for core frequency calculation
    steady_clock::time_point timeSecondPrev;
    steady_clock::time_point timeSecondCur;
    u32 accumulatedTime;

    u8 errors;

    int buffering = V_TRIPPLE_BUFFERING;

    float currentFrequency;
    float currentFramerate;

    std::thread hardwareThread;
    std::mutex mutHardware;

    std::atomic<bool> running;
    std::atomic<bool> debugEnable;
    std::atomic<bool> pauseExecution;
    std::atomic<int> emulationSpeed;
    std::atomic<bool> next;

    bool CheckDelay();
    void InitMembers();
    void CheckFpsAndClock();
};

