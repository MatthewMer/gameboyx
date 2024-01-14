#include "BaseCTRL.h"
#include "GameboyCTRL.h"

#include "logger.h"

BaseCTRL* BaseCTRL::instance = nullptr;

BaseCTRL* BaseCTRL::getInstance(machine_information& _machine_info) {
	resetInstance();

	instance = new GameboyCTRL(_machine_info);

	return instance;
}

void BaseCTRL::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}