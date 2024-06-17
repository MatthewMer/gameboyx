#include "BaseGPU.h"

#include "GameboyGPU.h"
#include "logger.h"

namespace Emulation {
	BaseGPU* BaseGPU::instance = nullptr;

	BaseGPU* BaseGPU::getInstance(BaseCartridge* _cartridge) {
		if (instance == nullptr) {
			switch (_cartridge->console) {
			case GB:
			case GBC:
				instance = new Gameboy::GameboyGPU(_cartridge);
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

	int BaseGPU::GetFrameCount() const {
		return frameCounter;
	}

	void BaseGPU::ResetFrameCount() {
		frameCounter = 0;
	}
}