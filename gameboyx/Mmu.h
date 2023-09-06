#pragma once

#include "Cartridge.h"
#include "Memory.h"

class Mmu
{
public:
	// get/reset instance
	static Mmu* getInstance(const Cartridge& _cart_obj);
	static void resetInstance();

	// clone/assign protection
	Mmu(Mmu const&) = delete;
	Mmu(Mmu&&) = delete;
	Mmu& operator=(Mmu const&) = delete;
	Mmu& operator=(Mmu&&) = delete;

protected:
	// constructor
	Mmu() = default;

	Memory* mem_instance;

private:
	static Mmu* instance;
	~Mmu() = default;
};


class MmuSM83 : protected Mmu
{
public:
	friend class Mmu;

private:
	// constructor
	explicit MmuSM83(const Cartridge& _cart_obj);
	// destructor
	~MmuSM83() = default;
};