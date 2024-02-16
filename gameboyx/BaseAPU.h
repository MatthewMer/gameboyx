#pragma once

#include "AudioMgr.h"
#include "BaseCartridge.h"
#include "HardwareMgr.h"

class BaseAPU {
public:
	// get/reset instance
	static BaseAPU* getInstance(BaseCartridge* _cartridge);
	static BaseAPU* getInstance();
	static void resetInstance();

	// clone/assign protection
	BaseAPU(BaseAPU const&) = delete;
	BaseAPU(BaseAPU&&) = delete;
	BaseAPU& operator=(BaseAPU const&) = delete;
	BaseAPU& operator=(BaseAPU&&) = delete;

	// public members
	virtual void ProcessAPU(const int& _ticks) = 0;
	virtual void SampleAPU(float* _data, const int& _sampling_rate) = 0;

protected:
	// constructor
	BaseAPU() {}
	virtual ~BaseAPU() = default;

	std::vector<float> audioData;

private:
	static BaseAPU* instance;
};