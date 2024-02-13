#pragma once

#include <string>
#include <SDL.h>
#include <imgui_impl_sdl2.h>

#include "general_config.h"
#include "HardwareStructs.h"

class GraphicsMgr {

public:
	// get/reset vkInstance
	static GraphicsMgr* getInstance(SDL_Window** _window);
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

	bool InitGraphicsBackend(virtual_graphics_information& _graphics_info);
	void DestroyGraphicsBackend();

protected:

	explicit GraphicsMgr() = default;
	~GraphicsMgr() = default;

	// sdl
	SDL_Window* window = nullptr;
	virtual void RecalcTex2dScaleMatrix() = 0;
	float aspectRatio = 1.f;

	virtual void UpdateTex2d() = 0;

	bool resizableBar = false;

	// gpu info
	std::string vendor = "";
	std::string driverVersion = "";

	virtual void DetectResizableBar() = 0;

	virtual bool Init2dGraphicsBackend() = 0;
	virtual void Destroy2dGraphicsBackend() = 0;

	virtual_graphics_information graphicsInfo;

	u32 win_width;
	u32 win_height;

	int shadersCompiled;
	int shadersTotal;
	bool shaderCompilationFinished;

private:
	static GraphicsMgr* instance;
};

