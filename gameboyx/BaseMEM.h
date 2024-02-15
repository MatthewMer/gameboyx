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
#include "GuiTable.h"
#include "defs.h"

class BaseMEM
{
public:
	// get/reset instance
	static BaseMEM* getInstance(BaseCartridge* _cartridge);
	static BaseMEM* getInstance();
	static void resetInstance();

	virtual void GetMemoryDebugTables(std::vector<Table<memory_entry>>& _tables) = 0;
	
protected:
	// constructor
	BaseMEM() = default;
	virtual ~BaseMEM() {}

	// members
	virtual void InitMemory(BaseCartridge* _cartridge) = 0;
	virtual void InitMemoryState() = 0;
	virtual bool ReadRomHeaderInfo(const std::vector<u8>& _vec_rom) = 0;
	virtual bool CopyRom(const std::vector<u8>& _vec_rom) = 0;
	virtual void FillMemoryDebugTable(TableSection<memory_entry>& _table_section, std::vector<u8>* _bank_data, const int& _offset) = 0;

	virtual void AllocateMemory() = 0;

	virtual void RequestInterrupts(const u8& isr_flags) = 0;

	virtual std::vector<u8>* GetProgramData(const int& _bank) const = 0;

	static BaseMEM* instance;
};