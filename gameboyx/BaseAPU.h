#pragma once

#include "AudioMgr.h"

// apu internal data
struct apu_data {
	std::vector<float> buffer;
	int channels = 0;
	int cursor = 0;
};

class BaseAPU {
public:
	// get/reset instance
	static BaseAPU* getInstance(audio_information& _audio_info, AudioMgr* audioMgr);
	static BaseAPU* getInstance();
	static void resetInstance();

	// clone/assign protection
	BaseAPU(BaseAPU const&) = delete;
	BaseAPU(BaseAPU&&) = delete;
	BaseAPU& operator=(BaseAPU const&) = delete;
	BaseAPU& operator=(BaseAPU&&) = delete;

	// public members
	virtual void ProcessAPU(const int& _ticks) = 0;
	virtual void SampleApuData(const int& _samples_num, const int& _sampling_rate) = 0;

	// get access to apu data -> audio thread
	const apu_data& GetApuData();

protected:
	// constructor
	BaseAPU(audio_information& _audio_info, AudioMgr* _audio_mgr) : soundInfo(_audio_info), audioMgr(_audio_mgr) {}
	virtual ~BaseAPU() = default;

	audio_information& soundInfo;

	AudioMgr* audioMgr;

	apu_data apuData;

private:
	static BaseAPU* instance;
};