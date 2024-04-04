#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	The GameboyMMU class emulating the different Gameboy (Color) memory mappers (directly built into the cartridge).
*	Therefor every cartridge can have a different size of ROM and RAM (or no RAM) and a optional built in RTC which
*	lets tick the clock when gameboy is turned off and battery buffered RAM for storing save states.
*	This class is purely there to emulate these memory mappings and extensions.
*/

#include "BaseMMU.h"
#include "GameboyCartridge.h"
#include "defs.h"

#include "GameboyMEM.h"

class GameboyMMU : protected BaseMMU {
public:
	friend class BaseMMU;

	static GameboyMMU* getInstance(BaseCartridge* _cartridge);

	// members
	void Write8Bit(const u8& _data, const u16& _addr) override {};
	void Write16Bit(const u16& _data, const u16& _addr) override {};
	u8 Read8Bit(const u16& _addr) override { return 0xFF; };

protected:

	explicit GameboyMMU(BaseCartridge* _cartridge);
	// destructor
	virtual ~GameboyMMU() {}

	GameboyMEM* mem_instance;

	// hardware info and access
	machine_context* machine_ctx;

	void ReadSave() override {
		std::string save_file = SAVE_FOLDER + machine_ctx->title + SAVE_EXT;

		if (check_file_exists(save_file)) {
			auto data = std::vector<char>();

			if (read_data(data, save_file)) {
				mem_instance->CopyDataToRAM(data);
				LOG_INFO("[emu] loaded from ", save_file);
			} else {
				LOG_ERROR("[emu] reading save file ", save_file);
			}
		} else {
			LOG_INFO("[emu] no save file found");
		}
	}

	void WriteSave() override {
		if (saveFinished.load()) {
			if (saveThread.joinable()) {
				saveThread.join();
			}

			std::string save_file = SAVE_FOLDER + machine_ctx->title + SAVE_EXT;
			check_and_create_file(save_file);

			saveFinished.store(false);
			saveTimePrev = high_resolution_clock::now();
			saveThread = std::thread(save_thread, (BaseMMU*)this, save_file);
		}

		std::unique_lock<std::mutex> lock_save_time(mutSave);
		mem_instance->CopyDataFromRAM(saveData);
		saveTimePrev = steady_clock::now();
	}
};

/* ***********************************************************************************************************
*
*		ROM only
*
*********************************************************************************************************** */
class MmuSM83_ROM : protected GameboyMMU
{
public:
	friend class GameboyMMU;

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
	explicit MmuSM83_ROM(BaseCartridge* _cartridge);
	// destructor
	~MmuSM83_ROM() override {
		BaseMEM::resetInstance();
	}
};

/* ***********************************************************************************************************
*
*		MBC1
*
*********************************************************************************************************** */
class MmuSM83_MBC1 : protected GameboyMMU
{
public:
	friend class GameboyMMU;

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
	explicit MmuSM83_MBC1(BaseCartridge* _cartridge);
	// destructor
	~MmuSM83_MBC1() override {
		BaseMEM::resetInstance();
	}

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
class MmuSM83_MBC3 : protected GameboyMMU
{
public:
	friend class GameboyMMU;

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
	explicit MmuSM83_MBC3(BaseCartridge* _cartridge);
	// destructor
	~MmuSM83_MBC3() override {
		BaseMEM::resetInstance();
	}

	// mbc3 control
	bool timerRamEnable = false;
	u8 rtcRegistersLastWrite = 0x00;

	void LatchClock();
	u8 ReadClock();
	void WriteClock(const u8& _data);
};

/* ***********************************************************************************************************
*
*		MBC5
*
*********************************************************************************************************** */
class MmuSM83_MBC5 : protected GameboyMMU {
public:
	friend class GameboyMMU;

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
	explicit MmuSM83_MBC5(BaseCartridge* _cartridge);
	// destructor
	~MmuSM83_MBC5() override {
		BaseMEM::resetInstance();
	}

	// mbc1 control
	bool ramEnable = false;
	int ramBankMask = 0x00;

	bool rom0Mapped = false;
};