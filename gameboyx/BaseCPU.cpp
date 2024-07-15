#include "BaseCPU.h"
#include "GameboyCPU.h"

#include "logger.h"

#include "general_config.h"

namespace Emulation {
	std::weak_ptr<BaseCPU> BaseCPU::m_Instance;

	std::shared_ptr<BaseCPU> BaseCPU::s_GetInstance(std::shared_ptr<BaseCartridge> _cartridge) {
		std::shared_ptr<BaseCPU> ptr = m_Instance.lock();

		if (!ptr) {
			switch (_cartridge->console) {
			case GB:
			case GBC:
				ptr = std::static_pointer_cast<BaseCPU>(std::make_shared<Gameboy::GameboyCPU>(_cartridge));
				break;
			}

			m_Instance = ptr;
		}

		return ptr;
	}

	std::shared_ptr<BaseCPU> BaseCPU::s_GetInstance() {
		return m_Instance.lock();
	}

	void BaseCPU::s_ResetInstance() {
		if (m_Instance.lock()) {
			m_Instance.reset();
		}
	}

	int BaseCPU::GetClockCycles() const {
		return tickCounter;
	}

	void BaseCPU::ResetClockCycles() {
		tickCounter = 0;
	}

	assembly_tables& BaseCPU::GetAssemblyTables() {
		return asmTables;
	}
}