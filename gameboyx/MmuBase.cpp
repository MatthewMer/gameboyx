#include "MmuBase.h"

#include "MmuSM83.h"
#include "logger.h"

#include <vector>

using namespace std;

/* ***********************************************************************************************************
	MMU BASE CLASS
*********************************************************************************************************** */
MmuBase* MmuBase::instance = nullptr;

MmuBase* MmuBase::getInstance(machine_information& _machine_info) {
	resetInstance();

	instance = getNewMmuInstance(_machine_info);
	if (instance == nullptr) {
		LOG_ERROR("Couldn't create MMU");
	}
	return instance;
}

void MmuBase::resetInstance() {
	if (instance != nullptr) {
		instance->ResetChildMemoryInstances();
		delete instance;
		instance = nullptr;
	}
}

MmuBase* MmuBase::getNewMmuInstance(machine_information& _machine_info) {
	const vector<u8>& vec_rom = Cartridge::getInstance()->GetRomVector();

	return MmuSM83::getInstance(_machine_info, vec_rom);
}

