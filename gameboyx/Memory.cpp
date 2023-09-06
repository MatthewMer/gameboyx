#include "Memory.h"

Memory* Memory::instance = nullptr;

Memory* Memory::getInstance(const Cartridge& _cart_obj) {
    if (instance == nullptr) {
        instance = new Memory(_cart_obj);
    }

    return instance;
}

void Memory::resetInstance() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

constexpr Memory::Memory(const Cartridge& _cart_obj) {

}