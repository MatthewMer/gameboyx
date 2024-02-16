#pragma once

#include "defs.h"
#include <vector>
#include <atomic>

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
	alignas(64) std::atomic<bool> audio_running = true;
	BaseAPU* sound_instance = nullptr;

	constexpr virtual_audio_information& operator=(virtual_audio_information& _right) noexcept {
		if (this != &_right) {
			channels = _right.channels;
			audio_running.store(_right.audio_running.load());
			sound_instance = _right.sound_instance;
		}
		return *this;
	}
};

struct audio_information {
	int channels_max = 0;
	int sampling_rate_max = 0;

	float volume = .0f;
	int channels = 0;
	int sampling_rate = 0;

	void* device = nullptr;
};

struct graphics_settings {
	int framerateTarget = 0;
	bool fpsUnlimited = false;
};