#include "BaseCPU.h"
#include "GameboyCPU.h"

#include "logger.h"

#include "general_config.h"

BaseCPU* BaseCPU::instance = nullptr;

BaseCPU* BaseCPU::getInstance() {
	resetInstance();

	instance = new GameboyCPU(_machine_info);

	return instance;
}

void BaseCPU::resetInstance() {
	if (instance != nullptr) {
		BaseMMU::resetInstance();
		delete instance;
		instance = nullptr;
	}
}