#include "ControlMgr.h"

#include "imgui_impl_sdl2.h"
#include "general_config.h"
#include "logger.h"

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
	controllerDatabase = CONTROL_FOLDER + CONTROL_DB;

	SDL_GameControllerAddMappingsFromFile(controllerDatabase.c_str());
}

void ControlMgr::ProcessInput(bool& _running, SDL_Window* _window) {
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