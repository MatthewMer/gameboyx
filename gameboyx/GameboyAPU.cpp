#include "GameboyAPU.h"

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
				

			}

			if (ch1FrequencyCounter == CH1_FREQU_SWEEP_RATE) {
				ch1FrequencyCounter = 0;


			}


		}
	}
}

// filles each vector with samples of the virtual channels and sets the virtual sampling rates for each channel (power of 2)
void GameboyAPU::SampleAPU(std::vector<std::vector<complex>>& _data, std::vector<int>& _frequencies) {
	
}