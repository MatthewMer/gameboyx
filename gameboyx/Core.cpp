#include "Core.h"

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

CoreSM83::CoreSM83(const Cartridge& _cart_obj) {
	Mmu::resetInstance();
	mmu_instance = Mmu::getInstance(_cart_obj);

	regs = registers_gbc();
}