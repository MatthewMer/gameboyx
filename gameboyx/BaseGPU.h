#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	The BaseGPU class is just used to derive the different GPU types (NES, GB, ..) of the emulated hardware from it.
*	The member methods are as generic as possible to fit every GPU type without exposing the GPUs
*	specific internals.
*/

#include "GraphicsMgr.h"
#include "BaseCartridge.h"
#include "defs.h"
#include "VHardwareStructs.h"

#include <vector>

namespace Emulation {
	class BaseGPU {
	public:
		// get/reset instance
		static BaseGPU* getInstance(BaseCartridge* _cartridge);
		static BaseGPU* getInstance();
		static void resetInstance();

		// clone/assign protection
		BaseGPU(BaseGPU const&) = delete;
		BaseGPU(BaseGPU&&) = delete;
		BaseGPU& operator=(BaseGPU const&) = delete;
		BaseGPU& operator=(BaseGPU&&) = delete;

		// members
		virtual void ProcessGPU(const int& _ticks) = 0;
		virtual int GetDelayTime() const = 0;
		virtual int GetTicksPerFrame(const float& _clock) const = 0;
		int GetFrameCount() const;
		void ResetFrameCount();

		virtual std::vector<std::tuple<int, std::string, bool>> GetGraphicsDebugSettings() = 0;
		virtual void SetGraphicsDebugSetting(const bool& _val, const int& _id) = 0;

	protected:
		// constructor
		BaseGPU() = default;
		virtual ~BaseGPU() {}

		int frameCounter = 0;
		int tickCounter = 0;

		std::vector<u8> imageData;

		std::vector<std::atomic<bool>*> graphicsDebugSettings;

	private:
		static BaseGPU* instance;
	};
}