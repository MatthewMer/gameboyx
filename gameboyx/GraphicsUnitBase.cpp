#include "GraphicsUnitBase.h"

#include "GraphicsUnitSM83.h"
#include "logger.h"

GraphicsUnitBase* GraphicsUnitBase::instance = nullptr;

GraphicsUnitBase* GraphicsUnitBase::getInstance(graphics_information& _graphics_info) {
	resetInstance();

	instance = new GraphicsUnitSM83(_graphics_info);
	return instance;
}

void GraphicsUnitBase::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}