#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	The BaseCPU class is just used to derive the different processor types (NES, GB, ..) of the emulated hardware from it.
*	The member methods are as generic as possible to fit every processor type without exposing the processor
*	specific internals.
*/

#include "HardwareMgr.h"

#include "GameboyCartridge.h"
#include "BaseMMU.h"
#include "BaseGPU.h"
#include "BaseAPU.h"
#include "BaseCartridge.h"
#include "defs.h"
#include "VHardwareTypes.h"
#include <map>

namespace Emulation {
	using assembly_table = std::vector<std::tuple<int, instr_entry>>;

	struct assembly_tables : std::vector<assembly_table> {
		std::string memory_type = "";

		void SetMemoryType(const std::string& _name) {
			memory_type = _name;
		}

		std::string GetMemoryType() {
			return memory_type;
		}

		assembly_tables(size_t _size) {
			this->assign(_size, {});
		}

		assembly_tables() = default;
	};

	class BaseCPU {
	public:
		// get/reset instance
		static std::shared_ptr<BaseCPU> s_GetInstance(std::shared_ptr<BaseCartridge> _cartridge);
		static std::shared_ptr<BaseCPU> s_GetInstance();
		static void s_ResetInstance();
		virtual void Init() = 0;

		// clone/assign protection
		BaseCPU(BaseCPU const&) = delete;
		BaseCPU(BaseCPU&&) = delete;
		BaseCPU& operator=(BaseCPU const&) = delete;
		BaseCPU& operator=(BaseCPU&&) = delete;

		// public members
		virtual void RunCycles() = 0;
		virtual void RunCycle() = 0;

		virtual void GetHardwareInfo(std::vector<data_entry>& _hardware_info) const = 0;
		virtual void GetInstrDebugFlags(std::vector<reg_entry>& _register_values, std::vector<reg_entry>& _flag_values, std::vector<reg_entry>& _misc_values) const = 0;
		virtual void UpdateDebugData(debug_data* _data) const = 0;

		virtual void GenerateAssemblyTables(std::shared_ptr<BaseCartridge> _cartridge) = 0;
		virtual void GenerateTemporaryAssemblyTable(assembly_tables& _table) = 0;

		virtual void Write8Bit(const u8& _data, const u16& _addr) = 0;
		virtual void Write16Bit(const u16& _data, const u16& _addr) = 0;
		virtual u8 Read8Bit(const u16& _addr) = 0;
		virtual u16 Read16Bit(const u16& _addr) = 0;

		virtual int GetPlayerCount() const = 0;

		int GetClockCycles() const;
		void ResetClockCycles();

		virtual void GetMemoryTypes(std::map<int, std::string>& _map) const = 0;

		assembly_tables& GetAssemblyTables();

	protected:
		// constructor
		explicit BaseCPU(std::shared_ptr<BaseCartridge> _cartridge) {};
		virtual ~BaseCPU() {}

		std::weak_ptr<BaseMMU> m_MmuInstance;
		std::weak_ptr<BaseGPU> m_GraphicsInstance;
		std::weak_ptr<BaseAPU> m_SoundInstance;

		int currentTicks = 0;
		int ticksPerFrame = 0;
		int tickCounter = 0;

		std::vector<callstack_data> callstack;

		virtual void ExecuteInstruction() = 0;
		virtual bool CheckInterrupts() = 0;

		virtual void InitRegisterStates() = 0;

		assembly_tables asmTables;

	private:
		static std::weak_ptr<BaseCPU> m_Instance;
	};
}