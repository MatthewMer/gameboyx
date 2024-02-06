#include "AudioSDL.h"
#include "format"
//#include "AudioMgr.h"
#include "BaseAPU.h"

#define _USE_MATH_DEFINES
#include <cmath>

using namespace std;

void audio_callback(void* userdata, u8* _device_buffer, int _length);
void audio_thread(audio_env* _audio_env, audio_samples* _audio_samples);
void sample_into_audio_buffer(audio_samples* _samples, const apu_data& _apu_data, const int& _region_1_size, const int& _region_2_size);

void AudioSDL::InitAudio(const bool& _reinit) {
	if (_reinit) { SDL_CloseAudioDevice(device); }

	SDL_zero(want);
	SDL_zero(have);

	want.freq = audioInfo.sampling_rate;
	want.format = AUDIO_F32;
	want.channels = (u8)audioInfo.channels;
	want.samples = SOUND_BUFFER_SIZE;				// buffer size per channel
	want.callback = audio_callback;
	want.userdata = &audioSamples;
	device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);

	// audio info (settings)
	audioInfo.sampling_rate = have.freq;
	audioInfo.channels = (int)have.channels;

	// audio samples (audio api data)
	int format_size = SDL_AUDIO_BITSIZE(have.format) / 8;
	bool is_float = SDL_AUDIO_ISFLOAT(have.format);
	if (format_size != sizeof(float) || !is_float) {
		LOG_ERROR("[SDL] audio format not supported");
		SDL_CloseAudioDevice(device);
		return;
	}
	audioSamples.buffer = std::vector<float>(SOUND_BUFFER_SIZE * audioInfo.channels, .0f);
	audioSamples.buffer_size = (int)audioSamples.buffer.size() * sizeof(float);
	audioSamples.read_cursor = 0;
	audioSamples.write_cursor = audioInfo.channels * sizeof(float);

	// finish audio data
	audioEnv.device = (void*)&device;
	audioEnv.audio_info = &audioInfo;

	SDL_PauseAudioDevice(device, 0);
	LOG_INFO("[SDL] ", name, " set: ", format("{:d} channels @ {:.1f}kHz", audioInfo.channels, audioInfo.sampling_rate / pow(10, 3)));
}

void AudioSDL::InitAudioBackend(BaseAPU* _sound_instance) {
	audioEnv.sound_instance = _sound_instance;
	audioEnv.audio_running = true;

	audioThread = thread(audio_thread, &audioEnv, &audioSamples);
	LOG_INFO("[SDL] audio backend initialized");
}

void AudioSDL::DestroyAudioBackend() {
	audioEnv.audio_running = false;
	LOG_INFO("[SDL] audio backend stopped");
}

// _user_data: struct passed to audiospec, _device_buffer: the audio buffer snippet that needs to be filled, _length: length of this buffer snippet
void audio_callback(void* _user_data, u8* _device_buffer, int _length) {
	audio_samples* samples = (audio_samples*)_user_data;

	u8* region_2_buffer = (u8*)samples->buffer.data();
	u8* region_1_buffer = region_2_buffer + samples->read_cursor;

	int region_1_size, region_2_size;
	if (samples->read_cursor + _length > samples->buffer_size) {
		region_1_size = samples->buffer_size - samples->read_cursor;
		region_2_size = _length - region_1_size;
	} else {
		region_1_size = _length;
		region_2_size = 0;
	}

	SDL_memcpy(_device_buffer, region_1_buffer, region_1_size);
	SDL_memcpy(_device_buffer + region_1_size, region_2_buffer, region_2_size);

	samples->read_cursor = (samples->read_cursor + _length) % samples->buffer_size;
}

void audio_thread(audio_env* _audio_env, audio_samples* _audio_samples) {
	SDL_AudioDeviceID* device = (SDL_AudioDeviceID*)_audio_env->device;
	audio_information* audio_info = _audio_env->audio_info;
	BaseAPU* sound_instance = _audio_env->sound_instance;

	const apu_data& apu_data_ref = sound_instance->GetApuData();

	while (_audio_env->audio_running) {
		// sizes in bytes
		int region_1_size = _audio_samples->read_cursor - _audio_samples->write_cursor;
		int region_2_size = 0;
		if (_audio_samples->read_cursor < _audio_samples->write_cursor) {
			region_1_size = _audio_samples->buffer_size - _audio_samples->write_cursor;
			region_2_size = _audio_samples->read_cursor;
		}

		// sizes in samples
		int region_1_samples = (region_1_size / sizeof(float)) / audio_info->channels;
		int region_2_samples = (region_2_size / sizeof(float)) / audio_info->channels;

		sound_instance->SampleApuData(region_1_samples + region_2_samples, audio_info->sampling_rate);
		SDL_LockAudioDevice(*device);
		sample_into_audio_buffer(_audio_samples, apu_data_ref, region_1_size, region_2_size);
		SDL_UnlockAudioDevice(*device);
	}
}

void sample_into_audio_buffer(audio_samples* _samples, const apu_data& _apu_data, const int& _region_1_size, const int& _region_2_size) {
	float* buffer = _samples->buffer.data() + _samples->write_cursor;
	

	_samples->write_cursor = (_samples->write_cursor + _region_1_size + _region_2_size) % _samples->buffer_size;
}