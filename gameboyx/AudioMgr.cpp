#include "AudioMgr.h"

#include "AudioOpenAL.h"
#include "AudioSDL.h"

AudioMgr* AudioMgr::instance = nullptr;

AudioMgr* AudioMgr::getInstance() {
	if (instance == nullptr) {
		instance = new AudioSDL();
	}

	return instance;
}

void AudioMgr::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}