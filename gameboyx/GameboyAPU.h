#pragma once
#include "BaseAPU.h"
#include "GameboyMEM.h"

#include "gameboy_defines.h"
#include <vector>

class GameboyAPU : protected BaseAPU {
public:
	friend class BaseAPU;

	// members
	void ProcessAPU(const int& _ticks) override;
	void SampleAPU(std::vector<std::vector<complex>>& _data, std::vector<int>& _frequencies) override;

private:
	// constructor
	GameboyAPU(BaseCartridge* _cartridge) {
		memInstance = (GameboyMEM*)BaseMEM::getInstance();
		soundCtx = memInstance->GetSoundContext();

		virtual_audio_information virt_audio_info = {};
		virt_audio_info.channels = APU_CHANNELS_NUM;
		virt_audio_info.sound_instance = this;
		HardwareMgr::InitAudioBackend(virt_audio_info);
	}
	// destructor
	~GameboyAPU() override {
		HardwareMgr::DestroyAudioBackend();
	}

	int envelopeSweepCounter = 0;
	int soundLengthCounter = 0;
	int ch1FrequencyCounter = 0;

	GameboyMEM* memInstance = nullptr;
	sound_context* soundCtx = nullptr;
};