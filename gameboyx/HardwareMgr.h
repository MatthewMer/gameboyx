#pragma once

#include <chrono>
using namespace std::chrono;

#include "defs.h"
#include "GraphicsMgr.h"
#include "AudioMgr.h"
#include <imgui.h>
#include <SDL.h>
#include <mutex>
#include "logger.h"
#include "HardwareStructs.h"
#include "ControlMgr.h"
#include "NetworkMgr.h"

#define HWMGR_ERR_ALREADY_RUNNING		0x00000001

class HardwareMgr {
public:
	static u8 InitHardware(graphics_settings& _graphics_settings, audio_settings& _audio_settings, control_settings& _control_settings);
	static void ShutdownHardware();
	static void NextFrame();
	static void RenderFrame();
	static void ProcessEvents(bool& _running);
	static void ToggleFullscreen();

	static void InitGraphicsBackend(virtual_graphics_information& _virt_graphics_info);
	static void InitAudioBackend(virtual_audio_information& _virt_audio_info);
	static void DestroyGraphicsBackend();
	static void DestroyAudioBackend();

	static void UpdateGpuData();

	static Sint32 GetScroll();

	static void GetGraphicsSettings(graphics_settings& _graphics_settings);

	static void SetFramerateTarget(const int& _target, const bool& _unlimited);

	static void SetSwapchainSettings(bool& _present_mode_fifo, bool& _triple_buffering);

	static void SetSamplingRate(int& _sampling_rate);

	static void SetMasterVolume(const float& _volume);

	static void GetAudioSettings(audio_settings& _audio_settings);

	static void ProcessTimedEvents();

	static std::queue<std::pair<SDL_Keycode, bool>>& GetKeyQueue();
	static std::queue<std::tuple<int, SDL_GameControllerButton, bool>>& GetButtonQueue();

	static void SetMouseAlwaysVisible(const bool& _visible);

	static ImFont* GetFont(const int& _index);

	static void OpenNetwork(network_settings& _network_settings);
	static bool CheckNetwork();
	static void CloseNetwork();

	static bool CheckFrame();

private:
	HardwareMgr() = default;
	~HardwareMgr() {
		ShutdownHardware();

		GraphicsMgr::resetInstance();
		AudioMgr::resetInstance();
		ControlMgr::resetInstance();
	}

	static GraphicsMgr* graphicsMgr;
	static AudioMgr* audioMgr;
	static ControlMgr* controlMgr;
	static NetworkMgr* networkMgr;
	static SDL_Window* window;

	static graphics_settings graphicsSettings;
	static audio_settings audioSettings;
	static control_settings controlSettings;
	static network_settings networkSettings;

	static HardwareMgr* instance;
	static u32 errors;

	// for framerate target
	static std::mutex mutTimeDelta;
	static std::condition_variable  notifyTimeDelta;
	static std::chrono::microseconds timePerFrame;
	static bool fpsLimit;
	static steady_clock::time_point timePointCur;
	static steady_clock::time_point timePointPrev;

	static bool nextFrame;

	// control
	static u32 currentMouseMove;
};