#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	The BaseMEM class is just used to derive the different Memory types (NES, GB, ..) of the emulated hardware from it.
*	The member methods are as generic as possible. The child classes are specifically tied to the different MMU and CPU types
*	and expose the entire interals of the memory. This way CPU and MMU can interact with it much more efficiently.
*/

#include "HardwareMgr.h"

#include "BaseCartridge.h"
#include "defs.h"
#include "VHardwareTypes.h"

namespace Emulation {

	using memory_type_table = std::vector<std::tuple<int, memory_entry>>;

	struct memory_type_tables : std::vector<memory_type_table> {
		std::string memory_type = "";

		void SetMemoryType(const std::string& _name) {
			memory_type = _name;
		}

		std::string GetMemoryType() {
			return memory_type;
		}
	};

	class BaseMEM {
	public:
		// get/reset instance
		static BaseMEM* getInstance(BaseCartridge* _cartridge);
		static BaseMEM* getInstance();
		static void resetInstance();

		std::vector<memory_type_tables>& GetMemoryTables();

	protected:
		// constructor
		BaseMEM() = default;
		virtual ~BaseMEM() {}

		// members
		virtual void InitMemory(BaseCartridge* _cartridge) = 0;
		virtual void InitMemoryState() = 0;
		virtual bool ReadRomHeaderInfo(const std::vector<u8>& _vec_rom) = 0;
		virtual bool InitRom(const std::vector<u8>& _vec_rom) = 0;
		virtual bool InitBootRom(const std::vector<u8>& _boot_rom, const std::vector<u8>& _vec_rom) = 0;

		virtual void GenerateMemoryTables() = 0;
		virtual void FillMemoryTable(std::vector<std::tuple<int, memory_entry>>& _table_section, u8* _bank_data, const int& _offset, const size_t& _size) = 0;		

		virtual void AllocateMemory(BaseCartridge* _cartridge) = 0;

		virtual void RequestInterrupts(const u8& isr_flags) = 0;

		static BaseMEM* instance;

		std::vector<u8> romData;

		std::vector<memory_type_tables> memoryTables;
	};
}