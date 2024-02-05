#pragma once
#include "BaseAPU.h"

#include "gameboy_defines.h"
#include <vector>

class GameboyAPU : protected BaseAPU {
public:
	friend class BaseAPU;

	// members
	void ProcessAPU(const int& _ticks) override;
	void SampleApuData(const int& _samples_num, const int& _sampling_rate) override;

private:
	// constructor
	GameboyAPU(audio_information& _audio_info, AudioMgr* _audio_mgr) : BaseAPU(_audio_info, _audio_mgr) {
		apuData.channels = APU_CHANNELS_NUM;
		apuData.cursor = apuData.channels;
		apuData.buffer = std::vector<float>(apuData.channels * SOUND_BUFFER_SIZE, .0f);
		audioMgr->InitAudioBackend(this);
	}
	// destructor
	~GameboyAPU() override {
		audioMgr->DestroyAudioBackend();
	}

	u8 divApuCounter = 0;
};