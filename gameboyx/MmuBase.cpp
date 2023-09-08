#include "MmuBase.h"

#include "MmuSM83.h"

Mmu* Mmu::instance = nullptr;

Mmu* Mmu::getInstance(const Cartridge& _cart_obj) {
	if (instance == nullptr) {
		instance = new MmuSM83_MBC3(_cart_obj);
	}

	return instance;
}

void Mmu::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}