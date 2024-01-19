#pragma once

#include "AudioMgr.h"

#include "data_containers.h"

class AudioOpenAL : AudioMgr {
public:
	friend class AudioMgr;

protected:
	AudioOpenAL(audio_information& _audio_info) : AudioMgr(_audio_info) {}

private:
	void InitAudio() override;
};

