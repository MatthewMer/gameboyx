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
	GameboyAPU(BaseCartridge* _cartridge) {
		virtual_audio_information virt_audio_info = {};
		virt_audio_info.channels = APU_CHANNELS_NUM;
		virt_audio_info.cursor = virt_audio_info.channels;
		audioData = std::vector<float>(virt_audio_info.channels * SOUND_BUFFER_SIZE, .0f);
		virt_audio_info.audio_data = &audioData;
		virt_audio_info.sound_instance = this;
		HardwareMgr::InitAudioBackend(virt_audio_info);
	}
	// destructor
	~GameboyAPU() override {
		HardwareMgr::DestroyAudioBackend();
	}

	u8 divApuCounter = 0;
};