#include "BaseAPU.h"
#include "GameboyAPU.h"

#include "logger.h"

namespace Emulation {
	std::weak_ptr<BaseAPU> BaseAPU::m_Instance;

	std::shared_ptr<BaseAPU> BaseAPU::s_GetInstance(std::shared_ptr<BaseCartridge> _cartridge) {
		std::shared_ptr<BaseAPU> ptr = m_Instance.lock();

		if (!ptr) {
			switch (_cartridge->console) {
			case GB:
			case GBC:
				ptr = std::static_pointer_cast<BaseAPU>(std::make_shared<Gameboy::GameboyAPU>(_cartridge));
				break;
			}

			m_Instance = ptr;
		}

		return ptr;
	}

	std::shared_ptr<BaseAPU> BaseAPU::s_GetInstance() {
		return m_Instance.lock();
	}

	void BaseAPU::s_ResetInstance() {
		if (m_Instance.lock()) {
			m_Instance.reset();
		}
	}

	int BaseAPU::GetChunkId() const {
		return m_audioChunkId;
	}

	void BaseAPU::NextChunk() {
		++m_audioChunkId;
	}
}