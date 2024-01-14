#include "BaseGPU.h"

#include "GameboyGPU.h"
#include "logger.h"

BaseGPU* BaseGPU::instance = nullptr;

BaseGPU* BaseGPU::getInstance(graphics_information& _graphics_info, GraphicsMgr* _graphics_mgr) {
	resetInstance();
	instance = new GameboyGPU(_graphics_info, _graphics_mgr);

	return instance;
}

BaseGPU* BaseGPU::getInstance() {
	return instance;
}

void BaseGPU::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}