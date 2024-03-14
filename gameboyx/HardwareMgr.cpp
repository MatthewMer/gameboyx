#include "HardwareMgr.h"

HardwareMgr* HardwareMgr::instance = nullptr;
u32 HardwareMgr::errors = 0x00000000;
GraphicsMgr* HardwareMgr::graphicsMgr = nullptr;
AudioMgr* HardwareMgr::audioMgr = nullptr;
SDL_Window* HardwareMgr::window = nullptr;

graphics_settings HardwareMgr::graphicsSettings = {};
audio_settings HardwareMgr::audioSettings = {};

std::queue<std::pair<SDL_Keycode, SDL_EventType>> HardwareMgr::keyMap = std::queue<std::pair<SDL_Keycode, SDL_EventType>>();
Sint32 HardwareMgr::mouseScroll = 0;

u32 HardwareMgr::timePerFrame = 0;
u32 HardwareMgr::currentTimePerFrame = 0;
steady_clock::time_point HardwareMgr::timeFramePrev = high_resolution_clock::now();
steady_clock::time_point HardwareMgr::timeFrameCur = high_resolution_clock::now();

u8 HardwareMgr::InitHardware(graphics_settings& _graphics_settings, audio_settings& _audio_settings) {
	errors = 0;

	if (instance == nullptr) {
		instance = new HardwareMgr();
	} else {
		errors |= HWMGR_ERR_ALREADY_RUNNING;
		return errors;
	}

	audioSettings = std::move(_audio_settings);
	graphicsSettings = std::move(_graphics_settings);

	SetFramerateTarget(graphicsSettings.framerateTarget, graphicsSettings.fpsUnlimited);

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
		if (!graphicsMgr->StartGraphics(graphicsSettings.presentModeFifo, graphicsSettings.tripleBuffering)) { return false; }

		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		if (!graphicsMgr->InitImgui()) { return false; }

		graphicsMgr->EnumerateShaders();
	}

	SDL_SetWindowMinimumSize(window, GUI_WIN_WIDTH_MIN, GUI_WIN_HEIGHT_MIN);

	// audio
	audioMgr = AudioMgr::getInstance();
	if (audioMgr != nullptr) {
		audioMgr->InitAudio(audioSettings, false);
	}

	return true;
}

void HardwareMgr::ShutdownHardware() {
	// graphics
	graphicsMgr->DestroyImgui();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	graphicsMgr->StopGraphics();
	graphicsMgr->ExitGraphics();

	SDL_DestroyWindow(window);
	SDL_Quit();
}

void HardwareMgr::NextFrame() {
	u32 win_min = SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED;

	if (!win_min) {
		graphicsMgr->NextFrameImGui();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
	}
}

void HardwareMgr::RenderFrame() {
	graphicsMgr->RenderFrame();
}

void HardwareMgr::ProcessInput(bool& _running) {
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		ImGui_ImplSDL2_ProcessEvent(&event);
		if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
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

std::queue<std::pair<SDL_Keycode, SDL_EventType>>& HardwareMgr::GetKeys() {
	return keyMap;
}

Sint32 HardwareMgr::GetScroll() {
	Sint32 scroll = mouseScroll;
	mouseScroll = 0;
	return scroll;
}

void HardwareMgr::ToggleFullscreen() {
	if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) {
		SDL_SetWindowFullscreen(window, 0);
	} else {
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
}

void HardwareMgr::InitGraphicsBackend(virtual_graphics_information& _virt_graphics_info) {
	graphicsMgr->InitGraphicsBackend(_virt_graphics_info);
}

void HardwareMgr::InitAudioBackend(virtual_audio_information& _virt_audio_info) {
	audioMgr->InitAudioBackend(_virt_audio_info);
}

void HardwareMgr::DestroyGraphicsBackend() {
	graphicsMgr->DestroyGraphicsBackend();
}

void HardwareMgr::DestroyAudioBackend() {
	audioMgr->DestroyAudioBackend();
}

void HardwareMgr::UpdateGpuData() {
	graphicsMgr->UpdateGpuData();
}

void HardwareMgr::SetFramerateTarget(const int& _target, const bool& _unlimited) {
	if (_target > 0 && !_unlimited) {
		timePerFrame = (u32)((1.f / _target) * pow(10, 6));
	}
	else {
		timePerFrame = 0;
	}
}

bool HardwareMgr::ExecuteDelay() {
	if (!graphicsSettings.fpsUnlimited) {
		if (currentTimePerFrame < timePerFrame) {
			timeFrameCur = high_resolution_clock::now();
			currentTimePerFrame = (u32)duration_cast<microseconds>(timeFrameCur - timeFramePrev).count();
			return false;
		} else {
			timeFramePrev = timeFrameCur;
			currentTimePerFrame = 0;
		}
	}

	return true;
}

void HardwareMgr::GetGraphicsSettings(graphics_settings& _graphics_settings) {
	_graphics_settings = graphicsSettings;
}

void HardwareMgr::SetSwapchainSettings(bool& _present_mode_fifo, bool& _triple_buffering) {
	graphicsMgr->SetSwapchainSettings(_present_mode_fifo, _triple_buffering);
}

void HardwareMgr::SetSamplingRate(int& _sampling_rate) {
	audioSettings.sampling_rate = _sampling_rate;
	audioMgr->SetSamplingRate(audioSettings);

	_sampling_rate = audioSettings.sampling_rate;
}

void HardwareMgr::SetMasterVolume(const float& _volume) {
	audioSettings.master_volume = _volume;
	audioMgr->SetMasterVolume(_volume);
}

void HardwareMgr::GetAudioSettings(audio_settings& _audio_settings) {
	_audio_settings = audioSettings;
}