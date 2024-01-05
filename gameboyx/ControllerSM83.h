#pragma once
#include "ControllerBase.h"
#include "MemorySM83.h"

class ControllerSM83 : protected ControllerBase {
public:
	friend class ControllerBase;

	// members
	bool SetKey(const SDL_Keycode& _key) override;
	bool ResetKey(const SDL_Keycode& _key) override;

private:
	// constructor
	ControllerSM83(machine_information& _machine_info) : machineInfo(_machine_info) {
		memInstance = MemorySM83::getInstance();
		controlCtx = memInstance->GetControlContext();

		InitKeyMap();
	}
	// destructor
	~ControllerSM83() = default;

	void InitKeyMap() override;

	// memory access
	MemorySM83* memInstance;
	control_context* controlCtx;
	machine_information& machineInfo;
};