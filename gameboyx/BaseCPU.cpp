#include "BaseCPU.h"
#include "GameboyCPU.h"

#include "logger.h"

#include "general_config.h"

BaseCPU* BaseCPU::instance = nullptr;

BaseCPU* BaseCPU::getInstance(BaseCartridge* _cartridge) {
	if (instance == nullptr) {
		switch (_cartridge->console) {
		case GB:
		case GBC:
			instance = new GameboyCPU(_cartridge);
			break;
		}
	}

	return instance;
}

void BaseCPU::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}