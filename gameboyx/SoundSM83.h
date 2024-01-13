#pragma once
#include "SoundBase.h"
#include "MemorySM83.h"

class SoundSM83 : protected SoundBase {
public:
	friend class SoundBase;

	// members
	void ProcessSound() override;

private:
	// constructor
	SoundSM83(machine_information& _machine_info) : machineInfo(_machine_info) {
		memInstance = MemorySM83::getInstance();
		soundCtx = memInstance->GetSoundContext();
		machineCtx = memInstance->GetMachineContext();
	}
	// destructor
	~SoundSM83() = default;

	// memory access
	MemorySM83* memInstance;
	sound_context* soundCtx;
	machine_context* machineCtx;
	machine_information& machineInfo;
};