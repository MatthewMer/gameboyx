#include "CoreBase.h"
#include "CoreSM83.h"

#include "logger.h"

CoreBase* CoreBase::instance = nullptr;

CoreBase* CoreBase::getInstance(message_buffer& _msg_buffer) {
	resetInstance();

	instance = new CoreSM83(_msg_buffer);
	return instance;
}

void CoreBase::resetInstance() {
	if (instance != nullptr) {
		MmuBase::resetInstance();
		delete instance;
		instance = nullptr;
	}
}