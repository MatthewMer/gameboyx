#include "GameboyAPU.h"
#include "gameboy_defines.h"

const float CH_1_2_PWM_SIGNALS[4][8] = {
	{1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, -1.f},
	{-1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, -1.f},
	{-1.f, -1.f, 1.f, 1.f, 1.f, 1.f, -1.f, -1.f},
	{-1.f, -1.f, -1.f, 1.f, 1.f, -1.f, -1.f, -1.f},
};


void GameboyAPU::ProcessAPU(const int& _ticks) {
	if (soundCtx->apuEnable) {
		for (int i = 0; i < _ticks; i++) {
			envelopeSweepCounter++;
			soundLengthCounter++;
			ch1FrequencyCounter++;

			if (envelopeSweepCounter == ENVELOPE_SWEEP_TICK_RATE) {
				envelopeSweepCounter = 0;
				
				if (soundCtx->ch1Enable) {
					ch1EnvelopeSweep();
				}
			}

			if (soundLengthCounter == SOUND_LENGTH_TICK_RATE) {
				soundLengthCounter = 0;
				
				if (soundCtx->ch1Enable) {
					ch1TickLengthTimer();
				}
			}

			if (ch1FrequencyCounter == CH1_FREQU_SWEEP_RATE) {
				ch1FrequencyCounter = 0;

				if (soundCtx->ch1Enable) {
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
					soundCtx->ch1Enable = false;
				}
				break;
			}

			if (writeback) {
				memInstance->GetIO(NR13_ADDR) = period & CH_1_2_PERIOD_LOW;
				u8& nr14 = memInstance->GetIO(NR14_ADDR);
				nr14 = (nr14 & ~CH_1_2_PERIOD_HIGH) | ((period >> 8) & CH_1_2_PERIOD_HIGH);

				soundCtx->ch1Period = period;

				LOG_INFO("sweep: fs = ", pow(2, 17) / (CH_1_2_PERIOD_FLIP - soundCtx->ch1Period));
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
			soundCtx->ch1Enable = false;
		}
		LOG_INFO("length: l = ", ch1LengthCounter, "; ch on: ", soundCtx->ch1Enable ? "true" : "false");
	}
}

void GameboyAPU::ch1EnvelopeSweep() {
	if (soundCtx->ch1EnvelopePace != 0) {
		ch1EnvelopeSweepCounter++;
		if (ch1EnvelopeSweepCounter >= soundCtx->ch1EnvelopePace) {
			ch1EnvelopeSweepCounter = 0;

			switch (soundCtx->ch1EnvelopeIncrease) {
			case true:
				if (soundCtx->ch1EnvelopeVolume < 0xF) { soundCtx->ch1EnvelopeVolume++; }
				break;
			case false:
				if (soundCtx->ch1EnvelopeVolume > 0x0) { --soundCtx->ch1EnvelopeVolume; }
				break;
			}
			LOG_INFO("envelope: x = ", soundCtx->ch1EnvelopeVolume);
		}
	}
}

// filles each vector with samples of the virtual channels and sets the virtual sampling rates for each channel (power of 2)
void GameboyAPU::SampleAPU(std::vector<std::vector<float>> _data, const int& _samples, int* _sampling_rates) {
	
}