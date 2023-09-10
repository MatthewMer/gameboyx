#pragma once

#include "MmuBase.h"
#include "Cartridge.h"
#include "defs.h"

class MmuSM83_MBC3 : protected Mmu
{
public:
	friend class Mmu;
	
	// members
	void Write8Bit(const u8& _data, const u16& _addr) override;
	void Write16Bit(const u16& _data, const u16& _addr) override;
	u8 Read8Bit(const u16& _addr) override;
	u16 Read16Bit(const u16& _addr) override;

private:
	// constructor
	explicit MmuSM83_MBC3(const Cartridge& _cart_obj);
	// destructor
	~MmuSM83_MBC3() = default;

	// members
	void InitMmu(const Cartridge& _cart_obj) override;
	bool ReadRomHeaderInfo(const std::vector<u8>& _vec_rom);

	bool isCgb = false;
	u16 dataBuffer;
};