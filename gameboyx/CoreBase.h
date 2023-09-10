#pragma once

#include "Cartridge.h"
#include "registers.h"
#include "MmuBase.h"

class Core
{
public:
	// get/reset instance
	static Core* getInstance(const Cartridge& _cart_obj);
	static void resetInstance();

	// clone/assign protection
	Core(Core const&) = delete;
	Core(Core&&) = delete;
	Core& operator=(Core const&) = delete;
	Core& operator=(Core&&) = delete;

protected:
	// constructor
	Core() = default;
	~Core() = default;

	Mmu* mmu_instance;
	
private:
	static Core* instance;

	// members
	virtual void InitCpu(const Cartridge& _cart_obj) = 0;
	virtual void InitRegisterStates() = 0;
};