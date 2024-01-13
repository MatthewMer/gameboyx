#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	This class manages all the emulated hardware and synchronizes it as well as initialization. It takes the game_info struct instance
*	passed by the GUI object and passes it to the constructors of the hardware classes. 
*/

#include "Cartridge.h"
#include "CoreBase.h"
#include "ControllerBase.h"
#include "information_structs.h"
#include "SDL.h"

#include "VulkanMgr.h"

class VHardwareMgr
{
public:
	static VHardwareMgr* getInstance(const game_info& _game_ctx, machine_information& _machine_info, VulkanMgr* _graphics_mgr, graphics_information& _graphics_info);
	static void resetInstance();

	// clone/assign protection
	VHardwareMgr(VHardwareMgr const&) = delete;
	VHardwareMgr(VHardwareMgr&&) = delete;
	VHardwareMgr& operator=(VHardwareMgr const&) = delete;
	VHardwareMgr& operator=(VHardwareMgr&&) = delete;

	// members for running hardware
	void ProcessHardware();

	// SDL
	bool EventKeyDown(const SDL_Keycode& _key);
	bool EventKeyUp(const SDL_Keycode& _key);

private:
	// constructor
	VHardwareMgr(const game_info& _game_ctx, machine_information& _machine_info, VulkanMgr* _graphics_mgr, graphics_information& _graphics_info);
	static VHardwareMgr* instance;
	~VHardwareMgr() = default;

	// hardware instances
	CoreBase* core_instance;
	GraphicsUnitBase* graphics_instance;
	ControllerBase* control_instance;
	Cartridge* cart_instance;

	// execution time
	u32 timePerFrame = 0;
	u32 currentTimePerFrame = 0;
	steady_clock::time_point timeFramePrev;
	steady_clock::time_point timeFrameCur;
	bool CheckDelay();

	void InitTime();

	// timestamps for core frequency calculation
	steady_clock::time_point timeSecondPrev;
	steady_clock::time_point timeSecondCur;
	u32 accumulatedTime = 0;
	void CheckFPSandClock();

	// message fifo
	machine_information& machineInfo;
};

