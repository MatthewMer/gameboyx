#pragma once

#include "defs.h"
#include "GraphicsMgr.h"
#include "AudioMgr.h"
#include "imgui.h"
#include "SDL.h"
#include "logger.h"
#include "HardwareStructs.h"

#include <unordered_map>

#define HWMGR_ERR_ALREADY_RUNNING		0x00000001

class HardwareMgr {
public:
	static u8 InitHardware();
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

private:
	HardwareMgr() = default;
	~HardwareMgr() = default;

	GraphicsMgr* graphicsMgr = nullptr;
	AudioMgr* audioMgr = nullptr;
	SDL_Window* window = nullptr;

	graphics_information graphicsInfo;
	audio_information audioInfo;

	// control
	std::unordered_map<SDL_EventType, SDL_Keycode> keyMap = std::unordered_map<SDL_EventType, SDL_Keycode>();
	Sint32 mouseScroll = 0;

	static HardwareMgr* instance;
	static u32 errors;
};