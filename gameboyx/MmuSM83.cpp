#include "MmuSM83.h"

MmuSM83_MBC3::MmuSM83_MBC3(const Cartridge& _cart_obj) {
	Memory::resetInstance();
	mem_instance = Memory::getInstance(_cart_obj);
}

void MmuSM83_MBC3::Write8Bit(const u8& _data, const u16& _addr) {

}

void MmuSM83_MBC3::Write16Bit(const u16& _data, const u16& _addr) {

}

u8 MmuSM83_MBC3::Read8Bit(const u16& _addr) {
	return 0x00;
}

u16 MmuSM83_MBC3::Read16Bit(const u16& _addr) {
	return 0x00;
}