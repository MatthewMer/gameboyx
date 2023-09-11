#pragma once

#include "Cartridge.h"
#include "registers.h"
#include "MmuBase.h"

class CoreBase
{
public:
	// get/reset instance
	static CoreBase* getInstance(const Cartridge& _cart_obj);
	static void resetInstance();

	// clone/assign protection
	CoreBase(CoreBase const&) = delete;
	CoreBase(CoreBase&&) = delete;
	CoreBase& operator=(CoreBase const&) = delete;
	CoreBase& operator=(CoreBase&&) = delete;

	// public members
	virtual void RunCycles() = 0;

protected:
	// constructor
	CoreBase() = default;
	~CoreBase() = default;

	MmuBase* mmu_instance;

	// members
	virtual void InitCpu(const Cartridge& _cart_obj) = 0;
	virtual void InitRegisterStates() = 0;
	
private:
	static CoreBase* instance;
};