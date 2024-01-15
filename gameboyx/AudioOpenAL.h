#pragma once

#include "AudioMgr.h"

#include "data_containers.h"

class AudioOpenAL : AudioMgr {
public:
	friend class AudioMgr;

protected:
	AudioOpenAL(machine_information& _machine_info) : AudioMgr(_machine_info){}

private:
	void InitAudio() override;
};

