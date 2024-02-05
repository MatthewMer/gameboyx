#pragma once
#include "data_containers.h"
#include "general_config.h"
#include "SDL.h"

class BaseAPU;

struct audio_samples {
	std::vector<float> buffer;
	int format_size = 0;
	int buffer_size = 0;

	int read_cursor = 0;
	int write_cursor = 0;
};

struct audio_data {
	audio_samples samples = {};
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
	audio_data audioData = audio_data();
	SDL_Thread* thread = nullptr;

private:
	static AudioMgr* instance;
};