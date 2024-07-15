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

namespace Emulation {
	namespace Gameboy {
		class GameboyMMU : public BaseMMU {
		public:
			friend class BaseMMU;

			static std::shared_ptr<GameboyMMU> s_GetInstance(std::shared_ptr<BaseCartridge> _cartridge);
			virtual void Init() override;

			// members
			virtual void Write8Bit(const u8& _data, const u16& _addr) override = 0;
			virtual void Write16Bit(const u16& _data, const u16& _addr) override = 0;
			virtual u8 Read8Bit(const u16& _addr) override = 0;

		protected:
			explicit GameboyMMU(std::shared_ptr<BaseCartridge> _cartridge);

			std::weak_ptr<GameboyMEM> m_MemInstance;

			// hardware info and access
			machine_context* machineCtx;

			std::string saveFile = "";

			/*
			void ReadSave() override {
				if (check_file_exists(saveFile)) {
					auto data = std::vector<char>();

					if (read_data(data, saveFile)) {
						m_MemInstance.lock()->CopyDataToRAM(data);
						LOG_INFO("[emu] loaded from ", saveFile);
					} else {
						LOG_ERROR("[emu] reading save file ", saveFile);
					}
				} else {
					LOG_INFO("[emu] no save file found");
				}
			}
			*/

			/*
			void WriteSave() override {
				if (saveFinished.load()) {
					if (saveThread.joinable()) {
						saveThread.join();
					}

					check_and_create_file(saveFile);

					saveFinished.store(false);
					saveTimePrev = high_resolution_clock::now();
					saveThread = std::thread(save_thread, (BaseMMU*)this, saveFile);
				}

				std::unique_lock<std::mutex> lock_save_time(mutSave);
				m_MemInstance.lock()->CopyDataFromRAM(saveData);
				saveTimePrev = steady_clock::now();
			}
			*/
		};

		/* ***********************************************************************************************************
		*
		*		ROM only
		*
		*********************************************************************************************************** */
		class MmuSM83_ROM : public GameboyMMU {
		public:
			friend class GameboyMMU;
			// constructor
			explicit MmuSM83_ROM(std::shared_ptr<BaseCartridge> _cartridge);
			void Init() override;

			// members
			void Write8Bit(const u8& _data, const u16& _addr) override;
			void Write16Bit(const u16& _data, const u16& _addr) override;
			u8 Read8Bit(const u16& _addr) override;
			//u16 Read16Bit(const u16& _addr) override;
		};

		/* ***********************************************************************************************************
		*
		*		MBC1
		*
		*********************************************************************************************************** */
		class MmuSM83_MBC1 : public GameboyMMU {
		public:
			friend class GameboyMMU;
			// constructor
			explicit MmuSM83_MBC1(std::shared_ptr<BaseCartridge> _cartridge);
			void Init() override;

			// members
			void Write8Bit(const u8& _data, const u16& _addr) override;
			void Write16Bit(const u16& _data, const u16& _addr) override;
			u8 Read8Bit(const u16& _addr) override;
			//u16 Read16Bit(const u16& _addr) override;

		private:
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
		class MmuSM83_MBC3 : public GameboyMMU {
		public:
			friend class GameboyMMU;
			// constructor
			explicit MmuSM83_MBC3(std::shared_ptr<BaseCartridge> _cartridge);
			void Init() override;

			// members
			void Write8Bit(const u8& _data, const u16& _addr) override;
			void Write16Bit(const u16& _data, const u16& _addr) override;
			u8 Read8Bit(const u16& _addr) override;
			//u16 Read16Bit(const u16& _addr) override;

		private:
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
		class MmuSM83_MBC5 : public GameboyMMU {
		public:
			friend class GameboyMMU;
			// constructor
			explicit MmuSM83_MBC5(std::shared_ptr<BaseCartridge> _cartridge);
			void Init() override;

			// members
			void Write8Bit(const u8& _data, const u16& _addr) override;
			void Write16Bit(const u16& _data, const u16& _addr) override;
			u8 Read8Bit(const u16& _addr) override;
			//u16 Read16Bit(const u16& _addr) override;

		private:
			// mbc1 control
			bool ramEnable = false;
			int ramBankMask = 0x00;

			int romBankValue = 0x00;
			bool rom0Mapped = false;
		};
	}
}