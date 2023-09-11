#include "CoreBase.h"
#include "CoreSM83.h"

#include "logger.h"

CoreBase* CoreBase::instance = nullptr;

CoreBase* CoreBase::getInstance(const Cartridge& _cart_obj) {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}

	instance = new CoreSM83(_cart_obj);
	return instance;
}

void CoreBase::resetInstance() {
	if (instance != nullptr) {
		MmuBase::resetInstance();
		delete instance;
		instance = nullptr;
	}
}