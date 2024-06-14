#pragma once

#include "HardwareStructs.h"
#include "general_config.h"
#include <thread>
#include <mutex>
#include <algorithm>

class BaseAPU;

using std::swap;

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

	complex(const complex& other) : real(other.real), imaginary(other.imaginary) {}

	complex& operator=(const complex& rhs) {
		complex tmp(rhs);
		std::swap(*this, tmp);
		return *this;
	}

	complex& operator+=(const complex& rhs) {
		real += rhs.real;
		imaginary += rhs.imaginary;
		return *this;
	}

	complex& operator-=(const complex& rhs) {
		real -= rhs.real;
		imaginary -= rhs.imaginary;
		return *this;
	}

	complex& operator*=(const complex& rhs) {
		real = real * rhs.real - imaginary * rhs.imaginary;
		imaginary = real * rhs.imaginary + imaginary * rhs.real;
		return *this;
	}

	complex& operator/=(const complex& rhs) {
		float divisor = (float)(pow(rhs.real, 2) + pow(rhs.imaginary, 2));
		real = (real * rhs.real + imaginary * rhs.imaginary) / divisor;
		imaginary = (imaginary * rhs.real - real * rhs.imaginary) / divisor;
		return *this;
	}

	complex operator!() const { 
		return complex(
			real,
			-imaginary
		);
	}
};

inline complex operator+(const complex& lhs, const complex& rhs) {
	complex res = lhs;
	res += rhs;
	return res;
}

inline complex operator-(const complex& lhs, const complex& rhs) {
	complex res = lhs;
	res -= rhs;
	return res;
}

inline complex operator*(const complex& lhs, const complex& rhs) {
	complex res = lhs;
	res *= rhs;
	return res;
}

inline complex operator/(const complex& lhs, const complex& rhs) {
	complex res = lhs;
	res /= rhs;
	return res;
}

void fft_cooley_tukey(complex* _samples, const int& _N);
void window_tukey(complex* _samples, const int& _N);

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
	alignas(64) std::atomic<float> lfe = 1.f;
	alignas(64) std::atomic<bool> volume_changed = false;

	alignas(64) std::atomic<float> delay = .02f;
	alignas(64) std::atomic<float> decay = .1f;
	alignas(64) std::atomic<bool> reload_reverb = false;
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

	void SetVolume(const float& _volume, const float& _lfe);
	void SetReverb(const float& _delay, const float& _decay);

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

