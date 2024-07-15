#pragma once
#include "BaseCTRL.h"
#include "GameboyMEM.h"

namespace Emulation {
	namespace Gameboy {
		class GameboyCTRL : public BaseCTRL {
		public:
			friend class BaseCTRL;
			// constructor
			explicit GameboyCTRL(std::shared_ptr<BaseCartridge> _cartridge);
			void Init() override;

			// members
			bool SetKey(const int& _player, const SDL_GameControllerButton& _key) override;
			bool ResetKey(const int& _player, const SDL_GameControllerButton& _key) override;

		private:
			// memory access
			std::weak_ptr<GameboyMEM> m_MemInstance;
			control_context* controlCtx;
		};
	}
}