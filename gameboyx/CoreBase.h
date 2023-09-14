#pragma once

#include "Cartridge.h"
#include "registers.h"
#include "MmuBase.h"
#include "message_fifo.h"
#include <chrono>
using namespace std::chrono;

class CoreBase
{
public:
	// get/reset instance
	static CoreBase* getInstance(const Cartridge& _cart_obj, const message_fifo& _msg_fifo);
	static void resetInstance();

	// clone/assign protection
	CoreBase(CoreBase const&) = delete;
	CoreBase(CoreBase&&) = delete;
	CoreBase& operator=(CoreBase const&) = delete;
	CoreBase& operator=(CoreBase&&) = delete;

	// public members
	virtual void RunCycles() = 0;
	virtual int GetDelayTime() = 0;

protected:
	// constructor
	CoreBase(const Cartridge& _cart_obj, const message_fifo& _msg_fifo) : msgFifo(_msg_fifo) {
		MmuBase::resetInstance();
		mmu_instance = MmuBase::getInstance(_cart_obj);
	};

	~CoreBase() = default;

	MmuBase* mmu_instance;

	// members
	int machineCyclesPerFrame = 0;
	
	virtual void InitCpu(const Cartridge& _cart_obj) = 0;
	virtual void InitRegisterStates() = 0;

	const message_fifo& msgFifo;
	
private:
	static CoreBase* instance;
};