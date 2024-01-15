#pragma once
#include "data_containers.h"

#include "AudioMgr.h"

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

protected:
	// constructor
	BaseAPU(audio_information& _audio_info, AudioMgr* _audio_mgr) : soundInfo(_audio_info), audioMgr(_audio_mgr) {}
	~BaseAPU() = default;

	audio_information& soundInfo;

	AudioMgr* audioMgr;

private:
	static BaseAPU* instance;
};