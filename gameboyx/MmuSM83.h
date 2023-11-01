#pragma once

#include "MmuBase.h"
#include "Cartridge.h"
#include "defs.h"

#include "MemorySM83.h"

/* ***********************************************************************************************************
*
*		ROM only
*
*********************************************************************************************************** */
class MmuSM83_ROM : protected MmuBase
{
public:
	friend class MmuBase;

	// members
	void Write8Bit(const u8& _data, const u16& _addr) override;
	void Write16Bit(const u16& _data, const u16& _addr) override;
	u8 Read8Bit(const u16& _addr) override;
	//u16 Read16Bit(const u16& _addr) override;

	// access machine states
	//int GetCurrentSpeed() const override;
	//u8 GetInterruptEnable() const override;
	//u8 GetInterruptRequests() const override;
	//void ResetInterruptRequest(const u8& _isr_flags) override;

private:
	// constructor
	explicit MmuSM83_ROM(machine_information& _machine_info);
	// destructor
	~MmuSM83_ROM() = default;

	void ResetChildMemoryInstances() override { MemorySM83::resetInstance(); }
	MemorySM83* mem_instance;

	// hardware info and access
	machine_state_context* machine_ctx;

	// RAM control
	bool ramPresent = false;
};

/* ***********************************************************************************************************
*
*		MBC1
*
*********************************************************************************************************** */
class MmuSM83_MBC1 : protected MmuBase
{
public:
	friend class MmuBase;

	// members
	void Write8Bit(const u8& _data, const u16& _addr) override;
	void Write16Bit(const u16& _data, const u16& _addr) override;
	u8 Read8Bit(const u16& _addr) override;
	//u16 Read16Bit(const u16& _addr) override;

	// access machine states
	//int GetCurrentSpeed() const override;
	//u8 GetInterruptEnable() const override;
	//u8 GetInterruptRequests() const override;
	//void ResetInterruptRequest(const u8& _isr_flags) override;

private:
	// constructor
	explicit MmuSM83_MBC1(machine_information& _machine_info);
	// destructor
	~MmuSM83_MBC1() = default;

	void ResetChildMemoryInstances() override { MemorySM83::resetInstance(); }
	MemorySM83* mem_instance;

	// hardware info and access
	machine_state_context* machine_ctx;

	// mbc1 control
	bool ramEnable = false;
	int ramBankMask = 0x00;
	bool ramPresent = false;

	bool advancedBankingMode = false;
	u8 advancedBankingValue = 0x00;
};

/* ***********************************************************************************************************
*
*		MBC3
*
*********************************************************************************************************** */
class MmuSM83_MBC3 : protected MmuBase
{
public:
	friend class MmuBase;

	// members
	void Write8Bit(const u8& _data, const u16& _addr) override;
	void Write16Bit(const u16& _data, const u16& _addr) override;
	u8 Read8Bit(const u16& _addr) override;
	//u16 Read16Bit(const u16& _addr) override;

	// access machine states
	//int GetCurrentSpeed() const override;
	//u8 GetInterruptEnable() const override;
	//u8 GetInterruptRequests() const override;
	//void ResetInterruptRequest(const u8& _isr_flags) override;

private:
	// constructor
	explicit MmuSM83_MBC3(machine_information& _machine_info);
	// destructor
	~MmuSM83_MBC3() = default;

	void ResetChildMemoryInstances() override { MemorySM83::resetInstance(); }
	MemorySM83* mem_instance;

	// hardware info and access
	machine_state_context* machine_ctx;

	// mbc3 control
	bool timerRamEnable = false;
	u8 rtcRegistersLastWrite = 0x00;

	void LatchClock();
	u8 ReadClock();
	void WriteClock(const u8& _data);
};