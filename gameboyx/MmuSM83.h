#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	The MmuSM83 class emulating the different Gameboy (Color) memory mappers (directly built into the cartridge).
*	Therefor every cartridge can have a different size of ROM and RAM (or no RAM) and a optional built in RTC which
*	lets tick the clock when gameboy is turned off and battery buffered RAM for storing save states.
*	This class is purely there to emulate these memory mappings and extensions.
*/

#include "MmuBase.h"
#include "Cartridge.h"
#include "defs.h"

#include "MemorySM83.h"

class MmuSM83 : protected MmuBase {
public:
	friend class MmuBase;

	// members
	void Write8Bit(const u8& _data, const u16& _addr) override {};
	void Write16Bit(const u16& _data, const u16& _addr) override {};
	u8 Read8Bit(const u16& _addr) override { return 0xFF; };

	static MmuSM83* getInstance(machine_information& _machine_info, const std::vector<u8>& _vec_rom);

protected:

	explicit MmuSM83(machine_information& _machine_info);
	// destructor
	~MmuSM83() = default;

	void ResetChildMemoryInstances() override { MemorySM83::resetInstance(); }
	MemorySM83* mem_instance;

	// hardware info and access
	machine_context* machine_ctx;

	void ReadSave() override {
		auto save_files = std::vector<std::string>();
		get_files_in_path(save_files, SAVE_FOLDER);

		const std::string save_file = SAVE_FOLDER + machineInfo.title + SAVE_EXT;

		for (const auto& file : save_files) {
			auto found_file_path = split_string(file, "/");
			std::string found_file = "/" + found_file_path[found_file_path.size() - 2] + "/" + found_file_path[found_file_path.size() - 1];

			if (found_file.compare(save_file) == 0) {
				auto data = std::vector<char>();

				if (read_data(data, save_file, true)) {
					mem_instance->CopyDataToRAM(data);
					LOG_INFO("[emu] loaded from ", save_file);
				} else {
					LOG_ERROR("[emu] reading save file ", save_file);
				}

				return;
			}
		}

		LOG_INFO("[emu] no save file found");
	}

	void WriteSave() const override {
		const std::string save_file = SAVE_FOLDER + machineInfo.title + SAVE_EXT;

		check_and_create_file(save_file, true);
		auto data = std::vector<char>();
		mem_instance->CopyDataFromRAM(data);

		write_data(data, save_file, true, true);
	}
};

/* ***********************************************************************************************************
*
*		ROM only
*
*********************************************************************************************************** */
class MmuSM83_ROM : protected MmuSM83
{
public:
	friend class MmuSM83;

	// members
	void Write8Bit(const u8& _data, const u16& _addr) override;
	void Write16Bit(const u16& _data, const u16& _addr) override;
	u8 Read8Bit(const u16& _addr) override;
	//u16 Read16Bit(const u16& _addr) override;

	// access machine states
	//int GetCurrentSpeed() const override;
	//u8 GetInterruptEnable() const override;
	//u8 GetInterruptRequests() const override;
	//void ResetInterruptRequest(const u8& _isr_flags) override;

private:
	// constructor
	explicit MmuSM83_ROM(machine_information& _machine_info);
	// destructor
	~MmuSM83_ROM() = default;
};

/* ***********************************************************************************************************
*
*		MBC1
*
*********************************************************************************************************** */
class MmuSM83_MBC1 : protected MmuSM83
{
public:
	friend class MmuSM83;

	// members
	void Write8Bit(const u8& _data, const u16& _addr) override;
	void Write16Bit(const u16& _data, const u16& _addr) override;
	u8 Read8Bit(const u16& _addr) override;
	//u16 Read16Bit(const u16& _addr) override;

	// access machine states
	//int GetCurrentSpeed() const override;
	//u8 GetInterruptEnable() const override;
	//u8 GetInterruptRequests() const override;
	//void ResetInterruptRequest(const u8& _isr_flags) override;

private:
	// constructor
	explicit MmuSM83_MBC1(machine_information& _machine_info);
	// destructor
	~MmuSM83_MBC1() = default;

	// mbc1 control
	bool ramEnable = false;
	int ramBankMask = 0x00;

	bool advancedBankingMode = false;
	u8 advancedBankingValue = 0x00;
};

/* ***********************************************************************************************************
*
*		MBC3
*
*********************************************************************************************************** */
class MmuSM83_MBC3 : protected MmuSM83
{
public:
	friend class MmuSM83;

	// members
	void Write8Bit(const u8& _data, const u16& _addr) override;
	void Write16Bit(const u16& _data, const u16& _addr) override;
	u8 Read8Bit(const u16& _addr) override;
	//u16 Read16Bit(const u16& _addr) override;

	// access machine states
	//int GetCurrentSpeed() const override;
	//u8 GetInterruptEnable() const override;
	//u8 GetInterruptRequests() const override;
	//void ResetInterruptRequest(const u8& _isr_flags) override;

private:
	// constructor
	explicit MmuSM83_MBC3(machine_information& _machine_info);
	// destructor
	~MmuSM83_MBC3() = default;

	// mbc3 control
	bool timerRamEnable = false;
	u8 rtcRegistersLastWrite = 0x00;

	void LatchClock();
	u8 ReadClock();
	void WriteClock(const u8& _data);
};