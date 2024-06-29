#pragma once
#include "BaseAPU.h"
#include "GameboyMEM.h"

#include "gameboy_defines.h"
#include <vector>
#include <queue>

namespace Emulation {
	namespace Gameboy {
		struct channel_info {
			int length_counter = 0;
			int envelope_sweep_counter = 0;
			int sample_count = 0;
			float virt_samples = .0f;
			int period_sweep_counter = 0;
		};

		class GameboyAPU : protected BaseAPU {
		public:
			friend class BaseAPU;

			// members
			void ProcessAPU(const int& _ticks) override;
			void SampleAPU(std::vector<std::complex<float>>& _data, const int& _samples, const int& _sampling_rate) override;

		private:
			// constructor
			GameboyAPU(BaseCartridge* _cartridge) : BaseAPU() {
				memInstance = (GameboyMEM*)BaseMEM::getInstance();
				soundCtx = memInstance->GetSoundContext();

				virtualChannels = APU_CHANNELS_NUM;

				Backend::virtual_audio_information virt_audio_info = {};
				virt_audio_info.channels = virtualChannels;
				virt_audio_info.apu_callback = [this](std::vector<std::complex<float>>& _samples, const int& _num, const int& _sampling_rate) {
					this->SampleAPU(_samples, _num, _sampling_rate);
					};
				Backend::HardwareMgr::StartAudioBackend(virt_audio_info);
			}
			// destructor
			~GameboyAPU() override {
				Backend::HardwareMgr::StopAudioBackend();
			}

			void tickLengthTimer(bool& _length_altered, const int& _length_timer, int& _length_counter, const u8& _ch_enable_bit, std::atomic<bool>* _ch_enable);
			void envelopeSweep(int& _sweep_counter, const int& _sweep_pace, const bool& _envelope_increase, int& _envelope_volume, std::atomic<float>* _volume);

			// TODO: probably combine these into structs and pass them by reference to the sweep and timer functions
			int envelopeSweepCounter = 0;
			int soundLengthCounter = 0;
			int ch1SamplingRateCounter = 0;

			
			void ch1PeriodSweep();

			channel_info chInfo[4] = { {}, {}, {}, {} };

			float ch4LFSRTickCounter = 0;
			std::mutex mutLFSR;
			std::vector<float> ch4LFSRSamples = std::vector<float>(CH_4_LFSR_BUFFER_SIZE);

			int virtualChannels = 0;

			alignas(64) std::atomic<int> ch4WriteCursor = 1;			// always points to the next sample to write
			alignas(64) std::atomic<int> ch4ReadCursor = 0;				// always points to the current sample to read

			void TickLFSR(const int& _ticks);

			GameboyMEM* memInstance = nullptr;
			sound_context* soundCtx = nullptr;
		};
	}
}