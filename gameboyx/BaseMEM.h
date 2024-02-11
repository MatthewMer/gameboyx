#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	The BaseMEM class is just used to derive the different Memory types (NES, GB, ..) of the emulated hardware from it.
*	The member methods are as generic as possible. The child classes are specifically tied to the different MMU and CPU types
*	and expose the entire interals of the memory. This way CPU and MMU can interact with it much more efficiently.
*/

#include "BaseCartridge.h"

class BaseMEM
{
protected:
	// constructor
	explicit BaseMEM(machine_information& _machine_info) : machineInfo(_machine_info) {};

	// members
	virtual void InitMemory() = 0;
	virtual void InitMemoryState() = 0;
	virtual bool ReadRomHeaderInfo(const std::vector<u8>& _vec_rom) = 0;
	virtual bool CopyRom(const std::vector<u8>& _vec_rom) = 0;
	virtual void SetupMemoryDebugTables() = 0;
	virtual void FillMemoryDebugTable(TableSection<memory_data>& _table_section, std::vector<u8>* _bank_data, const int& _offset) = 0;

	virtual void AllocateMemory() = 0;

	virtual void RequestInterrupts(const u8& isr_flags) = 0;

	virtual std::vector<u8>* GetProgramData(const int& _bank) const = 0;

	machine_information& machineInfo;
};