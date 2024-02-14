#include "BaseAPU.h"
#include "GameboyAPU.h"

#include "logger.h"

BaseAPU* BaseAPU::instance = nullptr;

BaseAPU* BaseAPU::getInstance(BaseCartridge* _cartridge) {
	if (instance == nullptr) {
		switch (_cartridge->console) {
		case GB:
		case GBC:
			instance = new GameboyAPU(_cartridge);
			break;
		}
	}

	return instance;
}

BaseAPU* BaseAPU::getInstance() {
	if (instance == nullptr) {
		LOG_ERROR("[emu] APU instance is nullptr");
	}
	return instance;
}

void BaseAPU::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}