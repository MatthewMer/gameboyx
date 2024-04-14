#include "ControlMgr.h"

#include "general_config.h"
#include "logger.h"
#include "helper_functions.h"
#include <format>

static std::string guid_to_string(const SDL_JoystickGUID& _guid) {
	char c_guid[33];
	SDL_GUIDToString(_guid, c_guid, 33);
	return std::string(c_guid);
}

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
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;

	// init controller DB
	controllerDatabase = CONTROL_FOLDER + CONTROL_DB;
	SDL_GameControllerAddMappingsFromFile(controllerDatabase.c_str());

	SDL_Surface* surface = SDL_LoadBMP((CURSOR_FOLDER + CURSOR_MAIN).c_str());
	if (surface == nullptr) { LOG_ERROR("[SDL] create cursor surface: ", SDL_GetError()); }
	else {
		if (cursor) { SDL_FreeCursor(cursor); }
		cursor = SDL_CreateColorCursor(surface, 0, 0);
		if (cursor == nullptr) { LOG_ERROR("[SDL] create cursor: ", SDL_GetError()); }
		else {
			SDL_SetCursor(cursor);
		}
	}
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
		case SDL_CONTROLLERBUTTONUP:
			
			break;
		case SDL_CONTROLLERBUTTONDOWN:

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

bool ControlMgr::GetMouseVisible() {
	int res = SDL_ShowCursor(SDL_QUERY);
	switch (res) {
	case SDL_ENABLE:
		return true;
		break;
	case SDL_DISABLE:
		return false;
		break;
	default:
		LOG_ERROR("[SDL] query mouse cursor state");
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
		SDL_JoystickID instance_id;
		std::string name;
		SDL_JoystickGUID guid;
		std::string s_guid;

		gamepad = SDL_GameControllerOpen(_device_index);
		instance_id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gamepad));

		guid = SDL_JoystickGetGUID(SDL_GameControllerGetJoystick(gamepad));
		s_guid = guid_to_string(guid);

		bool valid = false;
		const char* c_name = SDL_GameControllerName(gamepad);
		if (c_name != nullptr) {
			name = std::string(c_name);
			valid = true;
		} else {
			name = N_A;
		}

		SDL_GameControllerClose(gamepad);

		if (valid) {
			availableGamepads[instance_id] = { _device_index, name , guid, false };

			bool used = false;
			int i = 0;
			for (auto& n : connectedGamepads) {
				if (!n.has_controller) {
					SetPlayerController(i, instance_id);
					used = true;
					break;
				}
				i++;
			}	

			if (!used) {
				LOG_INFO("[SDL] gamepad added (instance ", instance_id, "; index ", _device_index, "): ", name, " - GUID: {", s_guid, "}");
			}
		}
	}
}

void ControlMgr::RemoveController(const Sint32& _instance_id) {
	if (auto it = availableGamepads.find(_instance_id); it != availableGamepads.end()) {

		int i = 0;
		bool used = false;
		if (get<GP_USED>(it->second)) {
			
			for (auto& n : connectedGamepads) {
				if (n.has_controller && n.instance_id == _instance_id) {
					UnsetPlayerController(i);
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

		std::string s_guid = guid_to_string(guid);
			
		availableGamepads.erase(it);

		if (!used) {
			LOG_INFO("[SDL] gamepad removed (instance ", instance_id, "; index ", device_index, "): ", name, " - GUID: {", s_guid, "}");
		}
	}
}

void ControlMgr::SetPlayerController(const int& _player, const int& _instance_id) {
	auto& player = connectedGamepads[_player];
	auto& controller = availableGamepads[_instance_id];

	if (player.has_controller) {
		UnsetPlayerController(_player);
	}

	player.instance_id = _instance_id;
	player.gamepad = SDL_GameControllerOpen(get<GP_DEVICE_INDEX>(controller));
	player.has_controller = true;

	get<GP_USED>(controller) = true;

	std::string s_guid = guid_to_string(get<GP_GUID>(controller));

	LOG_INFO("[SDL] gamepad connected for player ", _player + 1, " (instance ", player.instance_id, "; index ", get<GP_DEVICE_INDEX>(controller), "): ", get<GP_DEVICE_NAME>(controller), " - GUID: {", s_guid, "}");
}

void ControlMgr::UnsetPlayerController(const int& _player) {
	auto& player = connectedGamepads[_player];
	auto& controller = availableGamepads[player.instance_id];
	
	SDL_GameControllerClose(player.gamepad);
	player.has_controller = false;

	get<GP_USED>(controller) = false;

	std::string s_guid = guid_to_string(get<GP_GUID>(controller));

	LOG_INFO("[SDL] gamepad disconnected for player ", _player + 1, " (instance ", player.instance_id, "; index ", get<GP_DEVICE_INDEX>(controller), "): ", get<GP_DEVICE_NAME>(controller), " - GUID: {", s_guid, "}");
}



