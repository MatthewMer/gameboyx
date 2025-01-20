#include "GameboyAPU.h"
#include "gameboy_defines.h"

#include <mutex>

using namespace std;

namespace Emulation {
	namespace Gameboy {
		/* *************************************************************************************************
			PWM SIGNALS
		************************************************************************************************* */
		const float CH_1_2_PWM_SIGNALS[4][8] = {
			{1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, -1.f},
			{-1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, -1.f},
			{-1.f, 1.f, 1.f, 1.f, 1.f, -1.f, -1.f, -1.f},
			{1.f, -1.f, -1.f, -1.f, -1.f, -1.f, -1.f, 1.f},
		};

		/* *************************************************************************************************
			CONSTRUCTOR
		************************************************************************************************* */
		GameboyAPU::GameboyAPU(std::shared_ptr<BaseCartridge> _cartridge) : BaseAPU() {}

		GameboyAPU::~GameboyAPU() {
			Backend::HardwareMgr::StopAudioBackend();
		}

		void GameboyAPU::Init() {
			m_MemInstance = std::dynamic_pointer_cast<GameboyMEM>(BaseMEM::s_GetInstance());
			soundCtx = m_MemInstance.lock()->GetSoundContext();

			virtualChannels = APU_CHANNELS_NUM;

			Backend::virtual_audio_information virt_audio_info = {};
			virt_audio_info.channels = virtualChannels;
			virt_audio_info.apu_callback = [this](std::vector<std::complex<float>>& _samples, const int& _num, const int& _sampling_rate) {
				this->SampleAPU(_samples, _num, _sampling_rate);
				};
			Backend::HardwareMgr::StartAudioBackend(virt_audio_info);
		}

		/* *************************************************************************************************
			APU PROCESSING -> DIRECTLY ATTACHED TO THE CPUS INTERNAL TIMERS
		************************************************************************************************* */
		void GameboyAPU::ProcessAPU(const int& _ticks) {
			if (soundCtx->apuEnable) {
				for (int i = 0; i < _ticks; i++) {
					envelopeSweepCounter++;
					soundLengthCounter++;
					ch1SamplingRateCounter++;

					bool ch_enable[4] = {
						soundCtx->ch_ctxs[0].enable.load(),
						soundCtx->ch_ctxs[1].enable.load(),
						soundCtx->ch_ctxs[2].enable.load(),
						soundCtx->ch_ctxs[3].enable.load()
					};

					if (envelopeSweepCounter == ENVELOPE_SWEEP_TICK_RATE) {
						envelopeSweepCounter = 0;

						for (int j = 0; j < 4; j++) {
							if (j != 2 && ch_enable[j]) {
								envelopeSweep(&chInfos[j], &soundCtx->ch_ctxs[j]);
							}
						}
					}

					if (soundLengthCounter == SOUND_LENGTH_TICK_RATE) {
						soundLengthCounter = 0;

						for (int j = 0; j < 4; j++) {
							if (ch_enable[j]) {
								tickLengthTimer(&chInfos[j], &soundCtx->ch_ctxs[j]);
							}
						}
					}

					if (ch1SamplingRateCounter == CH1_FREQU_SWEEP_RATE) {
						ch1SamplingRateCounter = 0;

						auto* ch_ctx = &soundCtx->ch_ctxs[0];
						if (ch_ctx->enable.load()) {
							// frequency sweep
							periodSweep(&chInfos[0], ch_ctx);
						}
					}
				}

				TickLFSR(_ticks, &chInfos[3], &soundCtx->ch_ctxs[3]);
			}
		}

		/* *************************************************************************************************
			GENERATE NEW SAMPLES DURING INSTRUCTION EXECUTION, AS SOME GAMES RELY ON THIS
		************************************************************************************************* */
		void GameboyAPU::GenerateSamples(const int& _ticks) {
			SampleWaveRam(_ticks, &chInfos[2], &soundCtx->ch_ctxs[2]);
		}

		/* *************************************************************************************************
			GENERATE NEW SAMPLES BASED ON CURRENT WAVE RAM STATE
			some games require fast changes in volume and RAM state for more complex wave forms to work.
			One such example is pokemon yellow, here is a good breakdown of how the "pikachu" sound was
			achieved and why this is necessary: https://www.youtube.com/watch?v=fooSxCuWvZ4&t=421s
		************************************************************************************************* */
		void GameboyAPU::SampleWaveRam(const int& _ticks, channel_info* _ch_info, channel_context* _ch_ctx) {
			if (_ch_ctx->enable.load() && !_ch_ctx->use_current_state) {
				auto* wave_ctx = static_cast<ch_ext_waveram*>(_ch_ctx->exts[WAVE_RAM].get());

				int ticks_per_sample = (int)(BASE_CLOCK_CPU / _ch_ctx->sampling_rate.load());
				float volume = _ch_ctx->volume.load();

				wave_ctx->sample_tick_count += _ticks;
				for (; wave_ctx->sample_tick_count >= ticks_per_sample; wave_ctx->sample_tick_count -= ticks_per_sample) {
					++_ch_info->sample_count %= 32;
				}

				int write_cursor = ch3WriteCursor.load();
				int read_cursor = ch3ReadCursor.load();

				int ticks_per_sample_phys = ticksPerSample.load();
				ch3WaveTickCounter += _ticks;
				for (; ch3WaveTickCounter >= ticks_per_sample_phys; ch3WaveTickCounter -= ticks_per_sample_phys) {
					if (write_cursor != read_cursor) {
						ch3WaveSamples[write_cursor] = wave_ctx->wave_ram[_ch_info->sample_count] * volume;
						++write_cursor %= CH_3_WAVERAM_BUFFER_SIZE;
					}
				}

				ch3WriteCursor.store(write_cursor);
			}
		}

		/* *************************************************************************************************
			PROCESSES AND SAMPLES THE LFSR -> BASICALLY A PSEUDO RANDOM NUMBER GENERATOR (NOISE)
		************************************************************************************************* */
		void GameboyAPU::TickLFSR(const int& _ticks, channel_info* _ch_info, channel_context* _ch_ctx) {
			if (_ch_ctx->enable.load()) {
				auto* lfsr_ctx = static_cast<ch_ext_lfsr*>(_ch_ctx->exts[LFSR].get());
				for (int i = 0; i < _ticks * CH_4_LFSR_TICKS_PER_APU_TICK; i++) {
					ch4LFSRTickCounter += lfsr_ctx->lfsr_step;

					if (ch4LFSRTickCounter > 1.f) {
						ch4LFSRTickCounter -= 1.f;

						// tick LFSR
						u16& lfsr = lfsr_ctx->lfsr;
						int next = (lfsr & 0x01) ^ ((lfsr >> 1) & 0x01);

						switch (lfsr_ctx->lfsr_width_7bit) {
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
					}
				}
			}
		}

		/* *************************************************************************************************
			TICKS THE LENGHT TIMERS -> HOW LONG THE CHANNEL IS TURNED ON (IF SET)
		************************************************************************************************* */
		void GameboyAPU::tickLengthTimer(channel_info* _ch_info, channel_context* _ch_ctx) {
			if (_ch_ctx->length_enable) {
				if (_ch_ctx->length_altered) {
					_ch_info->length_counter = _ch_ctx->length_timer;

					_ch_ctx->length_altered = false;
				}

				_ch_info->length_counter++;
				if (_ch_info->length_counter == CH_LENGTH_TIMER_THRESHOLD) {
					_ch_ctx->enable.store(false);
					m_MemInstance.lock()->GetIO(NR52_ADDR) &= ~_ch_ctx->enable_bit;
				}
			}
		}

		/* *************************************************************************************************
			ALTERS VOLUME OVER TIME (IF SET)
		************************************************************************************************* */
		void GameboyAPU::envelopeSweep(channel_info* _ch_info, channel_context* _ch_ctx) {
			auto* env_ctx = static_cast<ch_ext_envelope*>(_ch_ctx->exts[ENVELOPE].get());

			if (env_ctx->envelope_pace != 0) {
				_ch_info->envelope_sweep_counter++;

				if (_ch_info->envelope_sweep_counter >= env_ctx->envelope_pace) {
					_ch_info->envelope_sweep_counter = 0;

					switch (env_ctx->envelope_increase) {
					case true:
						if (env_ctx->envelope_volume < 0xF) {
							env_ctx->envelope_volume++;
							_ch_ctx->volume.store((float)env_ctx->envelope_volume / 0xF);
						}
						break;
					case false:
						if (env_ctx->envelope_volume > 0x0) {
							--env_ctx->envelope_volume;
							_ch_ctx->volume.store((float)env_ctx->envelope_volume / 0xF);
						}
						break;
					}
				}
			}
		}

		/* *************************************************************************************************
			ALTERS THE SIGNAL FREQUENCY/TONE OVER TIME (IF SET)
		************************************************************************************************* */
		void GameboyAPU::periodSweep(channel_info* _ch_info, channel_context* _ch_ctx) {
			auto* period_ctx = static_cast<ch_ext_period*>(_ch_ctx->exts[PERIOD].get());

			if (period_ctx->sweep_pace != 0) {
				_ch_info->period_sweep_counter++;

				if (_ch_info->period_sweep_counter >= period_ctx->sweep_pace) {
					_ch_info->period_sweep_counter = 0;

					bool writeback = true;

					int period = _ch_ctx->period;
					switch (period_ctx->sweep_dir_subtract) {
					case true:
						period = _ch_ctx->period - (_ch_ctx->period >> period_ctx->sweep_period_step);

						if (period < 1) {
							writeback = false;
						}
						break;
					case false:
						period = _ch_ctx->period + (_ch_ctx->period >> period_ctx->sweep_period_step);

						if (period > CH_1_2_3_PERIOD_FLIP - 1) {
							writeback = false;
							_ch_ctx->enable.store(false);
							m_MemInstance.lock()->GetIO(NR52_ADDR) &= ~_ch_ctx->enable_bit;
						}
						break;
					}

					if (writeback) {
						m_MemInstance.lock()->GetIO(_ch_ctx->regs.nrX3) = period & CH_1_2_PERIOD_LOW;
						u8& period_ctrl = m_MemInstance.lock()->GetIO(_ch_ctx->regs.nrX4);
						period_ctrl = (period_ctrl & ~CH_1_2_3_PERIOD_HIGH) | ((period >> 8) & CH_1_2_3_PERIOD_HIGH);

						_ch_ctx->period = period;
						_ch_ctx->sampling_rate.store((float)(pow(2, 20) / (CH_1_2_3_PERIOD_FLIP - _ch_ctx->period)));
					}
				}
			}
		}

		/* *************************************************************************************************
			THE ACTUAL CALLBACK FOR THE AUDIO BACKEND WHICH GENERATES THE SAMPLES (DISCRETE SIGNAL) TO OUTPUT
		************************************************************************************************* */
		// filles each vector with samples of the virtual channels and sets the virtual sampling rates for each channel (power of 2)
		/* sample order :
		* 1. front right
		* 2. rear right
		* 3. rear left
		* 4. front left
		*/
		void GameboyAPU::SampleAPU(std::vector<std::complex<float>>& _data, const int& _samples, const int& _sampling_rate) {
			NextChunk();

			// TODO: update sampling rate via callback passed to audio backend
			ticksPerSample.store((int)((BASE_CLOCK_CPU / _sampling_rate) + .5f));

			bool right = soundCtx->outRightEnabled.load();
			bool left = soundCtx->outLeftEnabled.load();
			bool vol_right = soundCtx->masterVolumeRight.load();
			bool vol_left = soundCtx->masterVolumeLeft.load();

			channel_context* ch1_ctx = &soundCtx->ch_ctxs[0];
			channel_context* ch2_ctx = &soundCtx->ch_ctxs[1];
			channel_context* ch3_ctx = &soundCtx->ch_ctxs[2];
			channel_context* ch4_ctx = &soundCtx->ch_ctxs[3];

			float sample_steps[4] = {
				ch1_ctx->sampling_rate.load() / _sampling_rate,
				ch2_ctx->sampling_rate.load() / _sampling_rate,
				ch3_ctx->sampling_rate.load() / _sampling_rate,
				ch4_ctx->sampling_rate.load() / _sampling_rate
			};

			bool ch_enabled[4] = {
				ch1_ctx->enable.load(),
				ch2_ctx->enable.load(),
				ch3_ctx->enable.load(),
				ch4_ctx->enable.load()
			};

			bool ch_lr_enabled[4][2] = {
				{ch1_ctx->right.load(), ch1_ctx->left.load()},
				{ch2_ctx->right.load(), ch2_ctx->left.load()},
				{ch3_ctx->right.load(), ch3_ctx->left.load()},
				{ch4_ctx->right.load(), ch4_ctx->left.load()}
			};

			float ch_volume[4] = {
				ch1_ctx->volume.load(),
				ch2_ctx->volume.load(),
				ch3_ctx->volume.load(),
				ch4_ctx->volume.load()
			};

			float amps[4] = {
				ch_lr_enabled[0][0] && ch_lr_enabled[0][1] ? 1.f : 2.f,
				ch_lr_enabled[1][0] && ch_lr_enabled[1][1] ? 1.f : 2.f,
				ch_lr_enabled[2][0] && ch_lr_enabled[2][1] ? 1.f : 2.f,
				ch_lr_enabled[3][0] && ch_lr_enabled[3][1] ? 1.f : 2.f
			};

			int duty_cycle_index[2] = {
				static_cast<ch_ext_pwm*>(ch1_ctx->exts[PWM].get())->duty_cycle_index.load(),
				static_cast<ch_ext_pwm*>(ch2_ctx->exts[PWM].get())->duty_cycle_index.load()
			};

			bool ch3_stable_state = ch3_ctx->use_current_state;
			ch3_ctx->use_current_state = true;

			auto* wave_ctx = static_cast<ch_ext_waveram*>(ch3_ctx->exts[WAVE_RAM].get());

			unique_lock<mutex> lock_wave_ram(wave_ctx->mut_wave_ram, std::defer_lock);
			unique_lock<mutex> lock_lfsr_buffer(mutLFSR, std::defer_lock);

			float samples[4] = { .0f, .0f, .0f, .0f };

			for (int i = 0; i < _samples; i++) {
				memset(samples, 0x00, sizeof(float) * 4);

				for (int j = 0; j < 2; j++) {
					if (ch_enabled[j]) {
						chInfos[j].virt_samples += sample_steps[j];
						while (chInfos[j].virt_samples > 1.f) {
							chInfos[j].virt_samples -= 1.f;
							++chInfos[j].sample_count %= 8;
						}


						if (ch_lr_enabled[j][0]) {
							samples[1] += CH_1_2_PWM_SIGNALS[duty_cycle_index[j]][chInfos[j].sample_count] * ch_volume[j] * amps[j];
						}
						if (ch_lr_enabled[j][1]) {
							samples[0] += CH_1_2_PWM_SIGNALS[duty_cycle_index[j]][chInfos[j].sample_count] * ch_volume[j] * amps[j];
						}
					}
				}

				{
					auto& ch3Info = chInfos[2];

					if (ch3_stable_state) {
						ch3Info.virt_samples += sample_steps[2];
						while (ch3Info.virt_samples > 1.f) {
							ch3Info.virt_samples -= 1.f;
							++ch3Info.sample_count %= 32;
						}

						lock_wave_ram.lock();
						if (ch_lr_enabled[2][0]) {
							samples[3] += wave_ctx->wave_ram[ch3Info.sample_count] * ch_volume[2] * amps[2];
						}
						if (ch_lr_enabled[2][1]) {
							samples[2] += wave_ctx->wave_ram[ch3Info.sample_count] * ch_volume[2] * amps[2];
						}
						lock_wave_ram.unlock();
					}

					int read_cursor = ch3ReadCursor.load();
					int write_cursor = (ch3WriteCursor.load() - 1) % CH_3_WAVERAM_BUFFER_SIZE;

					if (read_cursor != write_cursor) {
						if (!ch3_stable_state) {
							lock_wave_ram.lock();
							if (ch_lr_enabled[2][0]) {
								samples[3] += ch3WaveSamples[read_cursor] * amps[2];
							}
							if (ch_lr_enabled[2][1]) {
								samples[2] += ch3WaveSamples[read_cursor] * amps[2];
							}
							lock_wave_ram.unlock();
						}

						++read_cursor %= CH_3_WAVERAM_BUFFER_SIZE;
						ch3ReadCursor.store(read_cursor);
					}
				}

				{
					auto& ch4Info = chInfos[3];
					lock_lfsr_buffer.lock();
					ch4Info.virt_samples += sample_steps[3];

					int read_cursor = ch4ReadCursor.load();
					int write_cursor = (ch4WriteCursor.load() - 1) % CH_4_LFSR_BUFFER_SIZE;
					while (ch4Info.virt_samples > 1.f) {
						ch4Info.virt_samples -= 1.f;

						// (0 - 1) % 10 = -1 % 10 and is defined as -1 * 10 + 9, where the added value is the remainder (remainder is by definition always positiv)
						if (read_cursor != write_cursor) {
							ch4LFSRSamples[read_cursor] = .0f;
							++read_cursor %= CH_4_LFSR_BUFFER_SIZE;
							ch4ReadCursor.store(read_cursor);
						}
					}

					if (ch_lr_enabled[3][0]) {
						samples[3] += ch4LFSRSamples[ch4ReadCursor] * ch_volume[3] * amps[3];
					}
					if (ch_lr_enabled[3][1]) {
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