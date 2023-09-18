#include "GraphicsUnitBase.h"

#include "GraphicsUnitSM83.h"
#include "logger.h"

GraphicsUnitBase* GraphicsUnitBase::instance = nullptr;

GraphicsUnitBase* GraphicsUnitBase::getInstance() {
	resetInstance();

	instance = new GraphicsUnitSM83();
	return instance;
}

void GraphicsUnitBase::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}