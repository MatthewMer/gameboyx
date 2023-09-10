#include "CoreBase.h"
#include "CoreSM83.h"

Core* Core::instance = nullptr;

Core* Core::getInstance(const Cartridge& _cart_obj) {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}

	instance = new CoreSM83(_cart_obj);
	return instance;
}

void Core::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}