#include "BaseMMU.h"

#include "GameboyMMU.h"
#include "logger.h"

#include <vector>

using namespace std;

/* ***********************************************************************************************************
	MMU BASE CLASS
*********************************************************************************************************** */
BaseMMU* BaseMMU::instance = nullptr;

BaseMMU* BaseMMU::getInstance(BaseCartridge* _cartridge) {
	if (instance == nullptr) {
		switch (_cartridge->console) {
		case GB:
		case GBC:
			instance = GameboyMMU::getInstance(_cartridge);
			break;
		}
	}

	return instance;
}

void BaseMMU::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}