#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	The BaseCPU class is just used to derive the different processor types (NES, GB, ..) of the emulated hardware from it.
*	The member methods are as generic as possible to fit every processor type without exposing the processor
*	specific internals.
*/

#include "GameboyCartridge.h"
#include "BaseMMU.h"
#include "BaseGPU.h"
#include "BaseAPU.h"
#include "data_containers.h"
#include <chrono>
using namespace std::chrono;

class BaseCPU
{
public:
	// get/reset instance
	static BaseCPU* getInstance(machine_information& _machine_info);
	static void resetInstance();

	// clone/assign protection
	BaseCPU(BaseCPU const&) = delete;
	BaseCPU(BaseCPU&&) = delete;
	BaseCPU& operator=(BaseCPU const&) = delete;
	BaseCPU& operator=(BaseCPU&&) = delete;

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

	virtual void SetHardwareInstances() = 0;

protected:
	// constructor
	explicit BaseCPU(machine_information& _machine_info) : machineInfo(_machine_info) {
		mmu_instance = BaseMMU::getInstance(_machine_info);
	};

	~BaseCPU() = default;

	BaseMMU* mmu_instance;
	BaseGPU* graphics_instance;
	BaseAPU* sound_instance;

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
	static BaseCPU* instance;
};