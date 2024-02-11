#include "HardwareMgr.h"

#include "GraphicsMgr.h"
#include "AudioMgr.h"
#include "imgui.h"
#include "SDL.h"
#include "logger.h"

#include <unordered_map>

namespace HardwareMgr {
	namespace {
		GraphicsMgr* graphicsMgr;

		AudioMgr* audioMgr;

		SDL_Window* window;
		u32 win_min;

		// graphics
		bool is2d;

		// audio

		// control
		std::unordered_map<SDL_EventType, SDL_Keycode> keyMap;
		Sint32 mouseScroll;
	}

	bool InitHardware() {
		// sdl
		window = nullptr;
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) != 0) {
			LOG_ERROR("[SDL]", SDL_GetError());
			return false;
		} else {
			LOG_INFO("[SDL] initialized");
		}

		// graphics
		graphicsMgr = GraphicsMgr::getInstance(&window);
		if (graphicsMgr != nullptr) {
			if (!graphicsMgr->InitGraphics()) { return false; }
			if (!graphicsMgr->StartGraphics()) { return false; }

			ImGui::CreateContext();
			ImGui::StyleColorsDark();
			if (!graphicsMgr->InitImgui()) { return false; }

			graphicsMgr->EnumerateShaders();
		}

		SDL_SetWindowMinimumSize(window, GUI_WIN_WIDTH_MIN, GUI_WIN_HEIGHT_MIN);

		// audio
		audioMgr = AudioMgr::getInstance();
		if (audioMgr != nullptr) {
			audioMgr->InitAudio(false);
		}

		return true;
	}

	void ShutdownHardware() {
		// graphics
		graphicsMgr->DestroyImgui();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();

		graphicsMgr->StopGraphics();
		graphicsMgr->ExitGraphics();

		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	void NextFrame() {
		win_min = SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED;

		if (!win_min) {
			graphicsMgr->NextFrameImGui();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();
		}
	}

	void RenderFrame() {
		graphicsMgr->RenderFrame();
	}

	void ProcessInput(bool& _running) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
				_running = false;

			keyMap.clear();

			switch (event.type) {
			case SDL_QUIT:
				_running = false;
				break;
			case SDL_KEYDOWN:
				keyMap.emplace(SDL_KEYDOWN, event.key.keysym.sym);
				break;
			case SDL_KEYUP:
				keyMap.emplace(SDL_KEYUP, event.key.keysym.sym);
				break;
			case SDL_MOUSEWHEEL:
				mouseScroll = event.wheel.y;
				break;
			default:
				mouseScroll = 0;
				break;
			}
		}
	}

	void ToggleFullscreen() {
		if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) {
			SDL_SetWindowFullscreen(window, 0);
		} else {
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
	}
}