#pragma once
#include "data_containers.h"
#include "general_config.h"

struct audio_callback_data {
	std::vector<float> buffer;
	int read_cursor = 0;
	int write_cursor = 0;
	int format_size = 0;
	int buffer_size = 0;
};

class AudioMgr {
public:
	// get/reset instance
	static AudioMgr* getInstance(audio_information& _audio_info);
	static void resetInstance();

	virtual void CheckAudio() = 0;
	virtual void InitAudio(const bool& _reinit) = 0;

	// clone/assign protection
	AudioMgr(AudioMgr const&) = delete;
	AudioMgr(AudioMgr&&) = delete;
	AudioMgr& operator=(AudioMgr const&) = delete;
	AudioMgr& operator=(AudioMgr&&) = delete;

protected:
	// constructor
	AudioMgr(audio_information& _audio_info) : audioInfo(_audio_info) {}
	~AudioMgr() = default;

	audio_information& audioInfo;

	int samplingRate = 0;
	bool formatSigned = false;
	bool formatFloat = false;

	audio_callback_data callbackData = audio_callback_data();

private:
	static AudioMgr* instance;
};