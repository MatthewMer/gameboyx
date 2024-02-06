#pragma once

#include <format>

#include "AudioMgr.h"

#include "data_containers.h"

class AudioSDL : AudioMgr {
public:
	friend class AudioMgr;
	void InitAudio(const bool& _reinit) override;

	void InitAudioBackend(BaseAPU* _sound_instance) override;
	void DestroyAudioBackend() override;

protected:
	explicit AudioSDL(audio_information& _audio_info) : AudioMgr(_audio_info) {
		SDL_AudioSpec dev_props;
		SDL_GetDefaultAudioInfo(&name, &dev_props, 0);

		if (audioInfo.sampling_rate_max > dev_props.freq) {
			audioInfo.sampling_rate_max = dev_props.freq;
			audioInfo.sampling_rate = dev_props.freq;
		} else {
			audioInfo.sampling_rate = audioInfo.sampling_rate_max;
		}

		if (audioInfo.channels_max > dev_props.channels) {
			audioInfo.channels_max = dev_props.channels;
			audioInfo.channels = dev_props.channels;
		} else {
			audioInfo.channels = audioInfo.channels_max;
		}

		LOG_INFO("[SDL] ", name, " supports: ", std::format("{:d} channels @ {:.1f}kHz", dev_props.channels, dev_props.freq / pow(10, 3)));
	}


private:
	SDL_AudioDeviceID device = {};
	SDL_AudioSpec want = {};
	SDL_AudioSpec have = {};
};

