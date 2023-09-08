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

	// members
	virtual void Write8Bit(const u8& _data, const u16& _addr) = 0;
	virtual void Write16Bit(const u16& _data, const u16& _addr) = 0;
	virtual u8 Read8Bit(const u16& _addr) = 0;
	virtual u16 Read16Bit(const u16& _addr) = 0;

protected:
	// constructor
	Mmu() = default;

	Memory* mem_instance;

private:
	static Mmu* instance;
	~Mmu() = default;
};
