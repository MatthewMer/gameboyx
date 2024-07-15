#include "BaseCTRL.h"
#include "GameboyCTRL.h"

#include "logger.h"

namespace Emulation {
	std::weak_ptr<BaseCTRL> BaseCTRL::m_Instance;

	std::shared_ptr<BaseCTRL> BaseCTRL::s_GetInstance(std::shared_ptr<BaseCartridge> _cartridge) {
		std::shared_ptr<BaseCTRL> ptr = m_Instance.lock();

		if (!ptr) {
			switch (_cartridge->console) {
			case GB:
			case GBC:
				ptr = std::static_pointer_cast<BaseCTRL>(std::make_shared<Gameboy::GameboyCTRL>(_cartridge));
				break;
			}

			m_Instance = ptr;
		}

		return ptr;
	}

	std::shared_ptr<BaseCTRL> BaseCTRL::s_GetInstance() {
		return m_Instance.lock();
	}

	void BaseCTRL::s_ResetInstance() {
		if (m_Instance.lock()) {
			m_Instance.reset();
		}
	}
}