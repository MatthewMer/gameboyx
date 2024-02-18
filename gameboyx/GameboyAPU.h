#pragma once
#include "BaseAPU.h"

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
		virtual_audio_information virt_audio_info = {};
		virt_audio_info.channels = APU_CHANNELS_NUM;
		virt_audio_info.sound_instance = this;
		HardwareMgr::InitAudioBackend(virt_audio_info);
	}
	// destructor
	~GameboyAPU() override {
		HardwareMgr::DestroyAudioBackend();
	}

	u8 divApuCounter = 0;
};