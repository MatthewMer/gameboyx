#pragma once

#include "HardwareMgr.h"
#include "BaseCartridge.h"

namespace Emulation {
	class BaseAPU {
	public:
		// get/reset instance
		static BaseAPU* getInstance(BaseCartridge* _cartridge);
		static BaseAPU* getInstance();
		static void resetInstance();

		// clone/assign protection
		BaseAPU(BaseAPU const&) = delete;
		BaseAPU(BaseAPU&&) = delete;
		BaseAPU& operator=(BaseAPU const&) = delete;
		BaseAPU& operator=(BaseAPU&&) = delete;

		// public members
		virtual void ProcessAPU(const int& _ticks) = 0;
		virtual void SampleAPU(std::vector<std::vector<std::complex<float>>>& _data, const int& _samples, const int& _sampling_rate) = 0;

	protected:
		// constructor
		BaseAPU() {}
		virtual ~BaseAPU() = default;

	private:
		static BaseAPU* instance;
	};
}