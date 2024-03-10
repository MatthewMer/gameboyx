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
	audioSamples.buffer = std::vector<float>(SOUND_BUFFER_SIZE * audioInfo.channels * 4, .0f);
	audioSamples.buffer_size = (int)audioSamples.buffer.size() * sizeof(float);
	audioSamples.write_cursor = 0;
	audioSamples.read_cursor = (audioSamples.write_cursor - (have.samples * have.channels) + (int)audioSamples.buffer.size()) % (int)audioSamples.buffer.size();

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
	for (auto& n : audioSamples.buffer) {
		n = .0f;
	}
	LOG_INFO("[SDL] audio backend stopped");
}

// _user_data: struct passed to audiospec, _device_buffer: the audio buffer snippet that needs to be filled, _length: length of this buffer snippet
void audio_callback(void* _user_data, u8* _device_buffer, int _length) {
	audio_samples* samples = (audio_samples*)_user_data;

	float* reg_2 = samples->buffer.data();
	float* reg_1 = reg_2 + samples->read_cursor;

	int b_read_cursor = samples->read_cursor * sizeof(float);

	int reg_1_size, reg_2_size;
	if (b_read_cursor + _length > samples->buffer_size) {
		reg_1_size = samples->buffer_size - b_read_cursor;
		reg_2_size = _length - reg_1_size;
	} else {
		reg_1_size = _length;
		reg_2_size = 0;
	}

	SDL_memcpy(_device_buffer, reg_1, reg_1_size);
	SDL_memcpy(_device_buffer + reg_1_size, reg_2, reg_2_size);

	samples->read_cursor = ((b_read_cursor + _length) % samples->buffer_size) / sizeof(float);
}

void audio_thread(audio_information* _audio_info, virtual_audio_information* _virt_audio_info, audio_samples* _samples) {
	SDL_AudioDeviceID* device = (SDL_AudioDeviceID*)_audio_info->device;
	BaseAPU* sound_instance = _virt_audio_info->sound_instance;

	const int channels = _audio_info->channels;
	const int sampling_rate = _audio_info->sampling_rate;

	const int virt_channels = _virt_audio_info->channels;

	// filled with samples per period of virtual channels
	float* virt_samples = new float[virt_channels];
	for (int i = 0; i < virt_channels; i++) {
		virt_samples[i] = .0f;
	}

	std::vector<float> virt_angles;
	{
		// TODO: revise translation from virtual to physical channels (something is here off, too)
		float step = (float)((360.f - 45.f) / _virt_audio_info->channels);
		int i = 0;
		for (float a = 45.f; i < _virt_audio_info->channels; a += step, i++) {
			virt_angles.push_back(a);
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
		int reg_1_size, reg_2_size;
		if (_samples->read_cursor < _samples->write_cursor) {
			reg_1_size = (int)_samples->buffer.size() - _samples->write_cursor;
			reg_2_size = _samples->read_cursor;
		} else if (_samples->read_cursor > _samples->write_cursor) {
			reg_1_size = _samples->read_cursor - _samples->write_cursor;
			reg_2_size = 0;
		} else {
			reg_1_size = 0;
			reg_2_size = 0;
		}

		/*
		if (reg_1_size || reg_2_size) {
			LOG_WARN(_samples->read_cursor, ";", _samples->write_cursor);
			LOG_WARN(reg_1_size, ";", reg_2_size);
		}
		*/

		if (reg_1_size || reg_2_size) {
			// transfer samples into ringbuffer
			float* buffer = _samples->buffer.data() + _samples->write_cursor;
			for (int i = 0; i < reg_1_size / channels; i++) {
				for (int j = 0; j < channels; j++) {
					buffer[j] = .0f;
				}
				for (int j = 0; j < virt_channels; j++) {
					virt_samples[j] = .0f;
				}

				sound_instance->SampleAPU(virt_samples);
				for (int j = 0; j < virt_channels; j++) {
					(*speaker_fn)(buffer, virt_samples[j], virt_angles[j]);
				}

				buffer += channels;
			}

			buffer = _samples->buffer.data();
			for (int i = 0; i < reg_2_size / channels; i++) {
				for (int j = 0; j < channels; j++) {
					buffer[j] = .0f;
				}
				for (int j = 0; j < virt_channels; j++) {
					virt_samples[j] = .0f;
				}

				sound_instance->SampleAPU(virt_samples);
				for (int j = 0; j < virt_channels; j++) {
					//(*speaker_fn)(buffer, virt_samples[j], virt_angles[j]);
				}

				buffer += channels;
			}
		}
		SDL_UnlockAudioDevice(*device);

		_samples->write_cursor = (_samples->write_cursor + reg_1_size + reg_2_size) % (int)_samples->buffer.size();
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
	_dest[0] = calc_sample(_sample, _angle, SOUND_STEREO_ANGLES[0]);
	_dest[0] += calc_sample(_sample, _angle, SOUND_STEREO_ANGLES[1]);
}