#pragma once
#include "BaseAPU.h"
#include "GameboyMEM.h"

#include "gameboy_defines.h"
#include <vector>
#include <queue>

namespace Emulation {
	namespace Gameboy {
		/* *************************************************************************************************
			THE STRUCT THAT HOLDS THE CHANNEL SPECIFIC DATA USED BY THE PROCESS FUNCTION 
			(BOUND TO THE CPUS TIMERS)
		************************************************************************************************* */
		struct channel_info {
			int length_counter = 0;
			int envelope_sweep_counter = 0;
			int sample_count = 0;
			float virt_samples = .0f;
			int period_sweep_counter = 0;
		};

		/* *************************************************************************************************
			GameboyAPU CLASS
		************************************************************************************************* */
		class GameboyAPU : protected BaseAPU {
		public:
			friend class BaseAPU;

			// members
			void ProcessAPU(const int& _ticks) override;
			void GenerateSamples(const int& _ticks) override;
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

			alignas(64) std::atomic<float> samplesPerTick = 0;

			int envelopeSweepCounter = 0;
			int soundLengthCounter = 0;
			int ch1SamplingRateCounter = 0;

			int virtualChannels = 0;
			channel_info chInfos[4] = { {}, {}, {}, {} };

			void tickLengthTimer(channel_info* _ch_info, channel_context* _ch_ctx);
			void envelopeSweep(channel_info* _ch_info, channel_context* _ch_ctx);
			void periodSweep(channel_info* _ch_info, channel_context* _ch_ctx);

			// Channel 4 specific (probably move to struct, too)
			float ch4LFSRTickCounter = 0;
			std::mutex mutLFSR;
			std::vector<float> ch4LFSRSamples = std::vector<float>(CH_4_LFSR_BUFFER_SIZE);
			alignas(64) std::atomic<int> ch4WriteCursor = 1;			// always points to the next sample to write
			alignas(64) std::atomic<int> ch4ReadCursor = 0;				// always points to the current sample to read

			void TickLFSR(const int& _ticks, channel_info* _ch_info, channel_context* _ch_ctx);

			float ch3WaveTickCounter = 0;
			std::mutex mutWaveRam;
			std::vector<float> ch3WaveSamples = std::vector<float>(CH_4_LFSR_BUFFER_SIZE);
			alignas(64) std::atomic<int> ch3WriteCursor = 1;			// always points to the next sample to write
			alignas(64) std::atomic<int> ch3ReadCursor = 0;				// always points to the current sample to read

			void SampleWaveRam(const int& _ticks, channel_info* _ch_info, channel_context* _ch_ctx);

			GameboyMEM* memInstance = nullptr;
			sound_context* soundCtx = nullptr;
		};
	}
}