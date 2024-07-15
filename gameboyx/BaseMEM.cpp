#include "BaseMEM.h"
#include "GameboyMEM.h"

namespace Emulation {
    std::weak_ptr<BaseMEM> BaseMEM::m_Instance;

    std::shared_ptr<BaseMEM> BaseMEM::s_GetInstance(std::shared_ptr<BaseCartridge> _cartridge) {
        std::shared_ptr<BaseMEM> ptr = m_Instance.lock();

        if (!ptr) {
            switch (_cartridge->console) {
            case GB:
            case GBC:
                ptr = std::static_pointer_cast<BaseMEM>(std::make_shared<Gameboy::GameboyMEM>(_cartridge));
                break;
            }

            m_Instance = ptr;
        }

        return ptr;
    }

    std::shared_ptr<BaseMEM> BaseMEM::s_GetInstance() {
        return m_Instance.lock();
    }

    void BaseMEM::s_ResetInstance() {
        if (m_Instance.lock()) {
            m_Instance.reset();
        }
    }

    std::vector<memory_type_tables>& BaseMEM::GetMemoryTables() {
        return memoryTables;
    }
}