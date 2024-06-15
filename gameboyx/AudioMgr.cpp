#include "AudioMgr.h"

#include "AudioOpenAL.h"
#include "AudioSDL.h"

#include <iostream>

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

void AudioMgr::SetSamplingRate(audio_settings& _audio_settings) {
	InitAudio(_audio_settings, true);
}

void AudioMgr::SetVolume(const float& _volume, const float& _lfe) {
	audioInfo.master_volume.store(_volume);
	audioInfo.lfe.store(_lfe);
	audioInfo.volume_changed.store(true);
}

void AudioMgr::SetReverb(const float& _delay, const float& _decay) {
	audioInfo.decay.store(_decay);
	audioInfo.delay.store(_delay);
	audioInfo.reload_reverb.store(true);
}