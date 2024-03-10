#include "GameboyAPU.h"
#include "gameboy_defines.h"

const float CH_1_2_PWM_SIGNALS[4][8] = {
	{1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, .0f},
	{.0f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, .0f},
	{.0f, 1.f, 1.f, 1.f, 1.f, .0f, .0f, .0f},
	{1.f, .0f, .0f, .0f, .0f, .0f, .0f, 1.f},
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
void GameboyAPU::SampleAPU(float* _data) {
	bool right = soundCtx->outRightEnabled.load();
	bool left = soundCtx->outLeftEnabled.load();
	bool vol_right = soundCtx->masterVolumeRight.load();
	bool vol_left = soundCtx->masterVolumeLeft.load();

	if (soundCtx->ch1Enable.load()) {
		float ch1_virt_sample_step = soundCtx->ch1SamplingRate.load() / physSamplingRate;
		ch1VirtSamples += ch1_virt_sample_step;
		if (ch1VirtSamples >= 1.f) {
			ch1VirtSamples -= 1.f;
			++ch1SampleCount %= 8;
		}
		
		int wave_index = soundCtx->ch1DutyCycleIndex.load();
		if (soundCtx->ch1Right.load()) {
			_data[0] += CH_1_2_PWM_SIGNALS[wave_index][ch1SampleCount] * soundCtx->ch1Volume.load() * vol_right;
		}
		if (soundCtx->ch1Left.load()) {
			_data[3] += CH_1_2_PWM_SIGNALS[wave_index][ch1SampleCount] * soundCtx->ch1Volume.load() * vol_left;
		}
	}

	if (soundCtx->ch2Enable.load()) {
		float ch2_virt_sample_step = soundCtx->ch2SamplingRate.load() / physSamplingRate;
		ch2VirtSamples += ch2_virt_sample_step;
		if (ch2VirtSamples >= 1.f) {
			ch2VirtSamples -= 1.f;
			++ch2SampleCount %= 8;
		}

		int wave_index = soundCtx->ch2DutyCycleIndex.load();
		if (soundCtx->ch2Right.load()) {
			_data[0] += CH_1_2_PWM_SIGNALS[wave_index][ch2SampleCount] * soundCtx->ch2Volume.load() * vol_right;
		}
		if (soundCtx->ch2Left.load()) {
			_data[3] += CH_1_2_PWM_SIGNALS[wave_index][ch2SampleCount] * soundCtx->ch2Volume.load() * vol_left; 
		}
	}
}