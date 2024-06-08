#pragma once

#include "HardwareStructs.h"
#include "general_config.h"
#include <thread>
#include <mutex>

class BaseAPU;

struct complex {
	float real;
	float imaginary;

	complex() {
		this->real = 0;
		this->imaginary = 0;
	}

	complex(float real, float imaginary) {
		this->real = real;
		this->imaginary = imaginary;
	}

	complex operator+(complex& _right) {
		return complex(real + _right.real, imaginary + _right.imaginary);
	}

	complex operator-(complex& _right) {
		return complex(real - _right.real, imaginary - _right.imaginary);
	}

	complex operator*(complex& _right) {
		return complex(
			real * _right.real - imaginary * _right.imaginary,
			real * _right.imaginary + _right.real * imaginary);
	}

	complex operator/(complex& _right) {
		return complex(
			((real * _right.real) + (imaginary * _right.imaginary)) / ((_right.real * _right.real) + (_right.imaginary * _right.imaginary)),
			((_right.real * imaginary) - (real * _right.imaginary)) / ((_right.real * _right.real) + (_right.imaginary * _right.imaginary))
		);
	}
};

void fft(complex* _samples, const int& _N);

struct audio_samples {
	std::vector<float> buffer;
	int buffer_size = 0;

	int read_cursor = 0;
	int write_cursor = 0;

	std::condition_variable notifyBufferUpdate;
	std::mutex mutBufferUpdate;
};

struct audio_information {
	int channels_max = 0;
	int sampling_rate_max = 0;

	int channels = 0;
	int sampling_rate = 0;

	void* device = nullptr;

	alignas(64) std::atomic<float> master_volume = 1.f;
};

class AudioMgr {
public:
	// get/reset instance
	static AudioMgr* getInstance();
	static void resetInstance();

	virtual void InitAudio(audio_settings& _audio_settings, const bool& _reinit) = 0;

	virtual bool InitAudioBackend(virtual_audio_information& _virt_audio_info) = 0;
	virtual void DestroyAudioBackend() = 0;

	void SetSamplingRate(audio_settings& _audio_settings);

	void SetMasterVolume(const float& _volume);

	// clone/assign protection
	AudioMgr(AudioMgr const&) = delete;
	AudioMgr(AudioMgr&&) = delete;
	AudioMgr& operator=(AudioMgr const&) = delete;
	AudioMgr& operator=(AudioMgr&&) = delete;

protected:
	// constructor
	explicit AudioMgr() {
		int sampling_rate_max = 0;
		for (const auto& [key, val] : SAMPLING_RATES) {
			if (val.first > sampling_rate_max) { sampling_rate_max = val.first; }
		}

		audioInfo.channels_max = SOUND_7_1;
		audioInfo.sampling_rate_max = sampling_rate_max;
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

