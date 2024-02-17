#pragma once

#include "HardwareStructs.h"
#include "general_config.h"
#include <thread>

class BaseAPU;

struct complex {
	float real;
	float imaginary;
};

void fft(std::vector<complex>& _signal);

struct audio_samples {
	std::vector<float> buffer;
	int buffer_size = 0;

	int read_cursor = 0;
	int write_cursor = 0;
};

class AudioMgr {
public:
	// get/reset instance
	static AudioMgr* getInstance();
	static void resetInstance();

	virtual void InitAudio(const bool& _reinit) = 0;

	virtual bool InitAudioBackend(virtual_audio_information& _virt_audio_info) = 0;
	virtual void DestroyAudioBackend() = 0;

	// clone/assign protection
	AudioMgr(AudioMgr const&) = delete;
	AudioMgr(AudioMgr&&) = delete;
	AudioMgr& operator=(AudioMgr const&) = delete;
	AudioMgr& operator=(AudioMgr&&) = delete;

protected:
	// constructor
	explicit AudioMgr() {
		audioInfo.channels_max = SOUND_7_1;
		audioInfo.sampling_rate_max = SOUND_SAMPLING_RATE_MAX;
		audioInfo.channels = 0;
		audioInfo.sampling_rate = 0;
	}
	~AudioMgr() = default;

	std::string name = "";
	audio_samples audioSamples = audio_samples();
	std::thread audioThread;

	audio_information audioInfo = {};
	virtual_audio_information virtAudioInfo = {};

private:
	static AudioMgr* instance;
};

