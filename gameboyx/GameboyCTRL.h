#pragma once
#include "BaseCTRL.h"
#include "GameboyMEM.h"

namespace Emulation {
	namespace Gameboy {
		class GameboyCTRL : protected BaseCTRL {
		public:
			friend class BaseCTRL;

			// members
			bool SetKey(const int& _player, const SDL_GameControllerButton& _key) override;
			bool ResetKey(const int& _player, const SDL_GameControllerButton& _key) override;

		private:
			// constructor
			GameboyCTRL(BaseCartridge* _cartridge) : BaseCTRL() {
				memInstance = (GameboyMEM*)BaseMEM::getInstance(_cartridge);
				controlCtx = memInstance->GetControlContext();
			}
			// destructor
			~GameboyCTRL() override = default;

			// memory access
			GameboyMEM* memInstance;
			control_context* controlCtx;
		};
	}
}