#pragma once

#include "Cartridge.h"
#include "registers.h"
#include "MmuBase.h"
#include "information_structs.h"
#include <chrono>
using namespace std::chrono;

class CoreBase
{
public:
	// get/reset instance
	static CoreBase* getInstance(machine_information& _machine_info);
	static void resetInstance();

	// clone/assign protection
	CoreBase(CoreBase const&) = delete;
	CoreBase(CoreBase&&) = delete;
	CoreBase& operator=(CoreBase const&) = delete;
	CoreBase& operator=(CoreBase&&) = delete;

	// public members
	virtual void RunCycles() = 0;
	virtual void RunCpu() = 0;

	virtual int GetDelayTime() = 0;
	virtual void GetCurrentHardwareState() const = 0;
	virtual void GetStartupHardwareInfo() const = 0;
	virtual void GetCurrentCoreFrequency() = 0;
	virtual bool CheckNextFrame() = 0;

	virtual void GetCurrentProgramCounter() const = 0;
	virtual void InitMessageBufferProgram() = 0;
	virtual void GetCurrentRegisterValues() const = 0;
	virtual void GetCurrentFlagsAndISR() const = 0;

protected:
	// constructor
	explicit CoreBase(machine_information& _machine_info) : machineInfo(_machine_info) {
		mmu_instance = MmuBase::getInstance(_machine_info);
	};

	~CoreBase() = default;

	MmuBase* mmu_instance;

	// members
	int machineCyclesPerFrame = 0;					// threshold
	int machineCycleCounterClock = 0;				// counter

	virtual void ExecuteInstruction() = 0;
	virtual void ExecuteInterrupts() = 0;

	virtual void InitCpu() = 0;
	virtual void InitRegisterStates() = 0;

	virtual void GetCurrentInstruction() const = 0;

	machine_information& machineInfo;

private:
	static CoreBase* instance;
};