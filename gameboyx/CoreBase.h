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
	static CoreBase* getInstance(message_buffer& _msg_buffer);
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
	virtual void GetCurrentHardwareState(message_buffer& _msg_buffer) const = 0;
	virtual void GetStartupHardwareInfo(message_buffer& _msg_buffer) const = 0;
	virtual u32 GetPassedClockCycles() = 0;
	virtual int GetDisplayFrequency() const = 0;
	virtual bool CheckNextFrame() = 0;

protected:
	// constructor
	CoreBase(message_buffer& _msg_buffer) : msgBuffer(_msg_buffer) {
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

	message_buffer& msgBuffer;
	
private:
	static CoreBase* instance;
};