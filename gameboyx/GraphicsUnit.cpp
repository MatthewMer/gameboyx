#include "GraphicsUnit.h"

GraphicsUnit* GraphicsUnit::instance = nullptr;

GraphicsUnit* GraphicsUnit::getInstance(const Cartridge& _cart_obj) {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}

	instance = new GraphicsUnitSM83(_cart_obj);
	return instance;
}

void GraphicsUnit::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}

GraphicsUnitSM83::GraphicsUnitSM83(const Cartridge& _cart_obj) {
	mmu_instance = Mmu::getInstance(_cart_obj);
}