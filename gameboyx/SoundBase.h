#pragma once
#include "information_structs.h"
#include <SDL_audio.h>

class SoundBase {
public:
	// get/reset instance
	static SoundBase* getInstance(machine_information& _machine_info);
	static void resetInstance();

	// clone/assign protection
	SoundBase(SoundBase const&) = delete;
	SoundBase(SoundBase&&) = delete;
	SoundBase& operator=(SoundBase const&) = delete;
	SoundBase& operator=(SoundBase&&) = delete;

	// public members
	virtual void ProcessSound() = 0;

protected:
	// constructor
	SoundBase() = default;
	~SoundBase() = default;

	// sdl audio
	SDL_AudioSpec specWant;
	SDL_AudioSpec specHave;
	SDL_AudioDeviceID device;

	void InitSound();

private:
	static SoundBase* instance;
};