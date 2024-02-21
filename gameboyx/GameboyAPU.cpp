#include "GameboyAPU.h"

void GameboyAPU::ProcessAPU(const int& _ticks) {
	if (soundCtx->apuEnable) {
		for (int i = 0; i < _ticks; i++) {
			static bool envelope_sweep = false;
			static bool sound_length = false;
			static bool ch1_frequency = false;

			envelopeSweepCounter++;
			soundLengthCounter++;
			ch1FrequencyCounter++;

			if (envelopeSweepCounter == ENVELOPE_SWEEP_TICK_RATE) {
				envelopeSweepCounter = 0;
				envelope_sweep = true;
			}

			if (soundLengthCounter == SOUND_LENGTH_TICK_RATE) {
				soundLengthCounter = 0;
				sound_length = true;
			}

			if (ch1FrequencyCounter == CH1_FREQU_SWEEP_RATE) {
				ch1FrequencyCounter = 0;
				ch1_frequency = true;
			}


		}
	}
}

// filles each vector with samples of the virtual channels and sets the virtual sampling rates for each channel (power of 2)
void GameboyAPU::SampleAPU(std::vector<std::vector<complex>>& _data, std::vector<int>& _frequencies) {
	
}