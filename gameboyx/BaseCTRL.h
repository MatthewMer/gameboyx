#pragma once
#include "HardwareMgr.h"

#include "BaseCartridge.h"

#include <SDL.h>

namespace Emulation {
	class BaseCTRL {
	public:
		// get/reset instance
		static std::shared_ptr<BaseCTRL> s_GetInstance(std::shared_ptr<BaseCartridge> _cartridge);
		static std::shared_ptr<BaseCTRL> s_GetInstance();
		static void s_ResetInstance();
		virtual void Init() = 0;

		// clone/assign protection
		BaseCTRL(BaseCTRL const&) = delete;
		BaseCTRL(BaseCTRL&&) = delete;
		BaseCTRL& operator=(BaseCTRL const&) = delete;
		BaseCTRL& operator=(BaseCTRL&&) = delete;

		// public members
		virtual bool SetKey(const int& _player, const SDL_GameControllerButton& _key) = 0;
		virtual bool ResetKey(const int& _player, const SDL_GameControllerButton& _key) = 0;

	protected:
		// constructor
		BaseCTRL() = default;
		virtual ~BaseCTRL() {}

	private:
		static std::weak_ptr<BaseCTRL> m_Instance;
	};
}