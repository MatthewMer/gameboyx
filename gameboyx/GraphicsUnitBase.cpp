#include "GraphicsUnitBase.h"

#include "GraphicsUnitSM83.h"
#include "logger.h"

GraphicsUnitBase* GraphicsUnitBase::instance = nullptr;

GraphicsUnitBase* GraphicsUnitBase::getInstance(const Cartridge& _cart_obj) {
	resetInstance();

	instance = new GraphicsUnitSM83(_cart_obj);
	return instance;
}

void GraphicsUnitBase::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}