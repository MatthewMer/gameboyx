#pragma once
#include "SDL.h"

#include "information_structs.h"

class ControllerBase {
public:
	// get/reset instance
	static ControllerBase* getInstance(machine_information& _machine_info);
	static void resetInstance();

	// clone/assign protection
	ControllerBase(ControllerBase const&) = delete;
	ControllerBase(ControllerBase&&) = delete;
	ControllerBase& operator=(ControllerBase const&) = delete;
	ControllerBase& operator=(ControllerBase&&) = delete;

	// public members
	virtual bool SetKey(const SDL_Keycode& _key) = 0;
	virtual bool ResetKey(const SDL_Keycode& _key) = 0;

protected:
	// constructor
	ControllerBase() = default;
	~ControllerBase() = default;

	virtual void InitKeyMap() = 0;

private:
	static ControllerBase* instance;
};