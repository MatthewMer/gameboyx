#pragma once
#include "BaseCTRL.h"
#include "GameboyMEM.h"

class GameboyCTRL : protected BaseCTRL {
public:
	friend class BaseCTRL;

	// members
	bool SetKey(const SDL_Keycode& _key) override;
	bool ResetKey(const SDL_Keycode& _key) override;

private:
	// constructor
	GameboyCTRL(machine_information& _machine_info) : machineInfo(_machine_info) {
		memInstance = GameboyMEM::getInstance();
		controlCtx = memInstance->GetControlContext();

		InitKeyMap();
	}
	// destructor
	~GameboyCTRL() = default;

	void InitKeyMap() override;

	// memory access
	GameboyMEM* memInstance;
	control_context* controlCtx;
	machine_information& machineInfo;
};