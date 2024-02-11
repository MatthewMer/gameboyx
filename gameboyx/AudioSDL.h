#pragma once

#include <format>

#include "AudioMgr.h"

class AudioSDL : AudioMgr {
public:
	friend class AudioMgr;
	void InitAudio(const bool& _reinit) override;

	void InitAudioBackend(BaseAPU* _sound_instance) override;
	void DestroyAudioBackend() override;

protected:
	explicit AudioSDL() : AudioMgr() {
		SDL_AudioSpec dev_props;
		char* c_name;
		SDL_GetDefaultAudioInfo(&c_name, &dev_props, 0);
		name = std::string(c_name);

		if (samplingRateMax > dev_props.freq) {
			samplingRateMax = dev_props.freq;
			samplingRate = dev_props.freq;
		} else {
			samplingRate = samplingRateMax;
		}

		if (audioChannelsMax > dev_props.channels) {
			audioChannelsMax = dev_props.channels;
			audioChannels = dev_props.channels;
		} else {
			audioChannels = audioChannelsMax;
		}

		LOG_INFO("[SDL] ", name, " supports: ", std::format("{:d} channels @ {:.1f}kHz", dev_props.channels, dev_props.freq / pow(10, 3)));
	}


private:
	SDL_AudioDeviceID device = {};
	SDL_AudioSpec want = {};
	SDL_AudioSpec have = {};
};

