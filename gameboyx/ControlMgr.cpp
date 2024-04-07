#include "ControlMgr.h"

#include "imgui_impl_sdl2.h"
#include "general_config.h"
#include "logger.h"
#include <format>

ControlMgr* ControlMgr::instance = nullptr;

ControlMgr* ControlMgr::getInstance() {
	if (instance == nullptr) {
		instance = new ControlMgr();
	}

	return instance;
}

void ControlMgr::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}

void ControlMgr::InitControl(control_settings& _control_settings) {
	// init controller DB
	controllerDatabase = CONTROL_FOLDER + CONTROL_DB;
	SDL_GameControllerAddMappingsFromFile(controllerDatabase.c_str());

	/*
	// setup gamepad
	int count = SDL_NumJoysticks();
	LOG_INFO("[SDL] gamepads detected: ", count);
	for (int i = 0; i < count; i++) {
		LOG_INFO("[SDL] ", i, ": ", SDL_JoystickNameForIndex(i));
	}
	if (count > 0) {
		gamepad = SDL_Game(0);
		guid = SDL_JoystickGetGUID(gamepad);

		char c_guid[33];
		SDL_GUIDToString(guid, c_guid, 33);
		sGuid = std::string(c_guid);

		LOG_INFO("[SDL] selected 0: ", SDL_JoystickNameForIndex(0), " - GUID: ", sGuid);
	}
	*/
}

void ControlMgr::ProcessEvents(bool& _running, SDL_Window* _window) {
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		ImGui_ImplSDL2_ProcessEvent(&event);
		if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(_window))
			_running = false;

		switch (event.type) {
		case SDL_QUIT:
			_running = false;
			break;
		case SDL_KEYDOWN:
			keyMap.push(std::pair(event.key.keysym.sym, SDL_KEYDOWN));
			break;
		case SDL_KEYUP:
			keyMap.push(std::pair(event.key.keysym.sym, SDL_KEYUP));
			break;
		case SDL_MOUSEWHEEL:
			mouseScroll = event.wheel.y;
			break;
		case SDL_CONTROLLERDEVICEADDED:
			OnGamepadConnect(event.cdevice);
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
			OnGamepadDisconnect(event.cdevice);
			break;
		default:
			break;
		}
	}
}

Sint32 ControlMgr::GetScroll() {
	Sint32 scroll = mouseScroll;
	mouseScroll = 0;
	return scroll;
}

std::queue<std::pair<SDL_Keycode, SDL_EventType>>& ControlMgr::GetKeys() {
	return keyMap;
}

bool ControlMgr::CheckMouseMove(int& _x, int& _y) {
	SDL_GetMouseState(&_x, &_y);

	if (mouseMoveX != _x || mouseMoveY != _y) {
		mouseMoveX = _x;
		mouseMoveY = _y;
		return true;
	} else {
		return false;
	}
}

void ControlMgr::DisableMouse() {
	ImGui::SetMouseCursor(ImGuiMouseCursor_None);

	if (ImGuiMouseCursor have = ImGui::GetMouseCursor(); have != ImGuiMouseCursor_None) {
		std::string result;
		switch (have) {
		case ImGuiMouseCursor_Arrow:
			result = "enabled";
			break;
		case ImGuiMouseCursor_None:
			result = "disabled";
			break;
		}

		LOG_ERROR("[SDL] set cursor state: ", result);
	}
}

void ControlMgr::OnGamepadConnect(SDL_ControllerDeviceEvent& e) {
	int device_index = e.which;

	if (!connected && SDL_IsGameController(device_index)) {
		gamepad = SDL_GameControllerOpen(device_index);

		SDL_Joystick* joystick = SDL_GameControllerGetJoystick(gamepad);
		guid = SDL_JoystickGetGUID(joystick);

		char c_guid[33];
		SDL_GUIDToString(guid, c_guid, 33);
		LOG_INFO("[SDL] gamepad connected: ", SDL_JoystickNameForIndex(device_index), " - GUID: {", c_guid, "}");

		connected = true;
	} else {
		LOG_ERROR("[SDL] gamepad not recognized");
	}
}

void ControlMgr::OnGamepadDisconnect(SDL_ControllerDeviceEvent& e) {
	int device_index = e.which;

	if (connected) {
		SDL_GameControllerClose(gamepad);
	} else {
		LOG_ERROR("[SDL] gamepad not recognized");
	}
}