#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	This class manages all the emulated hardware and synchronizes it as well as initialization. It takes the game_info struct instance
*	passed by the GUI object and passes it to the constructors of the hardware classes. 
*/

#include <SDL.h>

#include "defs.h"

class BaseCartridge;

#define VHWMGR_ERR_READ_ROM			0x01
#define VHWMGR_ERR_INIT_THREAD		0x02
#define VHWMGR_ERR_INIT_HW			0x04
#define VHWMGR_ERR_CART_NULL		0x08
#define VHWMGR_ERR_THREAD_RUNNING	0x10

class GuiMgr;
typedef void (GuiMgr::* gui_callback)();

namespace VHardwareMgr
{
	u8 InitHardware(BaseCartridge* _cartridge);
	void ShutdownHardware();

	// members for running hardware
	void ProcessHardware();

	// SDL
	bool EventKeyDown(const SDL_Keycode& _key);
	bool EventKeyUp(const SDL_Keycode& _key);

	u8 SetInitialValues(const bool& _debug_enable, const bool& _pause_execution, const int& _emulation_speed);
	void SetDebugEnabled(const bool& _debug_enabled);
	void SetPauseExecution(const bool& _pause_execution);
	void SetEmulationSpeed(const int& _emulation_speed);
	u8 ResetGame();
	
	void GetFpsAndClock(float& _clock, float& _fps);
};

