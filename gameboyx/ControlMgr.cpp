#include "ControlMgr.h"

#include "general_config.h"
#include "logger.h"
#include <format>

enum gamepad_data {
	GP_DEVICE_INDEX,
	GP_DEVICE_NAME,
	GP_GUID,
	GP_USED
};

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
	// mouse cursor
	io.ConfigFlags |= (ImGuiConfigFlags_NoMouseCursorChange | ImGuiConfigFlags_NavEnableGamepad);		// perhaps need to toggle NavEnableGamepad when in game or menu, needs some testing

	// init controller DB
	controllerDatabase = CONTROL_FOLDER + CONTROL_DB;
	SDL_GameControllerAddMappingsFromFile(controllerDatabase.c_str());
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

void ControlMgr::SetMouseVisible(const bool& _visible) {
	SDL_ShowCursor(_visible ? SDL_ENABLE : SDL_DISABLE);
}

void ControlMgr::OnGamepadConnect(SDL_ControllerDeviceEvent& e) {
	int device_index = e.which;
	AddController(device_index);
}

void ControlMgr::OnGamepadDisconnect(SDL_ControllerDeviceEvent& e) {
	Sint32 instance_id = e.which;
	RemoveController(instance_id);
}

void ControlMgr::AddController(const int& _device_index) {
	if (SDL_IsGameController(_device_index)) {
		SDL_GameController* gamepad;
		SDL_JoystickID id;
		std::string name;
		SDL_JoystickGUID guid;
		std::string s_guid;

		gamepad = SDL_GameControllerOpen(_device_index);
		id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gamepad));

		name = std::string(SDL_JoystickNameForIndex(_device_index));

		guid = SDL_JoystickGetGUID(SDL_GameControllerGetJoystick(gamepad));
		char c_guid[33];
		SDL_GUIDToString(guid, c_guid, 33);
		s_guid = std::string(c_guid);

		bool used = false;
		int i = 0;
		for (auto& n : connectedGamepads) {
			if (!n.has_controller) {
				n.instance_id = id;
				n.gamepad = gamepad;
				n.mapping = SDL_GameControllerMappingForGUID(guid);
				n.has_controller = true;

				used = true;
				break;
			}
			i++;
		}

		availableGamepads[id] = { _device_index, name , guid, used };

		if (used) {
			LOG_INFO("[SDL] gamepad connected for player ", i + 1, " (instance ", id, "; index ", _device_index, "): ", name, " - GUID: {", s_guid, "}");
		} else {
			LOG_INFO("[SDL] gamepad added (instance ", id, "; index ", _device_index, "): ", name, " - GUID: {", s_guid, "}");
			SDL_GameControllerClose(gamepad);
		}

	}

		/*
		bool found = false;
		for (auto it = gamepads.begin(); it != gamepads.end(); it++) {
			if (get<GP_DEVICE_INDEX>(it->second) == _device_index) {
				found = true;
			}
		}

		if (!found) {
			SDL_GameController* gamepad;
			SDL_JoystickGUID guid;
			SDL_JoystickID id;
			std::string s_guid;

			gamepad = SDL_GameControllerOpen(_device_index);

			SDL_Joystick* joystick = SDL_GameControllerGetJoystick(gamepad);
			guid = SDL_JoystickGetGUID(joystick);
			id = SDL_JoystickInstanceID(joystick);

			char c_guid[33];
			SDL_GUIDToString(guid, c_guid, 33);
			s_guid = std::string(c_guid);

			std::string name = std::string(SDL_JoystickNameForIndex(_device_index));

			char* mapping = SDL_GameControllerMappingForGUID(guid);
			if (mapping == nullptr) {
				SDL_GameControllerClose(gamepad);
				const char* res = SDL_GetError();
				LOG_ERROR("[SDL] no mapping found for gamepad (", _device_index, "): ", res);
				return;
			}

			gamepads[id] = std::tuple(_device_index, gamepad, guid, mapping, name);

			LOG_INFO("[SDL] gamepad connected (", _device_index, "): ", get<GP_DEVICE_NAME>(gamepads[_device_index]), " - GUID: {", s_guid, "}");
		} else {
			LOG_WARN("[SDL] gamepad already added");
		}
		*/
}

void ControlMgr::RemoveController(const Sint32& _instance_id) {
	if (auto it = availableGamepads.find(_instance_id); it != availableGamepads.end()) {

		int i = 0;
		bool used = false;
		if (get<GP_USED>(it->second)) {
			
			for (auto& n : connectedGamepads) {
				if (n.has_controller && n.instance_id == _instance_id) {
					SDL_GameControllerClose(n.gamepad);
					n.has_controller = false;
					used = true;
					break;
				}
				i++;
			}
		}

		Sint32 instance_id = it->first;
		int device_index = get<GP_DEVICE_INDEX>(it->second);
		std::string name = get<GP_DEVICE_NAME>(it->second);
		SDL_JoystickGUID guid = get<GP_GUID>(it->second);

		char c_guid[33];
		SDL_GUIDToString(guid, c_guid, 33);
		std::string s_guid = std::string(c_guid);
			
		availableGamepads.erase(_instance_id);

		if (used) {
			LOG_INFO("[SDL] gamepad disconnected for player ", i + 1, " (instance ", instance_id, "; index ", device_index, "): ", name, " - GUID: {", s_guid, "}");
		} else {
			LOG_INFO("[SDL] gamepad removed (instance ", instance_id, "; index ", device_index, "): ", name, " - GUID: {", s_guid, "}");
		}
	}


	/*
	for (auto it = gamepads.begin(); it != gamepads.end(); it++) {
		if (get<GP_DEVICE_INDEX>(it->second) == _device_index) {
			SDL_GameControllerClose(get<GP_GAMEPAD>(it->second));

			std::string name = get<GP_DEVICE_NAME>(it->second);

			char c_guid[33];
			SDL_GUIDToString(get<GP_GUID>(it->second), c_guid, 33);
			std::string s_guid = std::string(c_guid);

			SDL_free(get<GP_MAPPING>(it->second));

			SDL_JoystickID id = it->first;
			gamepads.erase(id);

			LOG_INFO("[SDL] gamepad disconnected (", _device_index, "): ", name, " - GUID: {", s_guid, "}");
			return;
		}
	}
	*/
}