#pragma once

#include <queue>
#include <chrono>
using namespace std::chrono;

#include "defs.h"
#include "GraphicsMgr.h"
#include "AudioMgr.h"
#include <imgui.h>
#include <SDL.h>
#include "logger.h"
#include "HardwareStructs.h"

#define HWMGR_ERR_ALREADY_RUNNING		0x00000001

class HardwareMgr {
public:
	static u8 InitHardware(graphics_settings& _graphics_settings);
	static void ShutdownHardware();
	static void NextFrame();
	static void RenderFrame();
	static void ProcessInput(bool& _running);
	static void ToggleFullscreen();

	static void InitGraphicsBackend(virtual_graphics_information& _virt_graphics_info);
	static void InitAudioBackend(virtual_audio_information& _virt_audio_info);
	static void DestroyGraphicsBackend();
	static void DestroyAudioBackend();

	static void UpdateGpuData();

	static std::queue<std::pair<SDL_Keycode, SDL_EventType>>& GetKeys();
	static Sint32 GetScroll();

	static void GetGraphicsSettings(graphics_settings& _graphics_settings);

	static void SetFramerateTarget(const int& _target, const bool& _unlimited);

	static void SetSwapchainSettings(bool& _present_mode_fifo, bool& _triple_buffering);

private:
	HardwareMgr() = default;
	~HardwareMgr() = default;

	static GraphicsMgr* graphicsMgr;
	static AudioMgr* audioMgr;
	static SDL_Window* window;

	static graphics_information graphicsInfo;
	static audio_information audioInfo;

	static graphics_settings graphicsSettings;

	// control
	static std::queue<std::pair<SDL_Keycode, SDL_EventType>> keyMap;
	static Sint32 mouseScroll;

	static HardwareMgr* instance;
	static u32 errors;

	// for framerate target
	static void CheckDelay();
	static u32 timePerFrame;
	static u32 currentTimePerFrame;
	static steady_clock::time_point timeFramePrev;
	static steady_clock::time_point timeFrameCur;
};