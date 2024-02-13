#pragma once

#include "defs.h"
#include <vector>

class BaseAPU;

struct virtual_graphics_information {
	// drawing mode
	bool is2d = true;
	bool en2d = true;

	// data for gameboy output
	std::vector<u8>* image_data = nullptr;
	u32 lcd_width = 1;
	u32 lcd_height = 1;
	float aspect_ratio = 1.f;
};

struct graphics_information {
	int win_width = 1;
	int win_height = 1;
};

struct virtual_audio_information {
	int channels = 0;
	int cursor = 0;
	std::vector<float>* audio_data;

	bool audio_running = true;

	BaseAPU* sound_instance = nullptr;
};

struct audio_information {
	int channels_max = 0;
	int sampling_rate_max = 0;

	float volume = 1.f;
	int channels = 0;
	int sampling_rate = 0;

	void* device = nullptr;
};