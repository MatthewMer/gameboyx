#include "BaseAPU.h"

#include "GameboyAPU.h"

#include "logger.h"

BaseAPU* BaseAPU::instance = nullptr;

BaseAPU* BaseAPU::getInstance(audio_information& _audio_info, AudioMgr* _audio_mgr) {
	resetInstance();

	instance = new GameboyAPU(_audio_info, _audio_mgr);

	return instance;
}

BaseAPU* BaseAPU::getInstance() {
	return instance;
}

void BaseAPU::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}

const apu_data& BaseAPU::GetApuData() {
	return apuData;
}