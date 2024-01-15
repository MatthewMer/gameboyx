#include "AudioMgr.h"

#include "AudioOpenAL.h"

void audio_callback(void* userdata, uint8_t* stream, int len);

AudioMgr* AudioMgr::instance = nullptr;

AudioMgr* AudioMgr::getInstance(machine_information& _machine_info) {
	resetInstance();

	instance = new AudioOpenAL(_machine_info);

	return instance;
}

void AudioMgr::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}

void AudioMgr::InitSound() {
	alcDev = alcOpenDevice(nullptr);				// nullptr returns the default audio device
	alcCtx = alcCreateContext(alcDev, nullptr);

	alcMakeContextCurrent(alcCtx);
}

void audio_callback(void* userdata, u8* stream, int len) {

}