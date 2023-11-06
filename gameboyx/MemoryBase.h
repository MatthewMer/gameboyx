#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	The MemoryBase class is just used to derive the different Memory types (NES, GB, ..) of the emulated hardware from it.
*	The member methods are as generic as possible. The child classes are specifically tied to the different MMU and CPU types
*	and expose the entire interals of the memory. This way CPU and MMU can interact with it much more efficiently.
*/

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

	virtual std::vector<std::pair<int, std::vector<u8>>> GetProgramData() const = 0;

	machine_information& machineInfo;
};