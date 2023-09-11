#include "GraphicsUnitSM83.h"

GraphicsUnitSM83::GraphicsUnitSM83(const Cartridge& _cart_obj) {
	mmu_instance = MmuBase::getInstance(_cart_obj);
}