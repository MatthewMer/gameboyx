#pragma once

#include "AudioMgr.h"
#include "al.h"
#include "alc.h"

#include "data_containers.h"

class AudioOpenAL : AudioMgr {
public:
	friend class AudioMgr;
	void InitAudio(const bool& _reinit) override;

protected:
	AudioOpenAL(audio_information& _audio_info) : AudioMgr(_audio_info) {}

private:
	ALCdevice* alcDev;
	ALCcontext* alcCtx;
	std::vector<ALshort> data;
	ALuint buffer;
	ALuint source;
};

