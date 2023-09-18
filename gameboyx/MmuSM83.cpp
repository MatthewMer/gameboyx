#include "MmuSM83.h"

#include "MemorySM83.h"
#include "gameboy_defines.h"

#include "logger.h"

/* ***********************************************************************************************************
	MBC3 DEFINES
*********************************************************************************************************** */
// addresses
#define RAM_TIMER_ENABLE			0x0000
#define ROM_BANK_NUMBER_SELECT		0x2000

#define RAM_BANK_NUMBER_OR_RTC_REG	0x4000
#define LATCH_CLOCK_DATA			0x6000

// masks
#define ROM_BANK_MASK				0x7f
#define RAM_ENABLE_MASK				0x0F

// values
#define RAM_ENABLE					0x0A

/* ***********************************************************************************************************
	CONSTRUCTOR
*********************************************************************************************************** */
MmuSM83_MBC3::MmuSM83_MBC3(const Cartridge& _cart_obj){
	InitMmu(_cart_obj);

	MemorySM83::resetInstance();
	mem_instance = MemorySM83::getInstance(_cart_obj);
	machine_ctx = mem_instance->GetMachineContext();
}

/* ***********************************************************************************************************
	MEMORY ACCESS
*********************************************************************************************************** */
void MmuSM83_MBC3::Write8Bit(const u8& _data, const u16& _addr) {
	// ROM Bank 0 -> RAM/Timer enable and ROM Bank select
	if (_addr < ROM_BANK_N_OFFSET) {
		// RAM/TIMER enable
		if (_addr < ROM_BANK_NUMBER_SELECT) {
			timerRamEnable = (_data & RAM_ENABLE_MASK) == RAM_ENABLE;
		}
		// ROM Bank number
		else {
			int romBankNumber = _data & ROM_BANK_MASK;
			if (romBankNumber == 0) romBankNumber = 1;
			machine_ctx->romBank = romBankNumber;
		}
	}
	// ROM Bank 1-n -> RAM Bank select or RTC register select or Latch Clock Data
	else if (_addr < VRAM_N_OFFSET) {
		// RAM Bank number or RTC register select
		if (_addr < LATCH_CLOCK_DATA) {
			int ramBankRtcNumber = _data;
			machine_ctx->ramBank = ramBankRtcNumber;
		}
		else {
			if (_data == 0x01 && rtcRegistersLastWrite == 0x00) {
				LatchClock();
			}
			else {
				rtcRegistersLastWrite = _data;
			}
		}
	}
	// VRAM 0-n
	else if (_addr < RAM_BANK_N_OFFSET) {
		mem_instance->WriteVRAM_N(_data, _addr);
	}
	// RAM 0-n -> RTC Registers 08-0C
	else if (_addr < WRAM_0_OFFSET) {
		if (timerRamEnable) {
			int ramBankNumber = machine_ctx->ramBank;
			if (ramBankNumber < 0x04) {
				mem_instance->WriteRAM_N(_data, _addr);
			}
			else if (ramBankNumber > 0x07 && ramBankNumber < 0x0D) {
				WriteClock(_data);
			}
		}
	}
	// WRAM 0
	else if (_addr < WRAM_N_OFFSET) {
		mem_instance->WriteWRAM_0(_data, _addr);
	}
	// WRAM 1-n
	else if (_addr < MIRROR_WRAM_OFFSET) {
		mem_instance->WriteWRAM_N(_data, _addr);
	}
	// MIRROR WRAM, prohibited
	else if (_addr < OAM_OFFSET) {
		return;
	}
	// OAM
	else if (_addr < NOT_USED_MEMORY_OFFSET) {
		mem_instance->WriteOAM(_data, _addr);
	}
	// NOT USED
	else if (_addr < IO_REGISTERS_OFFSET) {
		return;
	}
	// IO REGISTERS
	else if (_addr < HRAM_OFFSET) {
		mem_instance->WriteIO(_data, _addr);
	}
	// HRAM (stack,...)
	else if (_addr < IE_OFFSET) {
		mem_instance->WriteHRAM(_data, _addr);
	}
	// IE register
	else {
		mem_instance->WriteIE(_data);
	}
}

// CGB is little endian: low byte first, high byte second
void MmuSM83_MBC3::Write16Bit(const u16& _data, const u16& _addr) {
	Write8Bit(_data & 0xFF, _addr);
	Write8Bit((_data & 0xFF00) >> 8, _addr + 1);
}

u8 MmuSM83_MBC3::Read8Bit(const u16& _addr) {
	// ROM Bank 0
	if (_addr < ROM_BANK_N_OFFSET) {
		return mem_instance->ReadROM_0(_addr);
	}
	// ROM Bank 1-n
	else if (_addr < VRAM_N_OFFSET) {
		return mem_instance->ReadROM_N(_addr);
	}
	// VRAM 0-n
	else if (_addr < RAM_BANK_N_OFFSET) {
		return mem_instance->ReadVRAM_N(_addr);
	}
	// RAM 0-n
	else if (_addr < WRAM_0_OFFSET) {
		if (timerRamEnable) {
			int ramBankNumber = machine_ctx->ramBank;
			if (ramBankNumber < 0x04) {
				return mem_instance->ReadRAM_N(_addr);
			}
			else if (ramBankNumber > 0x07 && ramBankNumber < 0x0D) {
				return ReadClock();
			}
		}
	}
	// WRAM 0
	else if (_addr < WRAM_N_OFFSET) {
		return mem_instance->ReadWRAM_0(_addr);
	}
	// WRAM 1-n
	else if (_addr < MIRROR_WRAM_OFFSET) {
		return mem_instance->ReadWRAM_N(_addr);
	}
	// MIRROR WRAM (prohibited)
	else if (_addr < OAM_OFFSET) {
		return mem_instance->ReadWRAM_0(_addr);
	}
	// OAM
	else if (_addr < NOT_USED_MEMORY_OFFSET) {
		return mem_instance->ReadOAM(_addr);
	}
	// NOT USED
	else if (_addr < IO_REGISTERS_OFFSET) {
		return 0x00;
	}
	// IO REGISTERS
	else if (_addr < HRAM_OFFSET) {
		return mem_instance->ReadIO(_addr);
	}
	// HRAM (stack,...)
	else if (_addr < IE_OFFSET) {
		return mem_instance->ReadHRAM(_addr);
	}
	// IE register
	else {
		return mem_instance->ReadIE();
	}
}

// CGB is little endian: low byte first, high byte second
u16 MmuSM83_MBC3::Read16Bit(const u16& _addr) {
	dataBuffer = ((u16)Read8Bit(_addr + 1) << 8);
	dataBuffer |= Read8Bit(_addr);
	return dataBuffer;
}

/* ***********************************************************************************************************
	RTC			TODO !!!!!
*********************************************************************************************************** */
void MmuSM83_MBC3::LatchClock() {

}

u8 MmuSM83_MBC3::ReadClock() {
	return 0x00;
}

void MmuSM83_MBC3::WriteClock(const u8& _data) {

}

/* ***********************************************************************************************************
	MACHINE STATE ACCESS
*********************************************************************************************************** */
// access machine states
int MmuSM83_MBC3::GetCurrentSpeed() const {
	return machine_ctx->currentSpeed;
}

u8 MmuSM83_MBC3::GetInterruptEnable() const {
	return machine_ctx->IE;
}

u8 MmuSM83_MBC3::GetInterruptRequests() const {
	return machine_ctx->IF;
}

void MmuSM83_MBC3::ResetInterruptRequest(const u8& _isr_flags) {
	machine_ctx->IF &= ~_isr_flags;
}

void MmuSM83_MBC3::ProcessMachineCyclesCurInstruction(const int& _machine_cycles) {
	machine_ctx->machineCyclesDIVCounter += _machine_cycles;
	if (machine_ctx->machineCyclesDIVCounter > machine_ctx->machineCyclesPerDIVIncrement) {
		machine_ctx->machineCyclesDIVCounter -= machine_ctx->machineCyclesPerDIVIncrement;

		if (machine_ctx->DIV == 0xFF) {
			machine_ctx->DIV = 0x00;
			// TODO: interrupt or whatever
		}
		else {
			machine_ctx->DIV++;
		}
	}

	if (machine_ctx->TAC & TAC_CLOCK_ENABLE) {
		machine_ctx->machineCyclesTIMACounter += _machine_cycles;
		if (machine_ctx->machineCyclesTIMACounter > machine_ctx->machineCyclesPerTIMAIncrement) {
			machine_ctx->machineCyclesTIMACounter -= machine_ctx->machineCyclesPerTIMAIncrement;
			
			if (machine_ctx->TIMA == 0xFF) {
				machine_ctx->TIMA = machine_ctx->TMA;
				// request interrupt
				machine_ctx->IF |= ISR_TIMER;
			}
			else {
				machine_ctx->TIMA++;
			}
		}
	}
}

/* ***********************************************************************************************************
	MMU INITIALIZATION
*********************************************************************************************************** */
void MmuSM83_MBC3::InitMmu(const Cartridge& _cart_obj) {
	if (!ReadRomHeaderInfo(_cart_obj.GetRomVector())) { return; }


}

bool MmuSM83_MBC3::ReadRomHeaderInfo(const std::vector<u8>& _vec_rom) {
	if (_vec_rom.size() < ROM_HEAD_ADDR + ROM_HEAD_SIZE) { return false; }

	return true;
}