#pragma once

#include "Cartridge.h"
#include "MemoryBase.h"
#include "information_structs.h"

class MmuBase
{
public:
	// get/reset instance
	static MmuBase* getInstance(machine_information& _machine_info);
	static void resetInstance();

	// clone/assign protection
	MmuBase(MmuBase const&) = delete;
	MmuBase(MmuBase&&) = delete;
	MmuBase& operator=(MmuBase const&) = delete;
	MmuBase& operator=(MmuBase&&) = delete;

	// members
	virtual void Write8Bit(const u8& _data, const u16& _addr) = 0;
	virtual void Write16Bit(const u16& _data, const u16& _addr) = 0;
	virtual u8 Read8Bit(const u16& _addr) = 0;
	virtual u16 Read16Bit(const u16& _addr) = 0;

	virtual void ResetChildMemoryInstances() = 0;

protected:
	// constructor
	explicit MmuBase(machine_information& _machine_info) : machineInfo(_machine_info) {};
	~MmuBase() = default;

private:
	static MmuBase* instance;
	static MmuBase* getNewMmuInstance(machine_information& _machine_info);

	machine_information& machineInfo;
};
