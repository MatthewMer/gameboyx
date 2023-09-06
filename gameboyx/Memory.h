#pragma once

#include "Cartridge.h"
#include "Memory.h"

class Memory
{
public:
	// get/reset instance
	static Memory* getInstance(const Cartridge& _cart_obj);
	static void resetInstance();

	// clone/assign protection
	Memory(Memory const&) = delete;
	Memory(Memory&&) = delete;
	Memory& operator=(Memory const&) = delete;
	Memory& operator=(Memory&&) = delete;

private:
	// constructor
	explicit constexpr Memory(const Cartridge& _cart_obj);
	static Memory* instance;
	// destructor
	~Memory() = default;
};