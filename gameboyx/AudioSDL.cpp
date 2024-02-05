#include "AudioSDL.h"
#include "format"
//#include "AudioMgr.h"
#include "BaseAPU.h"

#define _USE_MATH_DEFINES
#include <cmath>

using namespace std;

void audio_callback(void* userdata, u8* _device_buffer, int _length);
static int audio_thread(void* _user_data);
void sample_into_audio_buffer(audio_samples* _samples, const apu_data& _apu_data, const int& _region_1_size, const int& _region_2_size);

void AudioSDL::CheckAudio() {
	SDL_memset(&want, 0, sizeof(want));
	SDL_memset(&have, 0, sizeof(have));

	want.freq = SOUND_SAMPLING_RATE_MAX;
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
	want.userdata = &audioData.samples;
	device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);

	// audio info (settings)
	audioInfo.sampling_rate = have.freq;
	audioInfo.channels = (int)have.channels;

	// audio samples (audio api data)
	audioData.samples.buffer = std::vector<float>(SOUND_BUFFER_SIZE * audioInfo.channels, .0f);
	audioData.samples.format_size = SDL_AUDIO_BITSIZE(have.format) / 8;
	audioData.samples.buffer_size = (int)audioData.samples.buffer.size() * audioData.samples.format_size;
	audioData.samples.read_cursor = 0;
	audioData.samples.write_cursor = audioInfo.channels * audioData.samples.format_size;

	// finish audio data
	audioData.device = (void*)&device;
	audioData.audio_info = &audioInfo;

	SDL_PauseAudioDevice(device, 0);
	LOG_INFO("[SDL] audio set: ", format("{:d} channels @ {:d}Hz", audioInfo.channels, audioInfo.sampling_rate));
}

void AudioSDL::InitAudioBackend(BaseAPU* _sound_instance) {
	audioData.sound_instance = _sound_instance;
	audioData.audio_running = true;

	thread = SDL_CreateThread(audio_thread, "audio thread", (void*)&audioData);
	LOG_INFO("[SDL] audio backend initialized");
}

void AudioSDL::DestroyAudioBackend() {
	audioData.audio_running = false;
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

static int audio_thread(void* _user_data) {
	audio_data* data = (audio_data*)_user_data;
	audio_samples* samples = &data->samples;

	SDL_AudioDeviceID* device = (SDL_AudioDeviceID*)data->device;

	audio_information* audio_info = data->audio_info;

	BaseAPU* sound_instance = data->sound_instance;

	const apu_data& apu_data_ref = sound_instance->GetApuData();

	while (data->audio_running) {
		// sizes in bytes
		int region_1_size = samples->read_cursor - samples->write_cursor;
		int region_2_size = 0;
		if (samples->read_cursor < samples->write_cursor) {
			region_1_size = samples->buffer_size - samples->write_cursor;
			region_2_size = samples->read_cursor;
		}

		// sizes in samples
		int region_1_samples = (region_1_size / sizeof(float)) / audio_info->channels;
		int region_2_samples = (region_2_size / sizeof(float)) / audio_info->channels;

		sound_instance->SampleApuData(region_1_samples + region_2_samples, audio_info->sampling_rate);
		SDL_LockAudioDevice(*device);
		sample_into_audio_buffer(samples, apu_data_ref, region_1_size, region_2_size);
		SDL_UnlockAudioDevice(*device);
	}

	return 0;
}

void sample_into_audio_buffer(audio_samples* _samples, const apu_data& _apu_data, const int& _region_1_size, const int& _region_2_size) {
	float* buffer = _samples->buffer.data() + _samples->write_cursor;
	

	_samples->write_cursor = (_samples->write_cursor + _region_1_size + _region_2_size) % _samples->buffer_size;
}