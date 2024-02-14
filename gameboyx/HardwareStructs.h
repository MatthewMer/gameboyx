#pragma once

#include "defs.h"
#include <vector>

class BaseAPU;

struct virtual_graphics_information {
	// drawing mode
	bool is2d = false;
	bool en2d = false;

	// data for gameboy output
	std::vector<u8>* image_data = nullptr;
	u32 lcd_width = 0;
	u32 lcd_height = 0;
	float aspect_ratio = 1.f;

	// settings
	int buffering = 0;
};

struct graphics_information {
	u32 win_width = 0;
	u32 win_height = 0;
};

struct virtual_audio_information {
	int channels = 0;
	int cursor = 0;
	std::vector<float>* audio_data;

	bool audio_running = false;

	BaseAPU* sound_instance = nullptr;
};

struct audio_information {
	int channels_max = 0;
	int sampling_rate_max = 0;

	float volume = .0f;
	int channels = 0;
	int sampling_rate = 0;

	void* device = nullptr;
};