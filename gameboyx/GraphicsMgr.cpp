#include "GraphicsMgr.h"

#include "GraphicsVulkan.h"

GraphicsMgr* GraphicsMgr::instance = nullptr;

GraphicsMgr* GraphicsMgr::getInstance(SDL_Window* _window, graphics_information& _graphics_info, game_status& _game_stat) {
	if (instance != nullptr) {
		instance->resetInstance();
	}
	instance = new GraphicsVulkan(_window, _graphics_info, _game_stat);
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