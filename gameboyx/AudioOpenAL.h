#pragma once

#include "AudioMgr.h"
#include "al.h"
#include "alc.h"


class AudioOpenAL : AudioMgr {
public:
	friend class AudioMgr;
	void InitAudio(const bool& _reinit) override;

protected:
	AudioOpenAL() : AudioMgr() {}

private:
	ALCdevice* alcDev = nullptr;
	ALCcontext* alcCtx = nullptr;
	std::vector<ALshort> data = {};
	ALuint buffer = 0;
	ALuint source = 0;
};

