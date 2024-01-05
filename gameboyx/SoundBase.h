#pragma once
#include "information_structs.h"

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

private:
	static SoundBase* instance;
};