#include "GameboyAPU.h"
#include "gameboy_defines.h"

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
				
				if (soundCtx->ch1Enable.load()) {
					ch1EnvelopeSweep();
				}
				if (soundCtx->ch2Enable.load()) {
					ch2EnvelopeSweep();
				}
			}

			if (soundLengthCounter == SOUND_LENGTH_TICK_RATE) {
				soundLengthCounter = 0;
				
				if (soundCtx->ch1Enable.load()) {
					ch1TickLengthTimer();
				}
				if (soundCtx->ch2Enable.load()) {
					ch2TickLengthTimer();
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
	}
}

void GameboyAPU::ch1PeriodSweep() {
	if (soundCtx->ch1SweepPace != 0) {
		ch1PeriodSweepCounter++;

		if (ch1PeriodSweepCounter >= soundCtx->ch1SweepPace) {
			ch1PeriodSweepCounter = 0;

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

				if (period > CH_1_2_PERIOD_FLIP - 1) {
					writeback = false;
					soundCtx->ch1Enable.store(false);
				}
				break;
			}

			if (writeback) {
				memInstance->GetIO(NR13_ADDR) = period & CH_1_2_PERIOD_LOW;
				u8& nr14 = memInstance->GetIO(NR14_ADDR);
				nr14 = (nr14 & ~CH_1_2_PERIOD_HIGH) | ((period >> 8) & CH_1_2_PERIOD_HIGH);

				soundCtx->ch1Period = period;
				soundCtx->ch1SamplingRate.store((float)(pow(2, 20) / (CH_1_2_PERIOD_FLIP - soundCtx->ch1Period)));
				//LOG_INFO("sweep: fs = ", soundCtx->ch1SamplingRate.load() / pow(2, 3), "; dir: ", soundCtx->ch1SweepDirSubtract ? "sub" : "add");
			}
		}
	}
}

void GameboyAPU::ch1TickLengthTimer() {
	if (soundCtx->ch1LengthEnable) {
		if (soundCtx->ch1LengthAltered) {
			ch1LengthCounter = soundCtx->ch1LengthTimer;

			soundCtx->ch1LengthAltered = false;
		}

		ch1LengthCounter++;
		if (ch1LengthCounter == CH_LENGTH_TIMER_THRESHOLD) {
			soundCtx->ch1Enable.store(false);
		}
		//LOG_INFO("length: l = ", ch1LengthCounter, "; ch on: ", soundCtx->ch1Enable ? "true" : "false");
	}
}

void GameboyAPU::ch1EnvelopeSweep() {
	if (soundCtx->ch1EnvelopePace != 0) {
		ch1EnvelopeSweepCounter++;
		if (ch1EnvelopeSweepCounter >= soundCtx->ch1EnvelopePace) {
			ch1EnvelopeSweepCounter = 0;

			switch (soundCtx->ch1EnvelopeIncrease) {
			case true:
				if (soundCtx->ch1EnvelopeVolume < 0xF) { 
					soundCtx->ch1EnvelopeVolume++; 
					soundCtx->ch1Volume.store((float)soundCtx->ch1EnvelopeVolume / 0xF);
				}
				break;
			case false:
				if (soundCtx->ch1EnvelopeVolume > 0x0) { 
					--soundCtx->ch1EnvelopeVolume;
					soundCtx->ch1Volume.store((float)soundCtx->ch1EnvelopeVolume / 0xF);
				}
				break;
			}
			//LOG_INFO("envelope: x = ", soundCtx->ch1EnvelopeVolume);
		}
	}
}

void GameboyAPU::ch2TickLengthTimer() {
	if (soundCtx->ch2LengthEnable) {
		if (soundCtx->ch2LengthAltered) {
			ch2LengthCounter = soundCtx->ch2LengthTimer;

			soundCtx->ch2LengthAltered = false;
		}

		ch2LengthCounter++;
		if (ch2LengthCounter == CH_LENGTH_TIMER_THRESHOLD) {
			soundCtx->ch2Enable.store(false);
		}
		//LOG_INFO("length: l = ", ch1LengthCounter, "; ch on: ", soundCtx->ch1Enable ? "true" : "false");
	}
}

void GameboyAPU::ch2EnvelopeSweep() {
	if (soundCtx->ch2EnvelopePace != 0) {
		ch2EnvelopeSweepCounter++;
		if (ch2EnvelopeSweepCounter >= soundCtx->ch2EnvelopePace) {
			ch2EnvelopeSweepCounter = 0;

			switch (soundCtx->ch2EnvelopeIncrease) {
			case true:
				if (soundCtx->ch2EnvelopeVolume < 0xF) {
					soundCtx->ch2EnvelopeVolume++;
					soundCtx->ch2Volume.store((float)soundCtx->ch2EnvelopeVolume / 0xF);
				}
				break;
			case false:
				if (soundCtx->ch2EnvelopeVolume > 0x0) {
					--soundCtx->ch2EnvelopeVolume;
					soundCtx->ch2Volume.store((float)soundCtx->ch2EnvelopeVolume / 0xF);
				}
				break;
			}
			//LOG_INFO("envelope: x = ", soundCtx->ch1EnvelopeVolume);
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
void GameboyAPU::SampleAPU(std::vector<std::vector<complex>>& _data, const int& _samples) {
	bool right = soundCtx->outRightEnabled.load();
	bool left = soundCtx->outLeftEnabled.load();
	bool vol_right = soundCtx->masterVolumeRight.load();
	bool vol_left = soundCtx->masterVolumeLeft.load();

	bool ch1_enable = soundCtx->ch1Enable.load();
	bool ch2_enable = soundCtx->ch2Enable.load();

	float ch1_virt_sample_step = soundCtx->ch1SamplingRate.load() / physSamplingRate;
	float ch2_virt_sample_step = soundCtx->ch2SamplingRate.load() / physSamplingRate;

	int ch1_wave_index = soundCtx->ch1DutyCycleIndex.load();
	int ch2_wave_index = soundCtx->ch2DutyCycleIndex.load();

	bool ch1_right = soundCtx->ch1Right.load();
	bool ch1_left = soundCtx->ch1Left.load();

	bool ch2_right = soundCtx->ch2Right.load();
	bool ch2_left = soundCtx->ch2Left.load();

	float ch1_vol = soundCtx->ch1Volume.load();
	float ch2_vol = soundCtx->ch2Volume.load();

	for (int i = 0; i < _samples; i++) {
		for (int n = 0; n < 4; n++) {
			_data[n].emplace_back(.0f, .0f);
		}

		if (ch1_enable) {
			ch1VirtSamples += ch1_virt_sample_step;
			if (ch1VirtSamples >= 1.f) {
				ch1VirtSamples -= 1.f;
				++ch1SampleCount %= 8;
			}


			if (ch1_right) {
				_data[0][i].real += CH_1_2_PWM_SIGNALS[ch1_wave_index][ch1SampleCount] * ch1_vol;
			}
			if (ch1_left) {
				_data[3][i].real += CH_1_2_PWM_SIGNALS[ch1_wave_index][ch1SampleCount] * ch1_vol;
			}
		}

		if (ch2_enable) {
			ch2VirtSamples += ch2_virt_sample_step;
			if (ch2VirtSamples >= 1.f) {
				ch2VirtSamples -= 1.f;
				++ch2SampleCount %= 8;
			}

			if (ch2_right) {
				_data[0][i].real += CH_1_2_PWM_SIGNALS[ch2_wave_index][ch2SampleCount] * ch2_vol;
			}
			if (ch2_left) {
				_data[3][i].real += CH_1_2_PWM_SIGNALS[ch2_wave_index][ch2SampleCount] * ch2_vol;
			}
		}

		_data[0][i].real *= vol_right * .05f;
		_data[1][i].real *= vol_right * .05f;
		_data[2][i].real *= vol_left * .05f;
		_data[3][i].real *= vol_left * .05f;
	}
}