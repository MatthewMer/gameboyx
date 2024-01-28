#include "AudioSDL.h"
#include "format"

#define _USE_MATH_DEFINES
#include <cmath>

using namespace std;

void audio_callback(void* userdata, u8* _device_buffer, int _length);

void AudioSDL::CheckAudio() {
	SDL_memset(&want, 0, sizeof(want));
	SDL_memset(&have, 0, sizeof(have));

	want.freq = SOUND_SAMPLING_RATE;
	want.format = AUDIO_F32;
	want.channels = SOUND_7_1;
	want.samples = SOUND_BUFFER_SIZE;
	want.callback = audio_callback;
	device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);

	audioInfo.max_channels = have.channels;
	audioInfo.max_sampling_rate = have.freq;
	audioInfo.channels = audioInfo.max_channels;
	audioInfo.sampling_rate = audioInfo.max_sampling_rate;

	SDL_CloseAudioDevice(device);
	LOG_INFO("[SDL] audio supported: ", format("{:d} channels @ {:d}Hz", audioInfo.max_channels, audioInfo.max_sampling_rate));
}

void AudioSDL::InitAudio(const bool& _reinit) {
	if (_reinit) { SDL_CloseAudioDevice(device); }

	SDL_zero(want);
	SDL_zero(have);

	want.freq = audioInfo.sampling_rate;
	want.format = AUDIO_F32;
	want.channels = (u8)audioInfo.channels;
	want.samples = SOUND_BUFFER_SIZE;
	want.callback = audio_callback;
	want.userdata = &callbackData;
	device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);

	audioInfo.sampling_rate = have.freq;
	audioInfo.channels = (int)have.channels;

	formatFloat = SDL_AUDIO_ISFLOAT(have.format);
	formatSigned = SDL_AUDIO_ISSIGNED(have.format);

	callbackData.buffer = std::vector<float>(SOUND_BUFFER_SIZE * audioInfo.channels, .0f);
	callbackData.format_size = SDL_AUDIO_BITSIZE(have.format) / 8;
	callbackData.buffer_size = SOUND_BUFFER_SIZE * audioInfo.channels * callbackData.format_size;
	callbackData.read_cursor = 0;
	callbackData.write_cursor = audioInfo.channels * callbackData.format_size;

	// only for testing, doesnt sound good because the wave gets cut at some point and starts again at 0
	for (int i = 0; i < callbackData.buffer.size(); i+=audioInfo.channels) {
		float sample = (sin(2 * M_PI * 96.f * ((float)(i / audioInfo.channels) / audioInfo.sampling_rate)) > 0 ? .1f : -.1f);

		// fills for 5.1 and 7.1 surround the samples for the front-left/-right speaker
		callbackData.buffer[i] = sample;
		callbackData.buffer[i + 1] = sample;
	}

	SDL_PauseAudioDevice(device, 0);

	LOG_INFO("[SDL] audio set: ", format("{:d} channels @ {:d}Hz", audioInfo.channels, audioInfo.sampling_rate));
}

void audio_callback(void* _user_data, u8* _device_buffer, int _length) {
	audio_callback_data* callback_data = (audio_callback_data*)_user_data;

	u8* region_2_buffer = (u8*)callback_data->buffer.data();
	u8* region_1_buffer = region_2_buffer + callback_data->read_cursor;

	int region_1_size, region_2_size;
	if (callback_data->read_cursor + _length > callback_data->buffer_size) {
		region_1_size = callback_data->buffer_size - callback_data->read_cursor;
		region_2_size = _length - region_1_size;
	} else {
		region_1_size = _length;
		region_2_size = 0;
	}

	SDL_memcpy(_device_buffer, region_1_buffer, region_1_size);
	SDL_memcpy(_device_buffer + region_1_size, region_2_buffer, region_2_size);

	callback_data->read_cursor = (callback_data->read_cursor + _length) % callback_data->buffer_size;
}