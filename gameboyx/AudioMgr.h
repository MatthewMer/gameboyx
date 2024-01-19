#pragma once
#include "data_containers.h"
#include "al.h"
#include "alc.h"

class AudioMgr {
public:
	// get/reset instance
	static AudioMgr* getInstance(audio_information& _audio_info);
	static void resetInstance();

	// clone/assign protection
	AudioMgr(AudioMgr const&) = delete;
	AudioMgr(AudioMgr&&) = delete;
	AudioMgr& operator=(AudioMgr const&) = delete;
	AudioMgr& operator=(AudioMgr&&) = delete;

protected:
	// constructor
	AudioMgr(audio_information& _audio_info) : audioInfo(_audio_info) {}
	~AudioMgr() = default;

	virtual void InitAudio() = 0;

	audio_information& audioInfo;

	ALCdevice* alcDev;
	ALCcontext* alcCtx;
	std::vector<ALshort> data;
	ALuint buffer, source;

private:
	static AudioMgr* instance;
};