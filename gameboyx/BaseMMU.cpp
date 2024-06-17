#include "BaseMMU.h"

#include "GameboyMMU.h"
#include "logger.h"

#include <vector>

using namespace std;

namespace Emulation {
	/* ***********************************************************************************************************
		MMU BASE CLASS
	*********************************************************************************************************** */
	BaseMMU* BaseMMU::instance = nullptr;

	BaseMMU* BaseMMU::getInstance(BaseCartridge* _cartridge) {
		if (instance == nullptr) {
			switch (_cartridge->console) {
			case Config::GB:
			case Config::GBC:
				instance = Gameboy::GameboyMMU::getInstance(_cartridge);
				break;
			}
		}

		return instance;
	}

	BaseMMU* BaseMMU::getInstance() {
		if (instance == nullptr) {
			LOG_ERROR("[emu] MMU instance is nullptr");
		}
		return instance;
	}

	void BaseMMU::resetInstance() {
		if (instance != nullptr) {
			delete instance;
			instance = nullptr;
		}
	}

	vector<char> BaseMMU::GetSaveData() {
		saveFinished.store(true);
		return saveData;
	}
}