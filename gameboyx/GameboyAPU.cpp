#include "GameboyAPU.h"
#include "gameboy_defines.h"

#include <mutex>

using namespace std;

namespace Emulation {
	namespace Gameboy {
		const float CH_1_2_PWM_SIGNALS[4][8] = {
			{1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, -1.f},
			{-1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, -1.f},
			{-1.f, 1.f, 1.f, 1.f, 1.f, -1.f, -1.f, -1.f},
			{1.f, -1.f, -1.f, -1.f, -1.f, -1.f, -1.f, 1.f},
		};


		void GameboyAPU::ProcessAPU(const int& _ticks) {
			if (soundCtx->apuEnable) {
				for (int i = 0; i < _ticks; i++) {
					envelopeSweepCounter++;
					soundLengthCounter++;
					ch1SamplingRateCounter++;

					if (envelopeSweepCounter == ENVELOPE_SWEEP_TICK_RATE) {
						envelopeSweepCounter = 0;

						// TODO: put stuff into seperate structs an just pass a pointer/reference to the required struct
						if (soundCtx->ch1Enable.load() && soundCtx->ch1EnvelopePace) {
							envelopeSweep(chInfo[0].envelope_sweep_counter, soundCtx->ch1EnvelopePace, soundCtx->ch1EnvelopeIncrease, soundCtx->ch1EnvelopeVolume, &soundCtx->ch1Volume);
						}
						if (soundCtx->ch2Enable.load() && soundCtx->ch2EnvelopePace) {
							envelopeSweep(chInfo[1].envelope_sweep_counter, soundCtx->ch2EnvelopePace, soundCtx->ch2EnvelopeIncrease, soundCtx->ch2EnvelopeVolume, &soundCtx->ch2Volume);
						}
						if (soundCtx->ch4Enable.load() && soundCtx->ch4EnvelopePace) {
							envelopeSweep(chInfo[3].envelope_sweep_counter, soundCtx->ch4EnvelopePace, soundCtx->ch4EnvelopeIncrease, soundCtx->ch4EnvelopeVolume, &soundCtx->ch4Volume);
						}
					}

					if (soundLengthCounter == SOUND_LENGTH_TICK_RATE) {
						soundLengthCounter = 0;

						if (soundCtx->ch1Enable.load() && soundCtx->ch1LengthEnable) {
							tickLengthTimer(soundCtx->ch1LengthAltered, soundCtx->ch1LengthTimer, chInfo[0].length_counter, CH_1_ENABLE, &soundCtx->ch1Enable);
						}
						if (soundCtx->ch2Enable.load() && soundCtx->ch2LengthEnable) {
							tickLengthTimer(soundCtx->ch2LengthAltered, soundCtx->ch2LengthTimer, chInfo[1].length_counter, CH_2_ENABLE, &soundCtx->ch2Enable);
						}
						if (soundCtx->ch3Enable.load() && soundCtx->ch3LengthEnable) {
							tickLengthTimer(soundCtx->ch3LengthAltered, soundCtx->ch3LengthTimer, chInfo[2].length_counter, CH_3_ENABLE, &soundCtx->ch3Enable);
						}
						if (soundCtx->ch4Enable.load() && soundCtx->ch4LengthEnable) {
							tickLengthTimer(soundCtx->ch4LengthAltered, soundCtx->ch4LengthTimer, chInfo[3].length_counter, CH_4_ENABLE, &soundCtx->ch4Enable);
						}
					}

					if (ch1SamplingRateCounter == CH1_FREQU_SWEEP_RATE) {
						ch1SamplingRateCounter = 0;

						if (soundCtx->ch1Enable.load()) {
							// frequency sweep
							ch1PeriodSweep();
						}
					}
				}

				TickLFSR(_ticks);
			}
		}

		void GameboyAPU::TickLFSR(const int& _ticks) {
			if (soundCtx->ch4Enable.load()) {
				for (int i = 0; i < _ticks * CH_4_LFSR_TICKS_PER_APU_TICK; i++) {
					ch4LFSRTickCounter += soundCtx->ch4LFSRStep;

					if (ch4LFSRTickCounter > 1.f) {
						ch4LFSRTickCounter -= 1.f;

						// tick LFSR
						u16 lfsr = soundCtx->ch4LFSR;
						int next = (lfsr & 0x01) ^ ((lfsr >> 1) & 0x01);

						switch (soundCtx->ch4LFSRWidth7Bit) {
						case true:
							lfsr = (lfsr & ~(CH_4_LFSR_BIT_7 | CH_4_LFSR_BIT_15)) | ((next << 7) | (next << 15));
							break;
						case false:
							lfsr = (lfsr & ~CH_4_LFSR_BIT_15) | (next << 15);
							break;
						}

						lfsr >>= 1;

						int write_cursor = ch4WriteCursor.load();

						if (write_cursor != ch4ReadCursor.load()) {
							unique_lock<mutex> lock_lfsr_buffer(mutLFSR);
							switch (lfsr & 0x0001) {
							case 0x0000:
								ch4LFSRSamples[write_cursor] = -1.f;
								break;
							case 0x0001:
								ch4LFSRSamples[write_cursor] = 1.f;
								break;
							}
							lock_lfsr_buffer.unlock();

							++write_cursor %= CH_4_LFSR_BUFFER_SIZE;
							ch4WriteCursor.store(write_cursor);
						}

						soundCtx->ch4LFSR = lfsr;

						//if(ch4LFSRSamples[ch4LFSRWriteCursor] != .0f)
						//LOG_INFO(ch4LFSRSamples[ch4LFSRWriteCursor], ";", lfsr);
					}
				}
			}
		}

		void GameboyAPU::tickLengthTimer(bool& _length_altered, const int& _length_timer, int& _length_counter, const u8& _ch_enable_bit, std::atomic<bool>* _ch_enable) {
			if (_length_altered) {
				_length_counter = _length_timer;

				_length_altered = false;
			}

			_length_counter++;
			if (_length_counter == CH_LENGTH_TIMER_THRESHOLD) {
				_ch_enable->store(false);
				memInstance->GetIO(NR52_ADDR) &= ~_ch_enable_bit;
			}
		}

		void GameboyAPU::envelopeSweep(int& _sweep_counter, const int& _sweep_pace, const bool& _envelope_increase, int& _envelope_volume, std::atomic<float>* _volume) {
			_sweep_counter++;
			if (_sweep_counter >= _sweep_pace) {
				_sweep_counter = 0;

				switch (_envelope_increase) {
				case true:
					if (_envelope_volume < 0xF) {
						_envelope_volume++;
						_volume->store((float)_envelope_volume / 0xF);
					}
					break;
				case false:
					if (_envelope_volume > 0x0) {
						--_envelope_volume;
						_volume->store((float)_envelope_volume / 0xF);
					}
					break;
				}
			}
		}

		void GameboyAPU::ch1PeriodSweep() {
			if (soundCtx->ch1SweepPace != 0) {
				chInfo[0].period_sweep_counter++;

				if (chInfo[0].period_sweep_counter >= soundCtx->ch1SweepPace) {
					chInfo[0].period_sweep_counter = 0;

					bool writeback = true;

					int period = soundCtx->ch1Period;
					switch (soundCtx->ch1SweepDirSubtract) {
					case true:
						period = soundCtx->ch1Period - (soundCtx->ch1Period >> soundCtx->ch1SweepPeriodStep);

						if (period < 1) {
							writeback = false;
						}
						break;
					case false:
						period = soundCtx->ch1Period + (soundCtx->ch1Period >> soundCtx->ch1SweepPeriodStep);

						if (period > CH_1_2_3_PERIOD_FLIP - 1) {
							writeback = false;
							soundCtx->ch1Enable.store(false);
							memInstance->GetIO(NR52_ADDR) &= ~CH_1_ENABLE;
						}
						break;
					}

					if (writeback) {
						memInstance->GetIO(NR13_ADDR) = period & CH_1_2_PERIOD_LOW;
						u8& nr14 = memInstance->GetIO(NR14_ADDR);
						nr14 = (nr14 & ~CH_1_2_3_PERIOD_HIGH) | ((period >> 8) & CH_1_2_3_PERIOD_HIGH);

						soundCtx->ch1Period = period;
						soundCtx->ch1SamplingRate.store((float)(pow(2, 20) / (CH_1_2_3_PERIOD_FLIP - soundCtx->ch1Period)));
						//LOG_INFO("sweep: fs = ", soundCtx->ch1SamplingRate.load() / pow(2, 3), "; dir: ", soundCtx->ch1SweepDirSubtract ? "sub" : "add");
					}
				}
			}
		}

		// filles each vector with samples of the virtual channels and sets the virtual sampling rates for each channel (power of 2)
		/* sample order :
		* 1. front right
		* 2. rear right
		* 3. rear left
		* 4. front left
		*/
		void GameboyAPU::SampleAPU(std::vector<std::complex<float>>& _data, const int& _samples, const int& _sampling_rate) {
			bool right = soundCtx->outRightEnabled.load();
			bool left = soundCtx->outLeftEnabled.load();
			bool vol_right = soundCtx->masterVolumeRight.load();
			bool vol_left = soundCtx->masterVolumeLeft.load();

			float sample_steps[4] = {
				soundCtx->ch1SamplingRate.load() / _sampling_rate,
				soundCtx->ch2SamplingRate.load() / _sampling_rate,
				soundCtx->ch3SamplingRate.load() / _sampling_rate,
				soundCtx->ch4SamplingRate.load() / _sampling_rate
			};

			bool ch_enabled[4] = {
				soundCtx->ch1Enable.load(),
				soundCtx->ch2Enable.load(),
				soundCtx->ch3Enable.load(),
				soundCtx->ch4Enable.load()
			};

			bool ch_lr_enabled[4][2] = {
				{soundCtx->ch1Right.load(), soundCtx->ch1Left.load()},
				{soundCtx->ch2Right.load(), soundCtx->ch2Left.load()},
				{soundCtx->ch3Right.load(), soundCtx->ch3Left.load()},
				{soundCtx->ch4Right.load(), soundCtx->ch4Left.load()}
			};

			float ch_volume[4] = {
				soundCtx->ch1Volume.load(),
				soundCtx->ch2Volume.load(),
				soundCtx->ch3Volume.load(),
				soundCtx->ch4Volume.load()
			};

			float amps[4] = {
				ch_lr_enabled[0][0] && ch_lr_enabled[0][1] ? 1.f : 2.f,
				ch_lr_enabled[1][0] && ch_lr_enabled[1][1] ? 1.f : 2.f,
				ch_lr_enabled[2][0] && ch_lr_enabled[2][1] ? 1.f : 2.f,
				ch_lr_enabled[3][0] && ch_lr_enabled[3][1] ? 1.f : 2.f
			};

			int duty_cycle_index[2] = {
				soundCtx->ch1DutyCycleIndex.load(),
				soundCtx->ch2DutyCycleIndex.load()
			};

			unique_lock<mutex> lock_wave_ram(soundCtx->mutWaveRam, std::defer_lock);
			unique_lock<mutex> lock_lfsr_buffer(mutLFSR, std::defer_lock);

			float samples[4];

			for (int i = 0; i < _samples; i++) {
				memset(samples, 0x00, sizeof(float) * 4);

				for (int j = 0; j < 2; j++) {
					if (ch_enabled[j]) {
						chInfo[j].virt_samples += sample_steps[j];
						while (chInfo[j].virt_samples > 1.f) {
							chInfo[j].virt_samples -= 1.f;
							++chInfo[j].sample_count %= 8;
						}


						if (ch_lr_enabled[j][0]) {
							samples[1] += CH_1_2_PWM_SIGNALS[duty_cycle_index[j]][chInfo[j].sample_count] * ch_volume[j] * amps[j];
						}
						if (ch_lr_enabled[j][1]) {
							samples[0] += CH_1_2_PWM_SIGNALS[duty_cycle_index[j]][chInfo[j].sample_count] * ch_volume[j] * amps[j];
						}
					}
				}

				if (ch_enabled[2]) {
					auto& ch3Info = chInfo[2];

					ch3Info.virt_samples += sample_steps[2];
					while (ch3Info.virt_samples > 1.f) {
						ch3Info.virt_samples -= 1.f;
						++ch3Info.sample_count %= 32;
					}

					lock_wave_ram.lock();
					if (ch_lr_enabled[2][0]) {
						samples[3] += soundCtx->waveRam[ch3Info.sample_count] * ch_volume[2] * amps[2];
					}
					if (ch_lr_enabled[2][0]) {
						samples[2] += soundCtx->waveRam[ch3Info.sample_count] * ch_volume[2] * amps[2];
					}
					lock_wave_ram.unlock();
				}

				{
					auto& ch4Info = chInfo[3];;
					lock_lfsr_buffer.lock();
					ch4Info.virt_samples += sample_steps[3];
					while (ch4Info.virt_samples > 1.f) {
						ch4Info.virt_samples -= 1.f;

						int read_cursor = ch4ReadCursor.load();

						// (0 - 1) % 10 = -1 % 10 and is defined as -1 * 10 + 9, where the added value is the remainder (remainder is by definition always positiv)
						if (read_cursor != (ch4WriteCursor.load() - 1) % CH_4_LFSR_BUFFER_SIZE) {
							ch4LFSRSamples[read_cursor] = .0f;
							++read_cursor %= CH_4_LFSR_BUFFER_SIZE;
							ch4ReadCursor.store(read_cursor);
						}
					}

					if (ch_lr_enabled[3][0]) {
						samples[3] += ch4LFSRSamples[ch4ReadCursor] * ch_volume[3] * amps[3];
					}
					if (ch_lr_enabled[3][0]) {
						samples[2] += ch4LFSRSamples[ch4ReadCursor] * ch_volume[3] * amps[3];
					}
					lock_lfsr_buffer.unlock();
				}

				_data[i * virtualChannels    ].real(samples[1] * vol_right * .05f);		// front-right
				_data[i * virtualChannels + 1].real(samples[3] * vol_right * .05f);		// rear-right
				_data[i * virtualChannels + 2].real(samples[2] * vol_left * .05f);		// rear-left
				_data[i * virtualChannels + 3].real(samples[0] * vol_left * .05f);		// front-left
			}
		}
	}
}