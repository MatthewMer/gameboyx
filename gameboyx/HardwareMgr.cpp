#include "HardwareMgr.h"

HardwareMgr* HardwareMgr::instance = nullptr;
u32 HardwareMgr::errors = 0x00000000;
GraphicsMgr* HardwareMgr::graphicsMgr = nullptr;
AudioMgr* HardwareMgr::audioMgr = nullptr;
ControlMgr* HardwareMgr::controlMgr = nullptr;
SDL_Window* HardwareMgr::window = nullptr;

graphics_settings HardwareMgr::graphicsSettings = {};
audio_settings HardwareMgr::audioSettings = {};
control_settings HardwareMgr::controlSettings = {};

u32 HardwareMgr::timePerFrame = 0;
u32 HardwareMgr::currentTimePerFrame = 0;
steady_clock::time_point HardwareMgr::timePointCur = high_resolution_clock::now();
steady_clock::time_point HardwareMgr::timePointPrev = high_resolution_clock::now();
bool HardwareMgr::nextFrame = false;

u32 HardwareMgr::currentMouseMove = 0;

#define HWMGR_SECOND	999999999

u8 HardwareMgr::InitHardware(graphics_settings& _graphics_settings, audio_settings& _audio_settings, control_settings& _control_settings) {
	errors = 0;

	if (instance == nullptr) {
		instance = new HardwareMgr();
	} else {
		errors |= HWMGR_ERR_ALREADY_RUNNING;
		return errors;
	}

	audioSettings = std::move(_audio_settings);
	graphicsSettings = std::move(_graphics_settings);
	controlSettings = std::move(_control_settings);

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
		std::string file_s = ICON_FOLDER + ICON_FILE;
		const char* file_c = file_s.c_str();
		SDL_Surface* icon = SDL_LoadBMP(file_c);

		SDL_SetWindowIcon(window, icon);

		if (!graphicsMgr->InitGraphics()) { return false; }
		if (!graphicsMgr->StartGraphics(graphicsSettings.presentModeFifo, graphicsSettings.tripleBuffering)) { return false; }

		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		if (!graphicsMgr->InitImgui()) { return false; }

		graphicsMgr->EnumerateShaders();
	} else {
		return false;
	}

	SDL_SetWindowMinimumSize(window, GUI_WIN_WIDTH_MIN, GUI_WIN_HEIGHT_MIN);

	// audio
	audioMgr = AudioMgr::getInstance();
	if (audioMgr != nullptr) {
		audioMgr->InitAudio(audioSettings, false);
	} else {
		return false;
	}

	controlMgr = ControlMgr::getInstance();
	if(controlMgr != nullptr){
		controlMgr->InitControl(controlSettings);
	} else {
		return false;
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

void HardwareMgr::ProcessEvents(bool& _running) {
	controlMgr->ProcessEvents(_running, window);
}

std::queue<std::pair<SDL_Keycode, SDL_EventType>>& HardwareMgr::GetKeys() {
	return controlMgr->GetKeys();
}

Sint32 HardwareMgr::GetScroll() {
	return controlMgr->GetScroll();
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
	graphicsSettings.fpsUnlimited = _unlimited;
	graphicsSettings.framerateTarget = _target;

	if (graphicsSettings.framerateTarget > 0) {
		timePerFrame = (u32)((1.f / graphicsSettings.framerateTarget) * pow(10, 9));
	}
	else {
		timePerFrame = 0;
	}
}

void HardwareMgr::ProcessTimedEvents() {
	timePointCur = high_resolution_clock::now();
	u32 time_diff = (u32)duration_cast<nanoseconds>(timePointCur - timePointPrev).count();
	timePointPrev = timePointCur;

	// framerate
	currentTimePerFrame += time_diff;
	if (currentTimePerFrame > timePerFrame) {
		currentTimePerFrame = 0;
		nextFrame = true;
	}

	// process mouse
	if (!controlSettings.mouse_always_visible) {
		static int x, y;
		static bool visible = true;

		if (controlMgr->CheckMouseMove(x, y)) {
			if (!visible) {
				controlMgr->SetMouseVisible(true);
				visible = true;
			}
		} else {
			if (currentMouseMove < HWMGR_SECOND) {
				currentMouseMove += time_diff;
			} else {
				if (visible) {
					controlMgr->SetMouseVisible(false);
					currentMouseMove = 0;
					visible = false;
				}
			}
		}
	}
}

bool HardwareMgr::CheckFrame() {
	if (graphicsSettings.fpsUnlimited) {
		return true;
	} else {
		if (nextFrame) {
			nextFrame = false;
			return true;
		} else {
			return false;
		}
	}
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

void HardwareMgr::SetMouseAlwaysVisible(const bool& _visible) {
	controlSettings.mouse_always_visible = _visible;
	if (_visible) {
		controlMgr->SetMouseVisible(true);
	}
}

ImFont* HardwareMgr::GetFont(const int& _index) {
	return graphicsMgr->GetFont(_index);
}