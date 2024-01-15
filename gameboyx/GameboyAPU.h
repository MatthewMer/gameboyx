#pragma once
#include "BaseAPU.h"

class GameboyAPU : protected BaseAPU {
public:
	friend class BaseAPU;

	// members
	void ProcessAPU(const int& _ticks) override;

private:
	// constructor
	GameboyAPU(audio_information& _audio_info, AudioMgr* _audio_mgr) : BaseAPU(_audio_info, _audio_mgr) {}
	// destructor
	~GameboyAPU() = default;
};