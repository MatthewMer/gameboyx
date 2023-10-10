#pragma once

#include "Cartridge.h"
#include "CoreBase.h"
#include "GraphicsUnitBase.h"
#include "information_structs.h"
#include "SDL.h"


class VHardwareMgr
{
public:
	static VHardwareMgr* getInstance(const game_info& _game_ctx, machine_information& _machine_info);
	static void resetInstance();

	// clone/assign protection
	VHardwareMgr(VHardwareMgr const&) = delete;
	VHardwareMgr(VHardwareMgr&&) = delete;
	VHardwareMgr& operator=(VHardwareMgr const&) = delete;
	VHardwareMgr& operator=(VHardwareMgr&&) = delete;

	// members for running hardware
	void ProcessNext();

	// SDL
	void EventKeyDown(const SDL_Keycode& _key);
	void EventKeyUp(const SDL_Keycode& _key);

private:
	// constructor
	VHardwareMgr(const game_info& _game_ctx, machine_information& _machine_info);
	static VHardwareMgr* instance;
	~VHardwareMgr() = default;

	// hardware instances
	CoreBase* core_instance;
	GraphicsUnitBase* graphics_instance;
	Cartridge* cart_instance;

	// execution time
	u32 timePerFrame = 0;
	u32 currentTimePerFrame = 0;
	steady_clock::time_point timeFramePrev;
	steady_clock::time_point timeFrameCur;
	void SimulateDelay();

	void InitTime();

	// timestamps for core frequency calculation
	const int msPerSecondThreshold = 999999;					// constant to measure 1 second
	steady_clock::time_point timeSecondPrev;
	steady_clock::time_point timeSecondCur;
	u32 accumulatedTime = 0;
	void CheckCoreFrequency();

	// message fifo
	machine_information& machineInfo;
};

