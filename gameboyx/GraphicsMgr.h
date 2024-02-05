#pragma once

#include <string>
#include <SDL.h>
#include <imgui_impl_sdl2.h>

#include "data_containers.h"
#include "general_config.h"

class GraphicsMgr {

public:
	// get/reset vkInstance
	static GraphicsMgr* getInstance(SDL_Window* _window, graphics_information& _graphics_info, game_status& _game_stat);
	static void resetInstance();

	// render
	virtual void RenderFrame() = 0;

	// initialize
	virtual bool InitGraphics() = 0;
	virtual bool StartGraphics() = 0;

	// deinit
	virtual bool ExitGraphics() = 0;
	virtual void StopGraphics() = 0;

	// shader compilation
	virtual void EnumerateShaders() = 0;
	virtual void CompileNextShader() = 0;

	// imgui
	virtual bool InitImgui() = 0;
	virtual void DestroyImgui() = 0;
	virtual void NextFrameImGui() const = 0;

	virtual void UpdateGpuData() = 0;

	virtual bool Init2dGraphicsBackend() = 0;
	virtual void Destroy2dGraphicsBackend() = 0;

protected:

	explicit GraphicsMgr(SDL_Window* _window, graphics_information& _graphics_info, game_status& _game_stat)
		: window(_window), graphicsInfo(_graphics_info), gameStat(_game_stat) {
	}
	~GraphicsMgr() = default;

	// sdl
	SDL_Window* window = nullptr;
	virtual void RecalcTex2dScaleMatrixInput() = 0;
	float aspectRatio = 1.f;

	bool resizableBar = false;

	// gpu info
	std::string vendor = "";
	std::string driverVersion = "";

	graphics_information& graphicsInfo;
	game_status& gameStat;

	virtual void DetectResizableBar() = 0;

private:

	static GraphicsMgr* instance;

};

