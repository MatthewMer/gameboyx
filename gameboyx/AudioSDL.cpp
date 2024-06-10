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

typedef void (*speaker_function)(float*, const int&, const int&, const float&, const float&, const float&, const float&, const float&);
void samples_7_1_surround(float* _buffer, const int& _size, const int& _offset, const float& _sample, const float& _angle, const float& _vol, const float& _lfe, const float& _delay_sample);
void samples_5_1_surround(float* _buffer, const int& _size, const int& _offset, const float& _sample, const float& _angle, const float& _vol, const float& _lfe, const float& _delay_sample);
void samples_stereo(float* _buffer, const int& _size, const int& _offset, const float& _sample, const float& _angle, const float& _vol, const float& _lfe, const float& _delay_sample);
void samples_mono(float* _buffer, const int& _size, const int& _offset, const float& _sample, const float& _angle, const float& _vol, const float& _lfe, const float& _delay_sample);

float reverb(float* _buffer, const int& _size, int& _cursor, const float& _decay, const float& _sample);

void AudioSDL::InitAudio(audio_settings& _audio_settings, const bool& _reinit) {
	bool _reinit_backend = virtAudioInfo.audio_running.load();

	if (_reinit) {
		if (_reinit_backend) {
			DestroyAudioBackend();
		}
		SDL_CloseAudioDevice(device);
	}

	SDL_zero(want);
	SDL_zero(have);

	if (_audio_settings.sampling_rate > audioInfo.sampling_rate_max) {
		_audio_settings.sampling_rate = audioInfo.sampling_rate_max;
	}

	Uint16 buff_size = 512;
	for (const auto& [key, val] : SAMPLING_RATES) {
		if (val.first == _audio_settings.sampling_rate) {
			buff_size = (Uint16)val.second;
		}
	}

	want.freq = _audio_settings.sampling_rate;
	want.format = AUDIO_F32;
	want.channels = (u8)audioInfo.channels;
	want.samples = buff_size;						// buffer size per channel
	want.callback = audio_callback;
	want.userdata = &audioSamples;
	device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);

	// audio info (settings)
	audioInfo.sampling_rate = have.freq;
	audioInfo.channels = (int)have.channels;

	_audio_settings.sampling_rate = have.freq;
	_audio_settings.sampling_rate_max = audioInfo.sampling_rate_max;

	audioInfo.master_volume.store(_audio_settings.master_volume);
	audioInfo.lfe.store(_audio_settings.lfe);

	// audio samples (audio api data)
	int format_size = SDL_AUDIO_BITSIZE(have.format) / 8;
	bool is_float = SDL_AUDIO_ISFLOAT(have.format);
	if (format_size != sizeof(float) || !is_float) {
		LOG_ERROR("[SDL] audio format not supported");
		SDL_CloseAudioDevice(device);
		return;
	}
	audioSamples.buffer = std::vector<float>(buff_size * audioInfo.channels, .0f);
	audioSamples.buffer_size = (int)audioSamples.buffer.size() * sizeof(float);
	audioSamples.write_cursor = 0;
	audioSamples.read_cursor = (audioSamples.write_cursor - (have.samples * have.channels) + (int)audioSamples.buffer.size()) % (int)audioSamples.buffer.size();

	// finish audio data
	audioInfo.device = (void*)&device;

	SDL_PauseAudioDevice(device, 0);
	LOG_INFO("[SDL] ", name, " set: ", format("{:d} channels @ {:d}Hz", audioInfo.channels, audioInfo.sampling_rate));

	if (_reinit && _reinit_backend) {
		InitAudioBackend(virtAudioInfo);
	}
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
	audioSamples.notifyBufferUpdate.notify_one();
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

	samples->notifyBufferUpdate.notify_one();
}

void audio_thread(audio_information* _audio_info, virtual_audio_information* _virt_audio_info, audio_samples* _samples) {
	SDL_AudioDeviceID* device = (SDL_AudioDeviceID*)_audio_info->device;
	BaseAPU* sound_instance = _virt_audio_info->sound_instance;

	const int channels = _audio_info->channels;
	const int sampling_rate = _audio_info->sampling_rate;
	const int buffer_size = _samples->buffer_size / channels;

	const int virt_channels = _virt_audio_info->channels;

	const int delay = (int)(.02f * sampling_rate);
	const float decay = .1f;
	std::vector<float> delay_buffer = std::vector<float>(delay, .0f);
	int delay_cursor = 0;

	// filled with samples per period of virtual channels
	std::vector<std::vector<complex>> virt_samples = std::vector<std::vector<complex>>(virt_channels);

	std::vector<float> virt_angles;
	{
		float a;
		if (_audio_info->channels == SOUND_7_1 || _audio_info->channels == SOUND_5_1) {
			a = 22.5f;
		} else {
			a = 45.f;
		}

		float step = (float)((360.f - (2 * a)) / (_virt_audio_info->channels - 1));

		for (int i = 0; i < _virt_audio_info->channels; i++) {
			virt_angles.push_back(a * (float)(M_PI / 180.f));
			a += step;
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
		std::unique_lock<mutex> lock_buffer(_samples->mutBufferUpdate);
		_samples->notifyBufferUpdate.wait(lock_buffer);

		float volume = _audio_info->master_volume.load();
		float lfe = _audio_info->lfe.load();

		SDL_LockAudioDevice(*device);
		int reg_1_size, reg_2_size;
		if (_samples->read_cursor <= _samples->write_cursor) {
			reg_1_size = (int)_samples->buffer.size() - _samples->write_cursor;
			reg_2_size = _samples->read_cursor;
		} else if (_samples->read_cursor > _samples->write_cursor) {
			reg_1_size = _samples->read_cursor - _samples->write_cursor;
			reg_2_size = 0;
		}

		if (reg_1_size || reg_2_size) {
			// get samples from APU
			int reg_1_samples = reg_1_size / channels;
			int reg_2_samples = reg_2_size / channels;

			// TODO: use FFT and other stuff for different effects
			u32 size = reg_1_samples + reg_2_samples;
			//while (size) { if (size == 1) {/* size is power of 2 */} size >>= 1; }
			int exp = (int)std::ceil(log2(size));
			int buf_size = (int)pow(2, exp);

			for (auto& n : virt_samples) { 
				n.assign(buf_size, {});				// resize to power of two and reset imaginary part
			}
			sound_instance->SampleAPU(virt_samples, size, sampling_rate);

			/*
			for (auto& samples : virt_samples) {
				fft(samples.data(), buf_size);

				LOG_WARN("-------------");
				for (int i = 0; const auto & m : samples) {		// only results in range 0<=x<N/2 (N = samples) matter
					LOG_INFO(i, ". frequency: ", (float)sampling_rate * i / buf_size);				// i = frequency index and the number of samples, as the actual frequency was not taken into account during FFT it needs to be set into relation with the sampling rate R of the given signal. DFT just shifts sinusoids in the scope of the samples in range 0<=x<R/2 (with step size R/N) along the signal and determines how well they match (FFT in Divide-and-Conquer manner). In other words, it assumes that the sampling rate = N.
					LOG_INFO(i, ". magnitude: ", sqrt(pow(m.real, 2) + pow(m.imaginary, 2)));		// regular pythagorean theorem -> magnitude of plotted sinusoid: y=imaginary, x=real (no matter the applied phase shift or frequency to e^(-i*2*PI*f*t)=cos(2*PI*f*t)-i*sin(2*PI*f*t), it just rotates in the complex plane with a fixed radius)
					LOG_INFO(i, ". phase: ", atan2(m.imaginary, m.real) * 180.f / M_PI);			// regular atan just gives angle in range +/-90° -> atan2(y,x); y=imaginary, x=real in the complex plane which gives us the shift in rad of the plotted sinusoid (e^(-i*2*PI*f*t)=e^(-i*2PI*f*t+2PI) -> sinusoid sifted by 360°)
					i++;
				}
			}
			*/

			float* delay_buf = delay_buffer.data();

			float* buffer = _samples->buffer.data();
			int offset = _samples->write_cursor;
			for (int i = 0; i < reg_1_samples; i++) {
				for (int j = 0; j < channels; j++) {
					buffer[offset + j] = .0f;
				}

				float delay_next = .0f;
				for (int j = 0; j < virt_channels; j++) {
					delay_next += virt_samples[j][i].real;
				}

				float delay_cur = reverb(delay_buf, delay, delay_cursor, decay, delay_next);
				for (int j = 0; j < virt_channels; j++) {
					(*speaker_fn)(buffer, buffer_size, offset, virt_samples[j][i].real, virt_angles[j], volume, lfe, delay_cur);
				}

				offset += channels;
			}

			offset = 0;
			for (int i = 0; i < reg_2_samples; i++) {
				for (int j = 0; j < channels; j++) {
					buffer[offset + j] = .0f;
				}

				float delay_next = .0f;
				for (int j = 0; j < virt_channels; j++) {
					delay_next += virt_samples[j][reg_1_samples + i].real;
				}

				float delay_cur = reverb(delay_buf, delay, delay_cursor, decay, delay_next);
				for (int j = 0; j < virt_channels; j++) {
					(*speaker_fn)(buffer, buffer_size, offset, virt_samples[j][reg_1_samples + i].real, virt_angles[j], volume, lfe, delay_cur);
				}

				offset += channels;
			}
		}
		_samples->write_cursor = (_samples->write_cursor + reg_1_size + reg_2_size) % (int)_samples->buffer.size();
		SDL_UnlockAudioDevice(*device);
	}
}

const float x = .5f;
const float D = 1.2f;		// gain
const float a = 2.f;

// using angles in rad
float calc_sample(const float& _sample, const float& _sample_angle, const float& _speaker_angle) {
	return tanh(D * _sample) * exp(a * .5f * cos(_sample_angle - _speaker_angle) - .5f);
}

// TODO: low frequency missing, probably use a low pass filter on all samples and combine and increase amplitude and output on low frequency channel
void samples_7_1_surround(float* _buffer, const int& _size, const int& _offset, const float& _sample, const float& _angle, const float& _vol, const float& _lfe, const float& _delay_sample) {
	float s = (_sample + _delay_sample) *_vol;

	_buffer[_offset] += calc_sample(s, _angle, SOUND_7_1_ANGLES[0]);	// front-left
	_buffer[_offset + 1] += calc_sample(s, _angle, SOUND_7_1_ANGLES[1]);	// front-right
	_buffer[_offset + 2] += calc_sample(s, _angle, SOUND_7_1_ANGLES[2]);	// centre
	_buffer[_offset + 4] += calc_sample(s, _angle, SOUND_7_1_ANGLES[4]);	// rear-left
	_buffer[_offset + 5] += calc_sample(s, _angle, SOUND_7_1_ANGLES[5]);	// rear-right
	_buffer[_offset + 6] += calc_sample(s, _angle, SOUND_7_1_ANGLES[6]);	// centre-left
	_buffer[_offset + 7] += calc_sample(s, _angle, SOUND_7_1_ANGLES[7]);	// centre-right

	_buffer[_offset + 3] += s * _lfe;
}

void samples_5_1_surround(float* _buffer, const int& _size, const int& _offset, const float& _sample, const float& _angle, const float& _vol, const float& _lfe, const float& _delay_sample) {
	float s = (_sample + _delay_sample) * _vol;

	_buffer[_offset] += calc_sample(s, _angle, SOUND_5_1_ANGLES[0]);
	_buffer[_offset + 1] += calc_sample(s, _angle, SOUND_5_1_ANGLES[1]);
	_buffer[_offset + 2] += calc_sample(s, _angle, SOUND_5_1_ANGLES[2]);
	_buffer[_offset + 4] += calc_sample(s, _angle, SOUND_5_1_ANGLES[4]);
	_buffer[_offset + 5] += calc_sample(s, _angle, SOUND_5_1_ANGLES[5]);

	_buffer[_offset + 3] += s * _lfe;
}

void samples_stereo(float* _buffer, const int& _size, const int& _offset, const float& _sample, const float& _angle, const float& _vol, const float& _lfe, const float& _delay_sample) {
	float s = (_sample + _delay_sample) * _vol;

	_buffer[_offset] += calc_sample(s, _angle, SOUND_STEREO_ANGLES[0]);
	_buffer[_offset + 1] += calc_sample(s, _angle, SOUND_STEREO_ANGLES[1]);
}

void samples_mono(float* _buffer, const int& _size, const int& _offset, const float& _sample, const float& _angle, const float& _vol, const float& _lfe, const float& _delay_sample) {
	float s = (_sample + _delay_sample) * _vol;

	_buffer[_offset] = calc_sample(s, _angle, SOUND_STEREO_ANGLES[0]);
	_buffer[_offset + 1] += calc_sample(s, _angle, SOUND_STEREO_ANGLES[1]);
}

float reverb(float* _buffer, const int& _size, int& _cursor, const float& _decay, const float& _sample) {
	_buffer[_cursor] += _sample;
	_buffer[_cursor] *= _decay;
	++_cursor %= _size;
	return _buffer[_cursor];
}