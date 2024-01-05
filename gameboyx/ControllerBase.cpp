#include "ControllerBase.h"
#include "ControllerSM83.h"

#include "logger.h"

ControllerBase* ControllerBase::instance = nullptr;

ControllerBase* ControllerBase::getInstance(machine_information& _machine_info) {
	resetInstance();

	instance = new ControllerSM83(_machine_info);

	return instance;
}

void ControllerBase::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}