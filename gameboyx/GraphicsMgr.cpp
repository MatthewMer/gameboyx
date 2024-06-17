#include "GraphicsMgr.h"

#include "GraphicsVulkan.h"

namespace Backend {
	namespace Graphics {
		GraphicsMgr* GraphicsMgr::instance = nullptr;

		GraphicsMgr* GraphicsMgr::getInstance(SDL_Window** _window) {
			if (instance != nullptr) {
				instance->resetInstance();
			}

			instance = new GraphicsVulkan(_window);
			return instance;
		}

		void GraphicsMgr::resetInstance() {
			if (instance != nullptr) {
				instance->StopGraphics();
				instance->ExitGraphics();
				delete instance;
				instance = nullptr;
			}
		}

		bool GraphicsMgr::InitGraphicsBackend(virtual_graphics_information& _graphics_info) {
			virtGraphicsInfo = _graphics_info;

			if (virtGraphicsInfo.is2d) {
				return Init2dGraphicsBackend();
			} else {
				return false;
			}
		}

		void GraphicsMgr::DestroyGraphicsBackend() {
			if (virtGraphicsInfo.is2d) {
				Destroy2dGraphicsBackend();
			}
		}

		ImFont* GraphicsMgr::GetFont(const int& _index) {
			if (fonts.size() > _index) { return fonts[_index]; } else { return nullptr; }
		}
	}
}