#pragma once
#include "BaseAPU.h"

class GameboyAPU : protected BaseAPU {
public:
	friend class BaseAPU;

	// members
	void ProcessSound() override;

private:
	// constructor
	GameboyAPU(machine_information& _machine_info) : BaseAPU(_machine_info) {}
	// destructor
	~GameboyAPU() = default;
};