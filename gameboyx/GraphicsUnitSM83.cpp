#include "GraphicsUnitSM83.h"

GraphicsUnitSM83::GraphicsUnitSM83(const Cartridge& _cart_obj) {
	mem_instance = MemorySM83::getInstance(_cart_obj);
}