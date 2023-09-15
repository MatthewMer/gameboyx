#include "MmuBase.h"

#include "MmuSM83.h"
#include "logger.h"

MmuBase* MmuBase::instance = nullptr;

MmuBase* MmuBase::getInstance(const Cartridge& _cart_obj) {
	resetInstance();
	instance = new MmuSM83_MBC3(_cart_obj);

	LOG_INFO("MMU initialized");
	return instance;
}

void MmuBase::resetInstance() {
	if (instance != nullptr) {
		instance->ResetChildMemoryInstances();
		delete instance;
		instance = nullptr;
	}
}