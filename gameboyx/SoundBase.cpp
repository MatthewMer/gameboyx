#include "SoundBase.h"

#include "SoundSM83.h"

#include "logger.h"

void audio_callback(void* userdata, uint8_t* stream, int len);

SoundBase* SoundBase::instance = nullptr;

SoundBase* SoundBase::getInstance(machine_information& _machine_info) {
	resetInstance();

	instance = new SoundSM83(_machine_info);

	return instance;
}

void SoundBase::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}

void SoundBase::InitSound() {
	SDL_zero(specHave);
	SDL_zero(specWant);
	specWant.freq = 48000;					// 48kHz -> DVD quality
	specWant.format = AUDIO_F32;
	specWant.channels = 2;					// 2 audio channels -> stereo
	specWant.samples = 4096;
	specWant.callback = audio_callback;		// callback that will read samples from the buffer (ringbuffer probably?) goes here

	device = SDL_OpenAudioDevice(nullptr, 0, &specWant, &specHave, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
}

void audio_callback(void* userdata, u8* stream, int len) {
	
}