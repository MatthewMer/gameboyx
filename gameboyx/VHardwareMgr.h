#pragma once

#include "Cartridge.h"
#include "CoreBase.h"
#include "GraphicsUnitBase.h"
#include "information_structs.h"
#include "SDL.h"


class VHardwareMgr
{
public:
	static VHardwareMgr* getInstance(const game_info& _game_ctx, message_buffer& _msg_buffer);
	static void resetInstance();

	// clone/assign protection
	VHardwareMgr(VHardwareMgr const&) = delete;
	VHardwareMgr(VHardwareMgr&&) = delete;
	VHardwareMgr& operator=(VHardwareMgr const&) = delete;
	VHardwareMgr& operator=(VHardwareMgr&&) = delete;

	// members for running hardware
	void ProcessNext();

	// SDL
	void KeyDown(const SDL_Keycode& _key);
	void KeyUp(const SDL_Keycode& _key);

private:
	// constructor
	explicit VHardwareMgr(const game_info& _game_ctx, message_buffer& _msg_buffer);
	static VHardwareMgr* instance;
	~VHardwareMgr() = default;

	// hardware instances
	CoreBase* core_instance;
	GraphicsUnitBase* graphics_instance;
	Cartridge* cart_instance;

	// execution time
	u32 timePerFrame = 0;
	u32 currentTimePerFrame = 0;
	u64 timeDelta = 0;
	int timeDeltaCounter = 0;
	int timeDeltaNumIncrements = 0;
	steady_clock::time_point prev;
	steady_clock::time_point cur;
	u32 displayFrequency = 0;
	void SimulateDelay();
	void GetCurrentCoreFrequency();
	void InitTime();

	// message fifo
	message_buffer& msgBuffer;
};

