#pragma once

#include "MmuBase.h"
#include "Cartridge.h"
#include "defs.h"

class MmuSM83_MBC3 : protected Mmu
{
public:
	friend class Mmu;
	
	// members
	void Write8Bit(const u8& _data, const u16& _addr);
	void Write16Bit(const u16& _data, const u16& _addr);
	u8 Read8Bit(const u16& _addr);
	u16 Read16Bit(const u16& _addr);

private:
	// constructor
	explicit MmuSM83_MBC3(const Cartridge& _cart_obj);
	// destructor
	~MmuSM83_MBC3() = default;
};