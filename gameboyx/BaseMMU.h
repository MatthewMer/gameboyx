#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	The BaseMMU class is just used to derive the different "MMU" types (NES, GB, ..) of the emulated hardware from it.
*	The member methods are as generic as possible to fit every memory management without exposing it's
*	specific internals.
*/

#include "HardwareMgr.h"

#include "GameboyCartridge.h"
#include "BaseMEM.h"

#include "general_config.h"
#include "helper_functions.h"

#include "BaseCartridge.h"

#include <string>
#include <thread>
#include <mutex>
#include <chrono>
using namespace std::chrono;

namespace Emulation {
	class BaseMMU {
	public:
		// get/reset instance
		static BaseMMU* getInstance(BaseCartridge* _cartridge);
		static BaseMMU* getInstance();
		static void resetInstance();

		// clone/assign protection
		BaseMMU(BaseMMU const&) = delete;
		BaseMMU(BaseMMU&&) = delete;
		BaseMMU& operator=(BaseMMU const&) = delete;
		BaseMMU& operator=(BaseMMU&&) = delete;

		// members
		virtual void Write8Bit(const u8& _data, const u16& _addr) = 0;
		virtual void Write16Bit(const u16& _data, const u16& _addr) = 0;
		virtual u8 Read8Bit(const u16& _addr) = 0;
		//virtual u16 Read16Bit(const u16& _addr) = 0;

		std::vector<char> GetSaveData();

	protected:
		// constructor
		BaseMMU() = default;
		virtual ~BaseMMU() = default;

		//virtual void ReadSave() = 0;
		//virtual void WriteSave() = 0;

		static BaseMMU* instance;

		std::thread saveThread;
		std::vector<char> saveData = std::vector<char>();
		steady_clock::time_point saveTimePrev;
		std::mutex mutSave;
		std::atomic<bool> saveFinished = true;
	};
}