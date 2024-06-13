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
};

struct virtual_audio_information {
	int channels = 0;
	alignas(64) std::atomic<bool> audio_running = false;
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

struct graphics_settings {
	int framerateTarget = 0;
	bool fpsUnlimited = false;
	bool tripleBuffering = false;
	bool presentModeFifo = false;
};

struct audio_settings {
	int sampling_rate = 0;
	float master_volume = 0;
	float lfe = 0;
	float delay = 0;
	float decay = 0;
	int sampling_rate_max = 0;
};

struct control_settings {
	bool mouse_always_visible = false;
};

struct network_settings {
	std::string ipv4_address;
	int port;
};