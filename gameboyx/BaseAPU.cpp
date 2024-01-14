#include "BaseAPU.h"

#include "GameboyAPU.h"

#include "logger.h"

BaseAPU* BaseAPU::instance = nullptr;

BaseAPU* BaseAPU::getInstance(machine_information& _machine_info) {
	resetInstance();

	instance = new GameboyAPU(_machine_info);

	return instance;
}

void BaseAPU::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}