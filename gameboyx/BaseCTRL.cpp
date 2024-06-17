#include "BaseCTRL.h"
#include "GameboyCTRL.h"

#include "logger.h"

namespace Emulation {
	BaseCTRL* BaseCTRL::instance = nullptr;

	BaseCTRL* BaseCTRL::getInstance(BaseCartridge* _cartridge) {
		if (instance == nullptr) {
			switch (_cartridge->console) {
			case Config::GB:
			case Config::GBC:
				instance = new Gameboy::GameboyCTRL(_cartridge);
				break;
			}
		}

		return instance;
	}

	void BaseCTRL::resetInstance() {
		if (instance != nullptr) {
			delete instance;
			instance = nullptr;
		}
	}
}