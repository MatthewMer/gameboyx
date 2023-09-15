#include "CoreBase.h"
#include "CoreSM83.h"

#include "logger.h"

CoreBase* CoreBase::instance = nullptr;

CoreBase* CoreBase::getInstance(const Cartridge& _cart_obj, message_fifo& _msg_fifo) {
	resetInstance();

	instance = new CoreSM83(_cart_obj, _msg_fifo);
	LOG_INFO("CPU initialized");
	return instance;
}

void CoreBase::resetInstance() {
	if (instance != nullptr) {
		MmuBase::resetInstance();
		delete instance;
		instance = nullptr;
	}
}