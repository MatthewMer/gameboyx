#pragma once
#include "data_containers.h"

class BaseAPU {
public:
	// get/reset instance
	static BaseAPU* getInstance(machine_information& _machine_info);
	static void resetInstance();

	// clone/assign protection
	BaseAPU(BaseAPU const&) = delete;
	BaseAPU(BaseAPU&&) = delete;
	BaseAPU& operator=(BaseAPU const&) = delete;
	BaseAPU& operator=(BaseAPU&&) = delete;

	// public members
	virtual void ProcessSound() = 0;

protected:
	// constructor
	BaseAPU(machine_information& _machine_info) : machineInfo(_machine_info) {}
	~BaseAPU() = default;

	machine_information& machineInfo;

	/*
	ALCdevice* alcDev;
	ALCcontext* alcCtx;
	std::vector<ALshort> data;
	ALuint buffer, source;
	*/

private:
	static BaseAPU* instance;
};