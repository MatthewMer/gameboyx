#include "AudioSDL.h"
#include "format"
#include "AudioMgr.h"
#include "BaseAPU.h"
#include "logger.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <glm.hpp>

using namespace std;

void audio_callback(void* userdata, u8* _device_buffer, int _length);
void audio_thread(audio_information* _audio_info, virtual_audio_information* _virt_audio_info, audio_samples* _samples);

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
	audioInfo.decay.store(_audio_settings.decay);
	audioInfo.delay.store(_audio_settings.delay);
	audioInfo.reload_reverb.store(false);

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

	memset(reg_1, 0, reg_1_size);
	memset(reg_2, 0, reg_2_size);

	samples->read_cursor = ((b_read_cursor + _length) % samples->buffer_size) / sizeof(float);

	samples->notifyBufferUpdate.notify_one();
}

struct delay_buffer {
	std::vector<std::vector<float>> buffer;

	delay_buffer() = delete;
	delay_buffer(const int& _sampling_rate, const int& _num_buffers) {
		buffer.assign(_num_buffers, {});
		for (auto& n : buffer) {
			n.assign((size_t)((M_DISTANCE_EARS / M_SPEED_OF_SOUND) * _sampling_rate), .0f);
		}
	}
};

struct reverb_buffer {
	std::vector<float> buffer;
	float decay = .0f;
	int cursor = 0;

	reverb_buffer() = delete;
	reverb_buffer(const int& _delay, const float& _decay) : decay(_decay) {
		if (_delay > 0) {
			buffer.assign(_delay, .0f);
		} else {
			buffer.assign(1, .0f);
		}
	}

	void add(const float& _sample) {
		buffer[cursor] += _sample;
	}

	float next() {
		buffer[cursor] *= decay;
		++cursor %= buffer.size();
		return buffer[cursor];
	}
};

struct speakers {
	typedef void (speakers::*speaker_function)(float*, const float&, const float&, const float&);
	speaker_function func;

	reverb_buffer r_buffer;
	delay_buffer d_buffer;

	float volume = .0f;
	float lfe = .0f;

	int sampling_rate;
	int channels;

	float* buffer;
	int buffer_size;

	std::vector<std::vector<complex>> fft_buffer;

	speakers() = delete;
	speakers(const int& _channels, const int& _sampling_rate, float* _buffer, const int& _buffer_size, const int& _delay, const float& _decay, const float& _volume, const float& _lfe) : r_buffer(_delay, _decay), d_buffer(_sampling_rate, _channels) {
		switch (_channels) {
		case SOUND_7_1:
			func = &speakers::samples_7_1_surround;
			break;
		case SOUND_5_1:
			func = &speakers::samples_5_1_surround;
			break;
		case SOUND_STEREO:
			func = &speakers::samples_stereo;
			break;
		case SOUND_MONO:
			func = &speakers::samples_mono;
			break;
		default:
			func = nullptr;
			LOG_ERROR("[SDL] speaker configuration currently not supported");
			break;
		}

		volume = _volume;
		lfe = _lfe;
		sampling_rate = _sampling_rate;
		channels = _channels;
		buffer = _buffer;
		buffer_size = _buffer_size;
		fft_buffer = std::vector<std::vector<complex>>();
	}

	void output(const int& _offset, const int& _sample_count, const std::vector<std::vector<complex>>& _samples, const std::vector<float>& _angles) {
		fft_buffer.assign(_samples.size(), {});
		for (int i = 0; i < _samples.size(); i++) {
			fft_buffer[i].assign(_samples[i].begin(), _samples[i].end());
		}

		int o = _offset;
		for (int i = 0; i < _sample_count; i++) {
			float reverb = r_buffer.next();

			for (int j = 0; j < (int)_angles.size(); j++) {
				float sample = _samples[j][i].real;
				(this->*func)(buffer + o, (sample + reverb) * volume, _angles[j], lfe);
				r_buffer.add(sample);
			}
			(o += channels) %= buffer_size;
		}
	}

	void reset_reverb(const int& _delay, const float& _decay) {
		r_buffer = reverb_buffer(_delay, _decay);
	}

	void set_volume(const float& _volume, const float& _lfe) {
		volume = _volume;
		lfe = _lfe;
	}

	const float x = .5f;
	const float D = 1.2f;		// gain
	const float a = 2.f;

	// using angles in rad
	float calc_sample(const float& _sample, const float& _sample_angle, const float& _speaker_angle) {
		return tanh(D * _sample) * exp(a * .5f * cos(_sample_angle - _speaker_angle) - .5f);
	}

	// TODO: low frequency missing, probably use a low pass filter on all samples and combine and increase amplitude for output on low frequency channel
	void samples_7_1_surround(float* _buffer, const float& _sample, const float& _angle, const float& _lfe) {
		_buffer[0] += calc_sample(_sample, _angle, SOUND_7_1_ANGLES[0]);	// front-left
		_buffer[1] += calc_sample(_sample, _angle, SOUND_7_1_ANGLES[1]);	// front-right
		_buffer[2] += calc_sample(_sample, _angle, SOUND_7_1_ANGLES[2]);	// centre
		_buffer[4] += calc_sample(_sample, _angle, SOUND_7_1_ANGLES[4]);	// rear-left
		_buffer[5] += calc_sample(_sample, _angle, SOUND_7_1_ANGLES[5]);	// rear-right
		_buffer[6] += calc_sample(_sample, _angle, SOUND_7_1_ANGLES[6]);	// centre-left
		_buffer[7] += calc_sample(_sample, _angle, SOUND_7_1_ANGLES[7]);	// centre-right

		_buffer[3] += _sample * _lfe;
	}

	void samples_5_1_surround(float* _buffer, const float& _sample, const float& _angle, const float& _lfe) {
		_buffer[0] += calc_sample(_sample, _angle, SOUND_5_1_ANGLES[0]);
		_buffer[1] += calc_sample(_sample, _angle, SOUND_5_1_ANGLES[1]);
		_buffer[2] += calc_sample(_sample, _angle, SOUND_5_1_ANGLES[2]);
		_buffer[4] += calc_sample(_sample, _angle, SOUND_5_1_ANGLES[4]);
		_buffer[5] += calc_sample(_sample, _angle, SOUND_5_1_ANGLES[5]);

		_buffer[3] += _sample * _lfe;
	}

	void samples_stereo(float* _buffer, const float& _sample, const float& _angle, const float& _lfe) {
		_buffer[0] += calc_sample(_sample, _angle, SOUND_STEREO_ANGLES[0]);
		_buffer[1] += calc_sample(_sample, _angle, SOUND_STEREO_ANGLES[1]);
	}

	void samples_mono(float* _buffer, const float& _sample, const float& _angle, const float& _lfe) {
		_buffer[0] += calc_sample(_sample, _angle, SOUND_STEREO_ANGLES[0]);
		_buffer[0] += calc_sample(_sample, _angle, SOUND_STEREO_ANGLES[1]);
	}
};

void audio_thread(audio_information* _audio_info, virtual_audio_information* _virt_audio_info, audio_samples* _samples) {
	SDL_AudioDeviceID* device = (SDL_AudioDeviceID*)_audio_info->device;
	BaseAPU* sound_instance = _virt_audio_info->sound_instance;

	const int channels = _audio_info->channels;
	const int sampling_rate = _audio_info->sampling_rate;

	const int virt_channels = _virt_audio_info->channels;

	speakers sp = speakers(
		channels,
		sampling_rate,
		_samples->buffer.data(),
		(int)_samples->buffer.size(),
		(int)(_audio_info->delay.load() * sampling_rate), 
		_audio_info->decay.load(), 
		_audio_info->master_volume.load(),
		_audio_info->lfe.load()
	);

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

	
	

	while (_virt_audio_info->audio_running.load()) {
		std::unique_lock<mutex> lock_buffer(_samples->mutBufferUpdate);
		_samples->notifyBufferUpdate.wait(lock_buffer);

		if (_audio_info->volume_changed.load()) {
			sp.set_volume(_audio_info->master_volume.load(), _audio_info->lfe.load());
			_audio_info->volume_changed.store(false);
		}

		if (_audio_info->reload_reverb.load()) {
			sp.reset_reverb((int)(_audio_info->delay.load() * sampling_rate), _audio_info->decay.load());
			_audio_info->reload_reverb.store(false);
		}

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
			int size = reg_1_samples + reg_2_samples;
			int buf_size = size;
			if ((size & (size - 1)) == 0) {
				int exp = (int)std::ceil(log2(size));
				buf_size = (int)pow(2, exp);
			}

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
			}*/

			sp.output(_samples->write_cursor, reg_1_samples + reg_2_samples, virt_samples, virt_angles);
			_samples->write_cursor = (_samples->write_cursor + reg_1_size + reg_2_size) % (int)_samples->buffer.size();
		}
		SDL_UnlockAudioDevice(*device);
	}
}