#pragma once

#include <string>
#include <SDL.h>
#include <imgui_impl_sdl2.h>
#include <atomic>

#include "general_config.h"
#include "HardwareStructs.h"

namespace Backend {
	namespace Graphics {
		class GraphicsMgr {

		public:
			// get/reset vkInstance
			static GraphicsMgr* getInstance(SDL_Window** _window);
			static void resetInstance();

			// render
			virtual void RenderFrame() = 0;

			// initialize
			virtual bool InitGraphics() = 0;
			virtual bool StartGraphics(bool& _present_mode_fifo, bool& _triple_buffering) = 0;

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

			virtual void SetSwapchainSettings(bool& _present_mode_fifo, bool& _triple_buffering) = 0;

			ImFont* GetFont(const int& _index);

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

			u32 win_width = 0;
			u32 win_height = 0;

			virtual_graphics_information virtGraphicsInfo = {};

			int shadersCompiled;
			int shadersTotal;
			bool shaderCompilationFinished;

			std::vector<ImFont*> fonts;

		private:
			static GraphicsMgr* instance;
		};
	}
}