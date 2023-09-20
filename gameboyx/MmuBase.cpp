#include "MmuBase.h"

#include "MmuSM83.h"
#include "logger.h"

MmuBase* MmuBase::instance = nullptr;

MmuBase* MmuBase::getInstance() {
	resetInstance();

	instance = new MmuSM83_MBC3();
	return instance;
}

void MmuBase::resetInstance() {
	if (instance != nullptr) {
		instance->ResetChildMemoryInstances();
		delete instance;
		instance = nullptr;
	}
}