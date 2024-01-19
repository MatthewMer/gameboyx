#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	This class manages all the emulated hardware and synchronizes it as well as initialization. It takes the game_info struct instance
*	passed by the GUI object and passes it to the constructors of the hardware classes. 
*/

#include "GameboyCartridge.h"
#include "BaseCPU.h"
#include "BaseCTRL.h"
#include "BaseAPU.h"
#include "data_containers.h"
#include <SDL.h>
#include "GraphicsMgr.h"
#include "AudioMgr.h"

class VHardwareMgr
{
public:
	static VHardwareMgr* getInstance(machine_information& _machine_info, GraphicsMgr* _graphics_mgr, graphics_information& _graphics_info, AudioMgr* _audio_mgr, audio_information& _audio_info);
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
	VHardwareMgr(machine_information& _machine_info, GraphicsMgr* _graphics_mgr, graphics_information& _graphics_info, AudioMgr* _audio_mgr, audio_information& _audio_info);
	static VHardwareMgr* instance;
	~VHardwareMgr() = default;

	// hardware instances
	BaseCPU* core_instance;
	BaseAPU* sound_instance;
	BaseGPU* graphics_instance;
	BaseCTRL* control_instance;
	GameboyCartridge* cart_instance;

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

