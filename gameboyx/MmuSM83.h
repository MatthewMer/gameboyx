#pragma once

#include "MmuBase.h"
#include "Cartridge.h"
#include "defs.h"

#include "MemorySM83.h"

class MmuSM83_MBC3 : protected MmuBase
{
public:
	friend class MmuBase;
	
	// members
	void Write8Bit(const u8& _data, const u16& _addr) override;
	void Write16Bit(const u16& _data, const u16& _addr) override;
	u8 Read8Bit(const u16& _addr) override;
	u16 Read16Bit(const u16& _addr) override;

	// access machine states
	int& GetCurrentSpeed() const;
	u8& GetInterruptEnable() const;
	u8& GetInterruptRequests() const;

private:
	// constructor
	explicit MmuSM83_MBC3(const Cartridge& _cart_obj);
	// destructor
	~MmuSM83_MBC3() = default;

	void ResetChildMemoryInstances() { mem_instance->resetInstance(); }
	MemorySM83* mem_instance;

	// members
	void InitMmu(const Cartridge& _cart_obj) override;
	bool ReadRomHeaderInfo(const std::vector<u8>& _vec_rom);

	// hardware info
	machine_state_context* machine_ctx;

	// mbc3 control
	bool timerRamEnable = false;
	u8 rtcRegistersLastWrite = 0x00;

	void LatchClock();
	u8 ReadClock();
	void WriteClock(const u8& _data);

	u16 dataBuffer;
};