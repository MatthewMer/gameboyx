#pragma once
#include "BaseCTRL.h"
#include "GameboyMEM.h"

class GameboyCTRL : protected BaseCTRL {
public:
	friend class BaseCTRL;

	// members
	bool SetKey(const SDL_Keycode& _key) override;
	bool ResetKey(const SDL_Keycode& _key) override;

private:
	// constructor
	GameboyCTRL(BaseCartridge* _cartridge) : BaseCTRL() {
		memInstance = (GameboyMEM*)BaseMEM::getInstance(_cartridge);
		controlCtx = memInstance->GetControlContext();

		InitKeyMap();
	}
	// destructor
	~GameboyCTRL() = default;

	void InitKeyMap() override;

	// memory access
	GameboyMEM* memInstance;
	control_context* controlCtx;
};