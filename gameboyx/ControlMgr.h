#pragma once

#include "HardwareStructs.h"
#include <queue>
#include "SDL.h"

class ControlMgr {
public:
	// get/reset instance
	static ControlMgr* getInstance();
	static void resetInstance();

	void InitControl(control_settings& _control_settings);

	// clone/assign protection
	ControlMgr(ControlMgr const&) = delete;
	ControlMgr(ControlMgr&&) = delete;
	ControlMgr& operator=(ControlMgr const&) = delete;
	ControlMgr& operator=(ControlMgr&&) = delete;

	void ProcessInput(bool& _running, SDL_Window* _window);
	std::queue<std::pair<SDL_Keycode, SDL_EventType>>& GetKeys();
	Sint32 GetScroll();
	bool CheckMouseMove(int& _x, int& _y);
	void DisableMouse();

protected:
	// constructor
	explicit ControlMgr() {}
	~ControlMgr() = default;

private:
	static ControlMgr* instance;

	// control
	std::queue<std::pair<SDL_Keycode, SDL_EventType>> keyMap = {};
	Sint32 mouseScroll = 0;

	std::string controllerDatabase;

	int mouseMoveX = 0;
	int mouseMoveY = 0;
};

