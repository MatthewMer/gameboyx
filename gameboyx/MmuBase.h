#pragma once

#include "Cartridge.h"
#include "MemoryBase.h"

class MmuBase
{
public:
	// get/reset instance
	static MmuBase* getInstance(const Cartridge& _cart_obj);
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
	MmuBase() = default;
	~MmuBase() = default;

private:
	static MmuBase* instance;

	// members
	virtual void InitMmu(const Cartridge& _cart_obj) = 0;
};
