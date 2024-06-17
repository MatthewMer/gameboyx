#pragma once

#include "HardwareStructs.h"

#include <queue>
#include <tuple>
#include <map>
#include <array>

#include "SDL.h"
#include "imgui_impl_sdl2.h"

namespace Backend {
	namespace Control {
		enum gamepad_data {
			GP_DEVICE_INDEX,
			GP_DEVICE_NAME,
			GP_GUID,
			GP_USED
		};

		struct controller_data {
			bool has_controller = false;

			SDL_JoystickID instance_id = 0;
			SDL_GameController* gamepad = nullptr;
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
			std::queue<std::pair<SDL_Keycode, bool>>& GetKeyQueue();
			std::queue<std::tuple<int, SDL_GameControllerButton, bool>>& GetButtonQueue();
			Sint32 GetScroll();
			bool CheckMouseMove(int& _x, int& _y);
			void SetMouseVisible(const bool& _visible);

			bool GetMouseVisible();

			void SetPlayerController(const int& _player, const int& _instance_id);
			void UnsetPlayerController(const int& _player);

		protected:
			// constructor
			explicit ControlMgr() {}
			~ControlMgr() = default;

		private:
			static ControlMgr* instance;

			// control
			std::queue<std::pair<SDL_Keycode, bool>> keyQueue = {};
			std::queue<std::tuple<int, SDL_GameControllerButton, bool>> buttonQueue = {};
			Sint32 mouseScroll = 0;

			std::string controllerDatabase;

			ImGuiIO& io = ImGui::GetIO();

			int mouseMoveX = 0;
			int mouseMoveY = 0;

			void OnGamepadConnect(SDL_ControllerDeviceEvent& e);
			void OnGamepadDisconnect(SDL_ControllerDeviceEvent& e);

			void AddController(const int& _device_index);
			void RemoveController(const Sint32& _instance_id);

			std::vector<controller_data> connectedGamepads = std::vector<controller_data>(1);

			std::map<SDL_JoystickID, std::tuple<int, std::string, SDL_JoystickGUID, bool>> availableGamepads;

			SDL_Cursor* cursor = nullptr;

			void EnqueueKeyboardInput(const SDL_Event& _event);
			void EnqueueControllerInput(const SDL_Event& _event);

		};
	}
}