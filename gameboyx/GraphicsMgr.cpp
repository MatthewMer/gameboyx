#include "GraphicsMgr.h"

#include "GraphicsVulkan.h"

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