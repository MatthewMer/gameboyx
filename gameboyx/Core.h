#pragma once

#include "Cartridge.h"
#include "registers.h"

#include "Mmu.h"

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

	Mmu* mmu_instance;
	
private:
	static Core* instance;
	~Core() = default;
};


class CoreSM83 : protected Core
{
public:
	friend class Core;

private:
	// constructor
	explicit CoreSM83(const Cartridge& _cart_obj);
	// destructor
	~CoreSM83() = default;

	// members
	registers_gbc regs;
};