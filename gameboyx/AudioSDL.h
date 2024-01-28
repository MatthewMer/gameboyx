#pragma once

#include "AudioMgr.h"

#include "data_containers.h"

class AudioSDL : AudioMgr {
public:
	friend class AudioMgr;
	void CheckAudio() override;
	void InitAudio(const bool& _reinit) override;

protected:
	AudioSDL(audio_information& _audio_info) : AudioMgr(_audio_info) {}

private:
	SDL_AudioDeviceID device;
	SDL_AudioSpec want, have;
};

