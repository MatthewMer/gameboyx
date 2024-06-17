#pragma once

#include <format>
#include "logger.h"

#include "AudioMgr.h"

namespace Backend {
	namespace Audio {
		class AudioSDL : AudioMgr {
		public:
			friend class AudioMgr;
			void InitAudio(audio_settings& _audio_settings, const bool& _reinit) override;

			bool InitAudioBackend(virtual_audio_information& _virt_audio_info) override;
			void DestroyAudioBackend() override;

		protected:
			explicit AudioSDL() : AudioMgr() {
				SDL_AudioSpec dev_props;
				char* c_name;
				SDL_GetDefaultAudioInfo(&c_name, &dev_props, 0);
				name = std::string(c_name);

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

				LOG_INFO("[SDL] ", name, " supports: ", std::format("{:d} channels @ {:d}Hz", dev_props.channels, dev_props.freq));
			}


		private:
			SDL_AudioDeviceID device = {};
			SDL_AudioSpec want = {};
			SDL_AudioSpec have = {};
		};
	}
}