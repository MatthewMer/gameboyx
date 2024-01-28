#include "AudioMgr.h"

#include "AudioOpenAL.h"
#include "AudioSDL.h"

AudioMgr* AudioMgr::instance = nullptr;

AudioMgr* AudioMgr::getInstance(audio_information& _audio_info) {
	resetInstance();

	instance = new AudioSDL(_audio_info);

	return instance;
}

void AudioMgr::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}