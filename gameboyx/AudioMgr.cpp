#include "AudioMgr.h"

#include "AudioOpenAL.h"

void audio_callback(void* userdata, uint8_t* stream, int len);

AudioMgr* AudioMgr::instance = nullptr;

AudioMgr* AudioMgr::getInstance(audio_information& _audio_info) {
	resetInstance();

	instance = new AudioOpenAL(_audio_info);

	return instance;
}

void AudioMgr::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}

void audio_callback(void* userdata, u8* stream, int len) {

}