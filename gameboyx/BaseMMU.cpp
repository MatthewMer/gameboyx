#include "BaseMMU.h"

#include "GameboyMMU.h"
#include "logger.h"

#include <vector>

using namespace std;

namespace Emulation {
	/* ***********************************************************************************************************
		MMU BASE CLASS
	*********************************************************************************************************** */
	std::weak_ptr<BaseMMU> BaseMMU::m_Instance;

	std::shared_ptr<BaseMMU> BaseMMU::s_GetInstance(std::shared_ptr<BaseCartridge> _cartridge) {
		std::shared_ptr<BaseMMU> ptr = m_Instance.lock();

		if (!ptr) {
			switch (_cartridge->console) {
			case GB:
			case GBC:
				ptr = std::static_pointer_cast<BaseMMU>(Gameboy::GameboyMMU::s_GetInstance(_cartridge));
				break;
			}

			m_Instance = ptr;
		}

		return ptr;
	}

	std::shared_ptr<BaseMMU> BaseMMU::s_GetInstance() {
		return m_Instance.lock();
	}

	void BaseMMU::s_ResetInstance() {
		if (m_Instance.lock()) {
			m_Instance.reset();
		}
	}
}