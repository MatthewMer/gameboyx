#include "BaseGPU.h"

#include "GameboyGPU.h"
#include "logger.h"

BaseGPU* BaseGPU::instance = nullptr;

BaseGPU* BaseGPU::getInstance(BaseCartridge* _cartridge, virtual_graphics_settings& _virt_graphics_settings) {
	if (instance == nullptr) {
		switch (_cartridge->console) {
		case GB:
		case GBC:
			instance = new GameboyGPU(_cartridge, _virt_graphics_settings);
			break;
		}
	}

	return instance;
}

BaseGPU* BaseGPU::getInstance() {
	if (instance == nullptr) {
		LOG_ERROR("[emu] GPU instance is nullptr");
	}
	return instance;
}

void BaseGPU::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}

int BaseGPU::GetFrameCount() {
	int frames = frameCounter;
	frameCounter = 0;
	return frames;
}