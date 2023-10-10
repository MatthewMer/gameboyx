#include "CoreBase.h"
#include "CoreSM83.h"

#include "logger.h"

CoreBase* CoreBase::instance = nullptr;

CoreBase* CoreBase::getInstance(machine_information& _machine_info) {
	resetInstance();

	instance = new CoreSM83(_machine_info);

	return instance;
}

void CoreBase::resetInstance() {
	if (instance != nullptr) {
		MmuBase::resetInstance();
		delete instance;
		instance = nullptr;
	}
}