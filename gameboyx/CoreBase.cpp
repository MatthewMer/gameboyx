#include "CoreBase.h"
#include "CoreSM83.h"

#include "logger.h"

CoreBase* CoreBase::instance = nullptr;

CoreBase* CoreBase::getInstance(machine_information& _machine_info, graphics_information& _graphics_info, VulkanMgr* _graphics_mgr) {
	resetInstance();

	instance = new CoreSM83(_machine_info, _graphics_info, _graphics_mgr);

	return instance;
}

void CoreBase::resetInstance() {
	if (instance != nullptr) {
		MmuBase::resetInstance();
		GraphicsUnitBase::resetInstance();
		delete instance;
		instance = nullptr;
	}
}