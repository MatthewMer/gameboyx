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
#include "GuiTable.h"
#include "VHardwareTypes.h"
#include <map>

namespace Emulation {
	class BaseCPU {
	public:
		// get/reset instance
		static BaseCPU* getInstance(BaseCartridge* _cartridge);
		static void resetInstance();

		static BaseCPU* getInstance();

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

		virtual void GetInstrDebugTable(GUI::GuiTable::Table<instr_entry>& _table) = 0;
		virtual void GetInstrDebugTableTmp(GUI::GuiTable::Table<instr_entry>& _table) = 0;

		virtual void Write8Bit(const u8& _data, const u16& _addr) = 0;
		virtual void Write16Bit(const u16& _data, const u16& _addr) = 0;
		virtual u8 Read8Bit(const u16& _addr) = 0;
		virtual u16 Read16Bit(const u16& _addr) = 0;

		virtual void SetInstances() = 0;

		virtual int GetPlayerCount() const = 0;

		int GetClockCycles() const;
		void ResetClockCycles();

		virtual void GetMemoryTypes(std::map<int, std::string>& _map) const = 0;

	protected:
		// constructor
		explicit BaseCPU(BaseCartridge* _cartridge) {
			mmu_instance = BaseMMU::getInstance(_cartridge);
		};
		virtual ~BaseCPU() {}

		BaseMMU* mmu_instance = nullptr;
		BaseGPU* graphics_instance = nullptr;
		BaseAPU* sound_instance = nullptr;

		int currentTicks = 0;
		int ticksPerFrame = 0;
		int tickCounter = 0;

		std::vector<callstack_data> callstack;

		virtual void ExecuteInstruction() = 0;
		virtual bool CheckInterrupts() = 0;

		virtual void InitRegisterStates() = 0;

	private:
		static BaseCPU* instance;
	};
}