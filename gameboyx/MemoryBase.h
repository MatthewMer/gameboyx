#pragma once

#include "Cartridge.h"

class MemoryBase
{
protected:
	// constructor
	MemoryBase() = default;

	// members
	virtual void InitMemory(const Cartridge& _cart_obj) = 0;
	virtual bool ReadRomHeaderInfo(const std::vector<u8>& _vec_rom) = 0;
	virtual bool CopyRom(const std::vector<u8>& _vec_rom) = 0;

	virtual void AllocateMemory() = 0;
	virtual void CleanupMemory() = 0;

	virtual void RequestInterrupts(const u8& isr_flags) = 0;
};