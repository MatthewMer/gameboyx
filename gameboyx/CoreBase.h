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
	virtual void GetCurrentHardwareState(machine_information& _machine_info) const = 0;
	virtual void GetStartupHardwareInfo(machine_information& _machine_info) const = 0;
	virtual float GetCurrentCoreFrequency() = 0;
	virtual bool CheckNextFrame() = 0;

	virtual void GetCurrentMemoryLocation(machine_information& _machine_info) const = 0;
	virtual void InitMessageBufferProgram(std::vector<std::vector<std::tuple<int, int, std::string, std::string>>>& _program_buffer) = 0;
	virtual void GetCurrentRegisterValues(std::vector<std::pair<std::string, std::string>>& _register_values) const = 0;

protected:
	// constructor
	CoreBase(machine_information& _machine_info) : machineInfo(_machine_info) {
		mmu_instance = MmuBase::getInstance();
	};

	~CoreBase() = default;

	MmuBase* mmu_instance;

	// members
	int machineCyclesPerFrame = 0;					// threshold
	int machineCycleCounterClock = 0;				// counter

	virtual void ExecuteInstruction() = 0;
	virtual void ExecuteInterrupts() = 0;
	virtual void ExecuteMachineCycles() = 0;
	
	virtual void InitCpu(const Cartridge& _cart_obj) = 0;
	virtual void InitRegisterStates() = 0;

	machine_information& machineInfo;
	
private:
	static CoreBase* instance;
};