#include "GameboyAPU.h"

void GameboyAPU::ProcessAPU(const int& _ticks) {
	// TODO: process APU (read registers and store parameters for sample production and process timer based events)
}

// filles each vector with samples of the virtual channels and sets the virtual sampling rates for each channel (power of 2)
void GameboyAPU::SampleAPU(std::vector<std::vector<complex>>& _data, std::vector<int>& _frequencies) {
	
}