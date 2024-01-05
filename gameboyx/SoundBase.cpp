#include "SoundBase.h"

#include "SoundSM83.h"

#include "logger.h"

SoundBase* SoundBase::instance = nullptr;

SoundBase* SoundBase::getInstance(machine_information& _machine_info) {
	resetInstance();

	instance = new SoundSM83(_machine_info);

	return instance;
}

void SoundBase::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}