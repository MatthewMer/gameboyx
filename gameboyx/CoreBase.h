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
	static CoreBase* getInstance(const Cartridge& _cart_obj, message_buffer& _msg_fifo);
	static void resetInstance();

	// clone/assign protection
	CoreBase(CoreBase const&) = delete;
	CoreBase(CoreBase&&) = delete;
	CoreBase& operator=(CoreBase const&) = delete;
	CoreBase& operator=(CoreBase&&) = delete;

	// public members
	virtual void RunCycles() = 0;
	virtual void RunCpu() = 0;

	virtual void ExecuteInstruction() = 0;
	virtual void ExecuteMachineCycles() = 0;
	virtual void ExecuteInterrupts() = 0;

	virtual int GetDelayTime() = 0;
	//virtual void ResetMachineCycleCounter() = 0;
	virtual bool CheckMachineCycles() const = 0;

protected:
	// constructor
	CoreBase(const Cartridge& _cart_obj, message_buffer& _msg_fifo) : msgBuffer(_msg_fifo) {
		mmu_instance = MmuBase::getInstance(_cart_obj);
	};

	~CoreBase() = default;

	MmuBase* mmu_instance;

	// members
	int machineCyclesPerFrame = 0;
	
	virtual void InitCpu(const Cartridge& _cart_obj) = 0;
	virtual void InitRegisterStates() = 0;

	message_buffer& msgBuffer;
	
private:
	static CoreBase* instance;
};