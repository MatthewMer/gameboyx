#include "BaseGPU.h"

#include "GameboyGPU.h"
#include "logger.h"

BaseGPU* BaseGPU::instance = nullptr;

BaseGPU* BaseGPU::getInstance(BaseCartridge* _cartridge) {
	if (instance == nullptr) {
		switch (_cartridge->console) {
		case GB:
		case GBC:
			instance = new GameboyGPU(_cartridge);
			break;
		}
	}

	return instance;
}

void BaseGPU::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}