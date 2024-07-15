#include "BaseGPU.h"

#include "GameboyGPU.h"
#include "logger.h"

namespace Emulation {
	std::weak_ptr<BaseGPU> BaseGPU::m_Instance;

	std::shared_ptr<BaseGPU> BaseGPU::s_GetInstance(std::shared_ptr<BaseCartridge> _cartridge) {
		std::shared_ptr<BaseGPU> ptr = m_Instance.lock();

		if (!ptr) {
			switch (_cartridge->console) {
			case GB:
			case GBC:
				ptr = std::static_pointer_cast<BaseGPU>(std::make_shared<Gameboy::GameboyGPU>(_cartridge));
				break;
			}

			m_Instance = ptr;
		}

		return ptr;
	}

	std::shared_ptr<BaseGPU> BaseGPU::s_GetInstance() {
		return m_Instance.lock();
	}

	void BaseGPU::s_ResetInstance() {
		if (m_Instance.lock()) {
			m_Instance.reset();
		}
	}

	int BaseGPU::GetFrameCount() const {
		return frameCounter;
	}

	void BaseGPU::ResetFrameCount() {
		frameCounter = 0;
	}
}