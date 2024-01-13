#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	The CoreBase class is just used to derive the different processor types (NES, GB, ..) of the emulated hardware from it.
*	The member methods are as generic as possible to fit every processor type without exposing the processor
*	specific internals.
*/

#include "Cartridge.h"
#include "MmuBase.h"
#include "GraphicsUnitBase.h"
#include "information_structs.h"
#include <chrono>
using namespace std::chrono;

class CoreBase
{
public:
	// get/reset instance
	static CoreBase* getInstance(machine_information& _machine_info, graphics_information& _graphics_info, VulkanMgr* _graphics_mgr);
	static void resetInstance();

	// clone/assign protection
	CoreBase(CoreBase const&) = delete;
	CoreBase(CoreBase&&) = delete;
	CoreBase& operator=(CoreBase const&) = delete;
	CoreBase& operator=(CoreBase&&) = delete;

	// public members
	virtual void RunCycles() = 0;
	virtual void RunCycle() = 0;

	virtual void GetCurrentHardwareState() const = 0;
	virtual u32 GetCurrentClockCycles() = 0;

	virtual void GetCurrentProgramCounter() = 0;
	virtual void InitMessageBufferProgram() = 0;
	virtual void InitMessageBufferProgramTmp() = 0;
	virtual void GetCurrentRegisterValues() const = 0;
	virtual void GetCurrentMiscValues() const = 0;
	virtual void GetCurrentFlagsAndISR() const = 0;

	virtual void Write8Bit(const u8& _data, const u16& _addr) = 0;
	virtual void Write16Bit(const u16& _data, const u16& _addr) = 0;
	virtual u8 Read8Bit(const u16& _addr) = 0;
	virtual u16 Read16Bit(const u16& _addr) = 0;

protected:
	// constructor
	explicit CoreBase(machine_information& _machine_info, graphics_information& _graphics_info, VulkanMgr* _graphics_mgr) : machineInfo(_machine_info) {
		mmu_instance = MmuBase::getInstance(_machine_info);
		graphics_instance = GraphicsUnitBase::getInstance(_graphics_info, _graphics_mgr);
	};

	~CoreBase() = default;

	MmuBase* mmu_instance;
	GraphicsUnitBase* graphics_instance;

	// members
	int machineCycleClockCounter = 0;				// counter

	int currentTicks = 0;
	int ticksPerFrame = 0;
	u32 tickCounter = 0;

	virtual void ExecuteInstruction() = 0;
	virtual bool CheckInterrupts() = 0;

	virtual void InitRegisterStates() = 0;

	machine_information& machineInfo;

private:
	static CoreBase* instance;
};