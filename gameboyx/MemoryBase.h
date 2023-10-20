#pragma once

#include "Cartridge.h"
#include "information_structs.h"

class MemoryBase
{
protected:
	// constructor
	explicit MemoryBase(machine_information& _machine_info) : machineInfo(_machine_info) {};

	// members
	virtual void InitMemory() = 0;
	virtual void InitMemoryState() = 0;
	virtual bool ReadRomHeaderInfo(const std::vector<u8>& _vec_rom) = 0;
	virtual bool CopyRom(const std::vector<u8>& _vec_rom) = 0;
	virtual void SetupDebugMemoryAccess() = 0;

	virtual void AllocateMemory() = 0;

	virtual void RequestInterrupts(const u8& isr_flags) = 0;

	machine_information& machineInfo;
};