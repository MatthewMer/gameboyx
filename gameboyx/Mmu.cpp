#include "Mmu.h"

Mmu* Mmu::instance = nullptr;

Mmu* Mmu::getInstance(const Cartridge& _cart_obj) {
	if (instance == nullptr) {
		instance = new MmuSM83(_cart_obj);
	}

	return instance;
}

void Mmu::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}

MmuSM83::MmuSM83(const Cartridge& _cart_obj) {
	Memory::resetInstance();
	mem_instance = Memory::getInstance(_cart_obj);
}