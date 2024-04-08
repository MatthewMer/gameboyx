#pragma once

#include "HardwareStructs.h"

#include <queue>
#include <tuple>
#include <map>
#include <array>

#include "SDL.h"
#include "imgui_impl_sdl2.h"

struct controller_data{
	bool has_controller = false;

	SDL_JoystickID instance_id = 0;
	SDL_GameController* gamepad = nullptr;
	char* mapping = nullptr;
	SDL_JoystickGUID guid = {};
};

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

	void ProcessEvents(bool& _running, SDL_Window* _window);
	std::queue<std::pair<SDL_Keycode, SDL_EventType>>& GetKeys();
	Sint32 GetScroll();
	bool CheckMouseMove(int& _x, int& _y);
	void SetMouseVisible(const bool& _visible);

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

	ImGuiIO& io = ImGui::GetIO();

	int mouseMoveX = 0;
	int mouseMoveY = 0;

	void OnGamepadConnect(SDL_ControllerDeviceEvent& e);
	void OnGamepadDisconnect(SDL_ControllerDeviceEvent& e);

	void AddController(const int& _device_index);
	void RemoveController(const Sint32& _instance_id);

	std::array<controller_data, 1> connectedGamepads;

	std::map<SDL_JoystickID, std::tuple<int, std::string, SDL_JoystickGUID, bool>> availableGamepads;
};