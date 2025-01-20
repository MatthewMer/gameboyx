#pragma once

#include "HardwareMgr.h"
#include "BaseCartridge.h"

namespace Emulation {
	class BaseAPU {
	public:
		// get/reset instance
		static std::shared_ptr<BaseAPU> s_GetInstance(std::shared_ptr<BaseCartridge> _cartridge);
		static std::shared_ptr<BaseAPU> s_GetInstance();
		static void s_ResetInstance();
		virtual void Init() = 0;
		int GetChunkId() const;
		void NextChunk();

		// clone/assign protection
		BaseAPU(BaseAPU const&) = delete;
		BaseAPU(BaseAPU&&) = delete;
		BaseAPU& operator=(BaseAPU const&) = delete;
		BaseAPU& operator=(BaseAPU&&) = delete;

		// public members
		virtual void ProcessAPU(const int& _ticks) = 0;
		virtual void SampleAPU(std::vector<std::complex<float>>& _data, const int& _samples, const int& _sampling_rate) = 0;
		virtual void GenerateSamples(const int& _ticks) = 0;

	protected:
		// constructor
		BaseAPU() {}
		virtual ~BaseAPU() {}

	private:
		int m_audioChunkId = 0;

		static std::weak_ptr<BaseAPU> m_Instance;
	};
}