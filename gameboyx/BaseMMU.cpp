#include "BaseMMU.h"

#include "GameboyMMU.h"
#include "logger.h"

#include <vector>

using namespace std;

/* ***********************************************************************************************************
	MMU BASE CLASS
*********************************************************************************************************** */
BaseMMU* BaseMMU::instance = nullptr;

BaseMMU* BaseMMU::getInstance(machine_information& _machine_info) {
	resetInstance();

	instance = getNewMmuInstance(_machine_info);
	if (instance == nullptr) {
		LOG_ERROR("Couldn't create MMU");
	}
	return instance;
}

void BaseMMU::resetInstance() {
	if (instance != nullptr) {
		instance->ResetChildMemoryInstances();
		delete instance;
		instance = nullptr;
	}
}

BaseMMU* BaseMMU::getNewMmuInstance(machine_information& _machine_info) {
	const vector<u8>& vec_rom = GameboyCartridge::getInstance()->GetRomVector();

	return GameboyMMU::getInstance(_machine_info, vec_rom);
}

