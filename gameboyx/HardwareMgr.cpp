#include "HardwareMgr.h"

HardwareMgr* HardwareMgr::instance = nullptr;
u32 HardwareMgr::errors = 0x00000000;

u8 HardwareMgr::InitHardware() {
	errors = 0;

	if (instance == nullptr) {
		instance = new HardwareMgr();
	}else{
		errors |= HWMGR_ERR_ALREADY_RUNNING;
		return errors;
	}


	// sdl
	instance->window = nullptr;
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) != 0) {
		LOG_ERROR("[SDL]", SDL_GetError());
		return false;
	} else {
		LOG_INFO("[SDL] initialized");
	}

	// graphics
	instance->graphicsMgr = GraphicsMgr::getInstance(&instance->window);
	if (instance->graphicsMgr != nullptr) {
		if (!instance->graphicsMgr->InitGraphics()) { return false; }
		if (!instance->graphicsMgr->StartGraphics()) { return false; }

		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		if (!instance->graphicsMgr->InitImgui()) { return false; }

		instance->graphicsMgr->EnumerateShaders();
	}

	SDL_SetWindowMinimumSize(instance->window, GUI_WIN_WIDTH_MIN, GUI_WIN_HEIGHT_MIN);

	// audio
	instance->audioMgr = AudioMgr::getInstance();
	if (instance->audioMgr != nullptr) {
		instance->audioMgr->InitAudio(false);
	}

	return true;
}

void HardwareMgr::ShutdownHardware() {
	// graphics
	instance->graphicsMgr->DestroyImgui();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	instance->graphicsMgr->StopGraphics();
	instance->graphicsMgr->ExitGraphics();

	SDL_DestroyWindow(instance->window);
	SDL_Quit();
}

void HardwareMgr::NextFrame() {
	u32 win_min = SDL_GetWindowFlags(instance->window) & SDL_WINDOW_MINIMIZED;

	if (!win_min) {
		instance->graphicsMgr->NextFrameImGui();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
	}
}

void HardwareMgr::RenderFrame() {
	instance->graphicsMgr->RenderFrame();
}

void HardwareMgr::ProcessInput(bool& _running) {
	SDL_Event event;
	auto& key_map = instance->keyMap;
	auto& mouse_scroll = instance->mouseScroll;

	while (SDL_PollEvent(&event)) {
		ImGui_ImplSDL2_ProcessEvent(&event);
		if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(instance->window))
			_running = false;

		key_map.clear();

		switch (event.type) {
		case SDL_QUIT:
			_running = false;
			break;
		case SDL_KEYDOWN:
			key_map.emplace(SDL_KEYDOWN, event.key.keysym.sym);
			break;
		case SDL_KEYUP:
			key_map.emplace(SDL_KEYUP, event.key.keysym.sym);
			break;
		case SDL_MOUSEWHEEL:
			mouse_scroll = event.wheel.y;
			break;
		default:
			mouse_scroll = 0;
			break;
		}
	}
}

void HardwareMgr::ToggleFullscreen() {
	if (SDL_GetWindowFlags(instance->window) & SDL_WINDOW_FULLSCREEN) {
		SDL_SetWindowFullscreen(instance->window, 0);
	} else {
		SDL_SetWindowFullscreen(instance->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
}

void HardwareMgr::InitGraphicsBackend(virtual_graphics_information& _virt_graphics_info) {
	instance->graphicsMgr->InitGraphicsBackend(_virt_graphics_info);
}

void HardwareMgr::InitAudioBackend(virtual_audio_information& _virt_audio_info) {
	instance->audioMgr->InitAudioBackend(_virt_audio_info);
}

void HardwareMgr::DestroyGraphicsBackend() {
	instance->graphicsMgr->DestroyGraphicsBackend();
}

void HardwareMgr::DestroyAudioBackend() {
	instance->audioMgr->DestroyAudioBackend();
}

void HardwareMgr::UpdateGpuData() {
	instance->graphicsMgr->UpdateGpuData();
}