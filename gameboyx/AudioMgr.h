#pragma once
#include "data_containers.h"
#include "general_config.h"
#include <thread>

class BaseAPU;

struct audio_samples {
	std::vector<float> buffer;
	int buffer_size = 0;

	int read_cursor = 0;
	int write_cursor = 0;
};

struct audio_env {
	void* device = nullptr;
	audio_information* audio_info = nullptr;
	bool audio_running = true;
	BaseAPU* sound_instance = nullptr;
};

class AudioMgr {
public:
	// get/reset instance
	static AudioMgr* getInstance(audio_information& _audio_info);
	static void resetInstance();

	virtual void CheckAudio() = 0;
	virtual void InitAudio(const bool& _reinit) = 0;

	virtual void InitAudioBackend(BaseAPU* _sound_instance) = 0;
	virtual void DestroyAudioBackend() = 0;

	// clone/assign protection
	AudioMgr(AudioMgr const&) = delete;
	AudioMgr(AudioMgr&&) = delete;
	AudioMgr& operator=(AudioMgr const&) = delete;
	AudioMgr& operator=(AudioMgr&&) = delete;

protected:
	// constructor
	explicit AudioMgr(audio_information& _audio_info) : audioInfo(_audio_info) {}
	~AudioMgr() = default;

	audio_information& audioInfo;
	audio_env audioEnv = audio_env();
	audio_samples audioSamples = audio_samples();
	std::thread audioThread;

private:
	static AudioMgr* instance;
};