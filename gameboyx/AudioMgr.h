#pragma once
#include "data_containers.h"
#include "al.h"
#include "alc.h"

class AudioMgr {
public:
	// get/reset instance
	static AudioMgr* getInstance(machine_information& _machine_info);
	static void resetInstance();

	// clone/assign protection
	AudioMgr(AudioMgr const&) = delete;
	AudioMgr(AudioMgr&&) = delete;
	AudioMgr& operator=(AudioMgr const&) = delete;
	AudioMgr& operator=(AudioMgr&&) = delete;

	// public members
	virtual void ProcessSound() = 0;

protected:
	// constructor
	AudioMgr(machine_information& _machine_info) : machineInfo(_machine_info) {}
	~AudioMgr() = default;

	virtual void InitSound() = 0;

	machine_information& machineInfo;

	ALCdevice* alcDev;
	ALCcontext* alcCtx;
	std::vector<ALshort> data;
	ALuint buffer, source;

private:
	static AudioMgr* instance;
};