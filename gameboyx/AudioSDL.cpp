#include "AudioSDL.h"
#include "format"
#include "AudioMgr.h"
#include "BaseAPU.h"
#include "logger.h"

#define _USE_MATH_DEFINES
#include <cmath>

using namespace std;

void audio_callback(void* userdata, u8* _device_buffer, int _length);
void audio_thread(audio_information* _audio_info, virtual_audio_information* _virt_audio_info, audio_samples* _samples);

typedef void (*speaker_function)(float*, const float&, const float&);
void samples_7_1_surround(float* _dest, const float& _sample, const float& _angle);
void samples_5_1_surround(float* _dest, const float& _sample, const float& _angle);
void samples_stereo(float* _dest, const float& _sample, const float& _angle);
void samples_mono(float* _dest, const float& _sample, const float& _angle);

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
	audioSamples.buffer = std::vector<float>(SOUND_BUFFER_SIZE * audioInfo.channels * 10, .0f);
	audioSamples.buffer_size = (int)audioSamples.buffer.size() * sizeof(float);
	audioSamples.read_cursor = 0;
	audioSamples.write_cursor = audioInfo.channels * sizeof(float);

	// finish audio data
	audioInfo.device = (void*)&device;

	SDL_PauseAudioDevice(device, 0);
	LOG_INFO("[SDL] ", name, " set: ", format("{:d} channels @ {:.1f}kHz", audioInfo.channels, audioInfo.sampling_rate / pow(10, 3)));
}

bool AudioSDL::InitAudioBackend(virtual_audio_information& _virt_audio_info) {
	virtAudioInfo = _virt_audio_info;
	virtAudioInfo.audio_running.store(true);

	audioThread = thread(audio_thread, &audioInfo, &virtAudioInfo, &audioSamples);
	if (!audioThread.joinable()) {
		return false;
	} else {
		LOG_INFO("[SDL] audio backend initialized");
		return true;
	}
}

void AudioSDL::DestroyAudioBackend() {
	virtAudioInfo.audio_running.store(false);
	if (audioThread.joinable()) {
		audioThread.join();
	}
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

void audio_thread(audio_information* _audio_info, virtual_audio_information* _virt_audio_info, audio_samples* _samples) {
	SDL_AudioDeviceID* device = (SDL_AudioDeviceID*)_audio_info->device;
	BaseAPU* sound_instance = _virt_audio_info->sound_instance;

	const int channels = _audio_info->channels;
	const float sampling_rate = (float)_audio_info->sampling_rate;

	const int virt_channels = _virt_audio_info->channels;

	// filled with samples per period of virtual channels
	std::vector<std::vector<float>> virt_samples = std::vector<std::vector<float>>(virt_channels);
	std::vector<int> virt_frequencies = std::vector<int>(virt_channels);
	std::vector<int> virt_frequencies_copy = std::vector<int>(virt_channels);

	// get ratios to fit samples into physical sampling rate
	std::vector<float> sampling_rate_ratios = std::vector<float>(virt_channels, 1.f);

	std::vector<float> virt_angles;
	{
		float step = (float)((2 * M_PI) / _virt_audio_info->channels);
		for (float i = .0f; i < 360.f; i += step) {
			virt_angles.push_back(i);
		}
	}

	speaker_function speaker_fn;
	switch (_audio_info->channels) {
	case SOUND_7_1:
		speaker_fn = &samples_7_1_surround;
		break;
	case SOUND_5_1:
		speaker_fn = &samples_5_1_surround;
		break;
	case SOUND_STEREO:
		speaker_fn = &samples_stereo;
		break;
	case SOUND_MONO:
		speaker_fn = &samples_mono;
		break;
	default:
		LOG_ERROR("[SDL] speaker configuration currently not supported");
		_virt_audio_info->audio_running.store(false);
		return;
		break;
	}

	

	while (_virt_audio_info->audio_running.load()) {
		SDL_LockAudioDevice(*device);
		// sizes in bytes
		int region_1_size = _samples->read_cursor - _samples->write_cursor;
		int region_2_size = 0;
		if (_samples->read_cursor < _samples->write_cursor) {
			region_1_size = _samples->buffer_size - _samples->write_cursor;
			region_2_size = _samples->read_cursor;
		}

		// sizes in samples
		int region_1_samples = (region_1_size / sizeof(float)) / _audio_info->channels;
		int region_2_samples = (region_2_size / sizeof(float)) / _audio_info->channels;

		// ***** transfer: channels n to channels m *****

		float* buffer = (float*)(((u8*)_samples->buffer.data()) + _samples->write_cursor);

		// keep track of current sample to avoid cutting off the signal
		float current_sample = .0f;

		// get samples for each channel of virtual hardware
		if (region_1_size > 0) {
			// get samples per period for all channels
			//sound_instance->SampleAPU(virt_samples, virt_frequencies);

			for (int i = 0; i < virt_channels; i++) {
				if (virt_frequencies_copy[i] != virt_frequencies[i]) {							// in case the signal changed
					//fft(virt_samples[i].data(), (int)virt_samples[i].size());						// perform fft on one period			
					for (int i = 0; auto & n : sampling_rate_ratios) {
						n = (virt_samples[i].size() * virt_frequencies[i]) / sampling_rate;			// get ratio to stretch/compress to fit physical sampling rate
						i++;
					}
					virt_frequencies_copy = virt_frequencies;
				}
			}

			// TODO: process fft result and perform inverse fft

			for (int i = 0; i < region_1_size; i++) {
				for (int j = 0; j < channels; j++) {
					buffer[j] = .0f;
				}

				// fill buffer
			}
		}

		buffer = _samples->buffer.data();
		if (region_2_size > 0) {
			for (int i = 0; i < region_1_samples; i++) {
				for (int j = 0; j < channels; j++) {
					buffer[j] = .0f;
				}

				// fill buffer
			}
		}

		_samples->write_cursor = (_samples->write_cursor + region_1_size + region_2_size) % _samples->buffer_size;

		SDL_UnlockAudioDevice(*device);
	}
}

const float x = .5f;
const float D = 1.2f;		// gain
const float a = 2.f;

float calc_sample(const float& _sample, const float& _sample_angle, const float& _speaker_angle) {
	return tanh(D * _sample) * exp(a * .5f * cos(_sample_angle - _speaker_angle) - .5f);
}

// TODO: low frequency missing, probably use a low pass filter on all samples and combine and increase amplitude and output on low frequency channel
void samples_7_1_surround(float* _dest, const float& _sample, const float& _angle) {
	_dest[0] += calc_sample(_sample, _angle, SOUND_7_1_ANGLES[0]);
	_dest[1] += calc_sample(_sample, _angle, SOUND_7_1_ANGLES[1]);
	_dest[2] += calc_sample(_sample, _angle, SOUND_7_1_ANGLES[2]);
	_dest[4] += calc_sample(_sample, _angle, SOUND_7_1_ANGLES[4]);
	_dest[5] += calc_sample(_sample, _angle, SOUND_7_1_ANGLES[5]);
	_dest[6] += calc_sample(_sample, _angle, SOUND_7_1_ANGLES[6]);
	_dest[7] += calc_sample(_sample, _angle, SOUND_7_1_ANGLES[7]);
}

void samples_5_1_surround(float* _dest, const float& _sample, const float& _angle) {
	_dest[0] += calc_sample(_sample, _angle, SOUND_5_1_ANGLES[0]);
	_dest[1] += calc_sample(_sample, _angle, SOUND_5_1_ANGLES[1]);
	_dest[2] += calc_sample(_sample, _angle, SOUND_5_1_ANGLES[2]);
	_dest[4] += calc_sample(_sample, _angle, SOUND_5_1_ANGLES[4]);
	_dest[5] += calc_sample(_sample, _angle, SOUND_5_1_ANGLES[5]);
}

void samples_stereo(float* _dest, const float& _sample, const float& _angle) {
	_dest[0] += calc_sample(_sample, _angle, SOUND_STEREO_ANGLES[0]);
	_dest[1] += calc_sample(_sample, _angle, SOUND_STEREO_ANGLES[1]);
}

void samples_mono(float* _dest, const float& _sample, const float& _angle) {
	//_dest[0] = calc_sample(_sample, _angle, SOUND_STEREO_ANGLES[0]);
}