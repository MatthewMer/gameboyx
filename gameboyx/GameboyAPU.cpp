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
	ch1FrequencySweepPaceCounter++;
	if (ch1FrequencySweepPaceCounter == ch1SweepPace) {
		ch1SweepPace = soundCtx->ch1SweepPace;
		ch1FrequencySweepPaceCounter = 0;

		ch1SweepEnabled = ch1SweepPace != 0;
		if (ch1SweepEnabled) {
			if (soundCtx->ch1Period != 0) {
				int period;
				bool writeback = true;

				if (soundCtx->ch1SweepDirSubtract) {
					period = soundCtx->ch1Period - (soundCtx->ch1Period >> soundCtx->ch1SweepPeriodStep);
					if (period < 0) {
						period = 0;
					} else {

					}
					soundCtx->ch1Period = period;
				} else {
					period = soundCtx->ch1Period + (soundCtx->ch1Period >> soundCtx->ch1SweepPeriodStep);

					if (period > CH_1_2_PERIOD_THRESHOLD) {
						soundCtx->ch1Enable = false;
						writeback = false;
					} else {
						soundCtx->ch1Period = period;
						soundCtx->ch1Frequency = CH_1_2_PERIOD_CLOCK / (CH_1_2_PERIOD_FLIP - period);
					}
				}

				if (writeback) {
					u8& nr13 = memInstance->GetIO(NR13_ADDR);
					u8& nr14 = memInstance->GetIO(NR14_ADDR);

					nr13 = (u8)(period & CH_1_2_PERIOD_LOW);
					nr14 = (nr14 & ~(CH_1_2_PERIOD_HIGH)) | (u8)((period >> 8) & CH_1_2_PERIOD_HIGH);
				}
			}
		}
	}
}

void GameboyAPU::ch1TickLengthTimer() {
	if (soundCtx->ch1LengthEnable) {
		//if (soundCtx->ch1LengthAltered) {
			ch1LengthTimerInitialValue = soundCtx->ch1LengthTimer;
			ch1LengthTimer = 0;

			//soundCtx->ch1LengthAltered = false;

		ch1LengthTimer++;
		if (ch1LengthTimer == CH_LENGTH_TIMER_THRESHOLD) {
			soundCtx->ch1Enable = false;
		}
	}
}

// filles each vector with samples of the virtual channels and sets the virtual sampling rates for each channel (power of 2)
void GameboyAPU::SampleAPU(std::vector<std::vector<float>> _data, const int& _samples, int* _sampling_rates) {
	
}