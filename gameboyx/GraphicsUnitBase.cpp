#include "GraphicsUnitBase.h"

#include "GraphicsUnitSM83.h"
#include "logger.h"

GraphicsUnitBase* GraphicsUnitBase::instance = nullptr;

GraphicsUnitBase* GraphicsUnitBase::getInstance(graphics_information& _graphics_info, VulkanMgr* _graphics_mgr) {
	resetInstance();
	instance = new GraphicsUnitSM83(_graphics_info, _graphics_mgr);

	return instance;
}

GraphicsUnitBase* GraphicsUnitBase::getInstance() {
	return instance;
}

void GraphicsUnitBase::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}