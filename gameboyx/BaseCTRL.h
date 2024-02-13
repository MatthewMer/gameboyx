#pragma once
#include "SDL.h"
#include "BaseCartridge.h"

class BaseCTRL {
public:
	// get/reset instance
	static BaseCTRL* getInstance(BaseCartridge* _cartridge);
	static void resetInstance();

	// clone/assign protection
	BaseCTRL(BaseCTRL const&) = delete;
	BaseCTRL(BaseCTRL&&) = delete;
	BaseCTRL& operator=(BaseCTRL const&) = delete;
	BaseCTRL& operator=(BaseCTRL&&) = delete;

	// public members
	virtual bool SetKey(const SDL_Keycode& _key) = 0;
	virtual bool ResetKey(const SDL_Keycode& _key) = 0;

protected:
	// constructor
	BaseCTRL() = default;
	~BaseCTRL() = default;

	virtual void InitKeyMap() = 0;

	std::unordered_map<SDL_Keycode, int> keyMap = std::unordered_map<SDL_Keycode, int>();

private:
	static BaseCTRL* instance;
};