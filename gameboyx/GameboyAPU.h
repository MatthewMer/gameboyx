#pragma once
#include "BaseAPU.h"
#include "GameboyMEM.h"

#include "gameboy_defines.h"
#include <vector>
#include <queue>

class GameboyAPU : protected BaseAPU {
public:
	friend class BaseAPU;

	// members
	void ProcessAPU(const int& _ticks) override;
	void SampleAPU(std::vector<std::vector<std::complex<float>>>& _data, const int& _samples, const int& _sampling_rate) override;

private:
	// constructor
	GameboyAPU(BaseCartridge* _cartridge) : BaseAPU() {
		memInstance = (GameboyMEM*)BaseMEM::getInstance();
		soundCtx = memInstance->GetSoundContext();

		virtualChannels = APU_CHANNELS_NUM;

		virtual_audio_information virt_audio_info = {};
		virt_audio_info.channels = virtualChannels;
		virt_audio_info.apu_callback = [this](std::vector<std::vector<std::complex<float>>>& _samples, const int& _num, const int& _sampling_rate) {
				this->SampleAPU(_samples, _num, _sampling_rate); 
			};
		HardwareMgr::InitAudioBackend(virt_audio_info);
	}
	// destructor
	~GameboyAPU() override {
		HardwareMgr::DestroyAudioBackend();
	}

	int envelopeSweepCounter = 0;
	int soundLengthCounter = 0;
	int ch1SamplingRateCounter = 0;
	
	int ch1PeriodSweepCounter = 0;
	int ch1LengthCounter = 0;
	int ch1EnvelopeSweepCounter = 0;
	void ch1PeriodSweep();
	void ch1TickLengthTimer();
	void ch1EnvelopeSweep();

	int ch1SampleCount = 0;
	float ch1VirtSamples = .0f;

	int ch2LengthCounter = 0;
	int ch2EnvelopeSweepCounter = 0;
	void ch2TickLengthTimer();
	void ch2EnvelopeSweep();

	int ch2SampleCount = 0;
	float ch2VirtSamples = .0f;

	int ch3LengthCounter = 0;
	void ch3TickLengthTimer();

	int ch3SampleCount = 0;
	float ch3VirtSamples = .0f;

	int ch4LengthCounter = 0;
	int ch4EnvelopeSweepCounter = 0;

	float ch4LFSRTickCounter = 0;
	std::mutex mutLFSR;
	std::vector<float> ch4LFSRSamples = std::vector<float>(CH_4_LFSR_BUFFER_SIZE);
	void ch4TickLengthTimer();
	void ch4EnvelopeSweep();

	float ch4VirtSamples = .0f;

	int virtualChannels = 0;

	alignas(64) std::atomic<int> ch4WriteCursor = 1;			// always points to the next sample to write
	alignas(64) std::atomic<int> ch4ReadCursor = 0;				// always points to the current sample to read

	void TickLFSR(const int& _ticks);

	GameboyMEM* memInstance = nullptr;
	sound_context* soundCtx = nullptr;
};