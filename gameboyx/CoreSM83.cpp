#include "CoreSM83.h"

/* ***********************************************************************************************************
    INCLUDES
*********************************************************************************************************** */
#include "gameboy_config.h"

#include "logger.h"

using namespace std;

/* ***********************************************************************************************************
    FLAG/BIT DEFINES
*********************************************************************************************************** */
#define FLAG_CARRY			0x10
#define FLAG_HCARRY			0x20
#define FLAG_SUB			0x40
#define FLAG_ZERO			0x80

#define MSB                 0x80
#define LSB                 0x01

/* ***********************************************************************************************************
    FLAG SET DEFINES
*********************************************************************************************************** */
#define RESET_FLAGS(flags, f) {f &= ~(flags);}
#define SET_FLAGS(flags, f) {f |= (flags);}
#define RESET_ALL_FLAGS(f) {f = 0x00;}

#define ADD_8_FLAGS(d, s, f) {f &= ~(FLAG_CARRY | FLAG_HCARRY); f |= ((u16)(d & 0xFF) + (s & 0xFF) > 0xFF ? FLAG_CARRY : 0); f |= ((d & 0xF) + (s & 0xF) > 0xF ? FLAG_HCARRY : 0);}
#define ADD_8_C(d, s, f) {f &= ~FLAG_CARRY; f |= ((u16)(d & 0xFF) + (s & 0xFF) > 0xFF ? FLAG_CARRY : 0);}
#define ADD_8_HC(d, s, f) {f &= ~FLAG_HCARRY; f |= ((d & 0xF) + (s & 0xF) > 0xF ? FLAG_HCARRY : 0);}

#define ADC_8_C(d, s, c, f) {f &= ~FLAG_CARRY; f |= ((u16)(d & 0xFF) + (u16)(s & 0xFF) + c > 0xFF ? FLAG_CARRY : 0);}
#define ADC_8_HC(d, s, c, f) {f &= ~FLAG_HCARRY; f |= ((d & 0xF) + (s & 0xF) + c > 0xF ? FLAG_HCARRY : 0);}

#define ADD_16_FLAGS(d, s, f) {f &= ~(FLAG_CARRY | FLAG_HCARRY); f |= ((u32)(d & 0xFFFF) + (s & 0xFFFF) > 0xFFFF ? FLAG_CARRY : 0); f |= ((u16)(d & 0xFFF) + (s & 0xFFF) > 0xFFF ? FLAG_HCARRY : 0);}

#define SUB_8_FLAGS(d, s, f) {f &= ~(FLAG_CARRY | FLAG_HCARRY); f |= ((u16)(d & 0xFF) - (s & 0xFF) > 0xFF ? FLAG_CARRY : 0); f |= ((d & 0xF) - (s & 0xF) > 0xF ? FLAG_HCARRY : 0);}
#define SUB_8_C(d, s, f) {f &= ~FLAG_CARRY; f |= ((u16)(d & 0xFF) - (s & 0xFF) > 0xFF ? FLAG_CARRY : 0);}
#define SUB_8_HC(d, s, f) {f &= ~FLAG_HCARRY; f|= ((d & 0xF) - (s & 0xF) > 0xF ? FLAG_HCARRY : 0);}

#define SBC_8_C(d, s, c, f) {f &= ~FLAG_CARRY; f |= ((d & 0xFF) - (((u16)s & 0xFF) + c) > 0xFF ? FLAG_CARRY : 0);}
#define SBC_8_HC(d, s, c, f) {f &= ~FLAG_HCARRY; f |= ((d & 0xF) - ((s & 0xF) + c) > 0xF ? FLAG_HCARRY : 0);}

#define ZERO_FLAG(x, f) {f &= ~FLAG_ZERO; f |= (x == 0 ? FLAG_ZERO : 0);}

/* ***********************************************************************************************************
    CONSTRUCTOR
*********************************************************************************************************** */
CoreSM83::CoreSM83(const Cartridge& _cart_obj) {
	Mmu::resetInstance();
	mmu_instance = Mmu::getInstance(_cart_obj);
}

/* ***********************************************************************************************************
*
*   GAMEBOY (COLOR) CORESM83 INSTRUCTION LOOKUP TABLE
*
*********************************************************************************************************** */
void CoreSM83::setupLookupTable() {
    instrMap.clear();

    // Elements: opcode, instruction function, machine cycles

    // 0x00
    instrMap.emplace_back(0x00, &CoreSM83::NOP, 1);
    instrMap.emplace_back(0x01, &CoreSM83::LDd16, 3);
    instrMap.emplace_back(0x02, &CoreSM83::LDfromAtoRef, 2);
    instrMap.emplace_back(0x03, &CoreSM83::INC16, 2);
    instrMap.emplace_back(0x04, &CoreSM83::INC8, 1);
    instrMap.emplace_back(0x05, &CoreSM83::DEC8, 1);
    instrMap.emplace_back(0x06, &CoreSM83::LDd8, 2);
    instrMap.emplace_back(0x07, &CoreSM83::RLCA, 1);
    instrMap.emplace_back(0x08, &CoreSM83::LDSPa16, 5);
    instrMap.emplace_back(0x09, &CoreSM83::ADDHL, 2);
    instrMap.emplace_back(0x0a, &CoreSM83::LDtoAfromRef, 2);
    instrMap.emplace_back(0x0b, &CoreSM83::DEC16, 2);
    instrMap.emplace_back(0x0c, &CoreSM83::INC8, 1);
    instrMap.emplace_back(0x0d, &CoreSM83::DEC8, 1);
    instrMap.emplace_back(0x0e, &CoreSM83::LDd8, 2);
    instrMap.emplace_back(0x0f, &CoreSM83::RRCA, 1);

    // 0x10
    instrMap.emplace_back(0x10, &CoreSM83::STOP, 1);
    instrMap.emplace_back(0x11, &CoreSM83::LDd16, 3);
    instrMap.emplace_back(0x12, &CoreSM83::LDfromAtoRef, 2);
    instrMap.emplace_back(0x13, &CoreSM83::INC16, 2);
    instrMap.emplace_back(0x14, &CoreSM83::INC8, 1);
    instrMap.emplace_back(0x15, &CoreSM83::DEC8, 1);
    instrMap.emplace_back(0x16, &CoreSM83::LDd8, 2);
    instrMap.emplace_back(0x17, &CoreSM83::RLA, 1);
    instrMap.emplace_back(0x18, &CoreSM83::JR, 3);
    instrMap.emplace_back(0x19, &CoreSM83::ADDHL, 2);
    instrMap.emplace_back(0x1a, &CoreSM83::LDtoAfromRef, 2);
    instrMap.emplace_back(0x1b, &CoreSM83::DEC16, 2);
    instrMap.emplace_back(0x1c, &CoreSM83::INC8, 1);
    instrMap.emplace_back(0x1d, &CoreSM83::DEC8, 1);
    instrMap.emplace_back(0x1e, &CoreSM83::LDd8, 2);
    instrMap.emplace_back(0x1f, &CoreSM83::RRA, 1);

    // 0x20
    instrMap.emplace_back(0x20, &CoreSM83::JR, 0);
    instrMap.emplace_back(0x21, &CoreSM83::LDd16, 3);
    instrMap.emplace_back(0x22, &CoreSM83::LDfromAtoRef, 2);
    instrMap.emplace_back(0x23, &CoreSM83::INC16, 2);
    instrMap.emplace_back(0x24, &CoreSM83::INC8, 1);
    instrMap.emplace_back(0x25, &CoreSM83::DEC8, 1);
    instrMap.emplace_back(0x26, &CoreSM83::LDd8, 2);
    instrMap.emplace_back(0x27, &CoreSM83::DAA, 1);
    instrMap.emplace_back(0x28, &CoreSM83::JR, 0);
    instrMap.emplace_back(0x29, &CoreSM83::ADDHL, 2);
    instrMap.emplace_back(0x2a, &CoreSM83::LDtoAfromRef, 2);
    instrMap.emplace_back(0x2b, &CoreSM83::DEC16, 2);
    instrMap.emplace_back(0x2c, &CoreSM83::INC8, 1);
    instrMap.emplace_back(0x2d, &CoreSM83::DEC8, 1);
    instrMap.emplace_back(0x2e, &CoreSM83::LDd8, 2);
    instrMap.emplace_back(0x2f, &CoreSM83::CPL, 1);

    // 0x30
    instrMap.emplace_back(0x30, &CoreSM83::JR, 0);
    instrMap.emplace_back(0x31, &CoreSM83::LDd16, 3);
    instrMap.emplace_back(0x32, &CoreSM83::LDfromAtoRef, 2);
    instrMap.emplace_back(0x33, &CoreSM83::INC16, 2);
    instrMap.emplace_back(0x34, &CoreSM83::INC8, 1);
    instrMap.emplace_back(0x35, &CoreSM83::DEC8, 1);
    instrMap.emplace_back(0x36, &CoreSM83::LDd8, 2);
    instrMap.emplace_back(0x37, &CoreSM83::SCF, 1);
    instrMap.emplace_back(0x38, &CoreSM83::JR, 0);
    instrMap.emplace_back(0x39, &CoreSM83::ADDHL, 2);
    instrMap.emplace_back(0x3a, &CoreSM83::LDtoAfromRef, 2);
    instrMap.emplace_back(0x3b, &CoreSM83::DEC16, 2);
    instrMap.emplace_back(0x3c, &CoreSM83::INC8, 1);
    instrMap.emplace_back(0x3d, &CoreSM83::DEC8, 1);
    instrMap.emplace_back(0x3e, &CoreSM83::LDd8, 2);
    instrMap.emplace_back(0x3f, &CoreSM83::CCF, 1);

    // 0x40
    instrMap.emplace_back(0x40, &CoreSM83::LDtoB, 1);
    instrMap.emplace_back(0x41, &CoreSM83::LDtoB, 1);
    instrMap.emplace_back(0x42, &CoreSM83::LDtoB, 1);
    instrMap.emplace_back(0x43, &CoreSM83::LDtoB, 1);
    instrMap.emplace_back(0x44, &CoreSM83::LDtoB, 1);
    instrMap.emplace_back(0x45, &CoreSM83::LDtoB, 1);
    instrMap.emplace_back(0x46, &CoreSM83::LDtoB, 2);
    instrMap.emplace_back(0x47, &CoreSM83::LDtoB, 1);
    instrMap.emplace_back(0x48, &CoreSM83::LDtoC, 1);
    instrMap.emplace_back(0x49, &CoreSM83::LDtoC, 1);
    instrMap.emplace_back(0x4a, &CoreSM83::LDtoC, 1);
    instrMap.emplace_back(0x4b, &CoreSM83::LDtoC, 1);
    instrMap.emplace_back(0x4c, &CoreSM83::LDtoC, 1);
    instrMap.emplace_back(0x4d, &CoreSM83::LDtoC, 1);
    instrMap.emplace_back(0x4e, &CoreSM83::LDtoC, 2);
    instrMap.emplace_back(0x4f, &CoreSM83::LDtoC, 1);

    // 0x50
    instrMap.emplace_back(0x50, &CoreSM83::LDtoD, 1);
    instrMap.emplace_back(0x51, &CoreSM83::LDtoD, 1);
    instrMap.emplace_back(0x52, &CoreSM83::LDtoD, 1);
    instrMap.emplace_back(0x53, &CoreSM83::LDtoD, 1);
    instrMap.emplace_back(0x54, &CoreSM83::LDtoD, 1);
    instrMap.emplace_back(0x55, &CoreSM83::LDtoD, 1);
    instrMap.emplace_back(0x56, &CoreSM83::LDtoD, 2);
    instrMap.emplace_back(0x57, &CoreSM83::LDtoD, 1);
    instrMap.emplace_back(0x58, &CoreSM83::LDtoE, 1);
    instrMap.emplace_back(0x59, &CoreSM83::LDtoE, 1);
    instrMap.emplace_back(0x5a, &CoreSM83::LDtoE, 1);
    instrMap.emplace_back(0x5b, &CoreSM83::LDtoE, 1);
    instrMap.emplace_back(0x5c, &CoreSM83::LDtoE, 1);
    instrMap.emplace_back(0x5d, &CoreSM83::LDtoE, 1);
    instrMap.emplace_back(0x5e, &CoreSM83::LDtoE, 2);
    instrMap.emplace_back(0x5f, &CoreSM83::LDtoE, 1);

    // 0x60
    instrMap.emplace_back(0x60, &CoreSM83::LDtoH, 1);
    instrMap.emplace_back(0x61, &CoreSM83::LDtoH, 1);
    instrMap.emplace_back(0x62, &CoreSM83::LDtoH, 1);
    instrMap.emplace_back(0x63, &CoreSM83::LDtoH, 1);
    instrMap.emplace_back(0x64, &CoreSM83::LDtoH, 1);
    instrMap.emplace_back(0x65, &CoreSM83::LDtoH, 1);
    instrMap.emplace_back(0x66, &CoreSM83::LDtoH, 2);
    instrMap.emplace_back(0x67, &CoreSM83::LDtoH, 1);
    instrMap.emplace_back(0x68, &CoreSM83::LDtoL, 1);
    instrMap.emplace_back(0x69, &CoreSM83::LDtoL, 1);
    instrMap.emplace_back(0x6a, &CoreSM83::LDtoL, 1);
    instrMap.emplace_back(0x6b, &CoreSM83::LDtoL, 1);
    instrMap.emplace_back(0x6c, &CoreSM83::LDtoL, 1);
    instrMap.emplace_back(0x6d, &CoreSM83::LDtoL, 1);
    instrMap.emplace_back(0x6e, &CoreSM83::LDtoL, 2);
    instrMap.emplace_back(0x6f, &CoreSM83::LDtoL, 1);

    // 0x70
    instrMap.emplace_back(0x70, &CoreSM83::LDtoHLref, 2);
    instrMap.emplace_back(0x71, &CoreSM83::LDtoHLref, 2);
    instrMap.emplace_back(0x72, &CoreSM83::LDtoHLref, 2);
    instrMap.emplace_back(0x73, &CoreSM83::LDtoHLref, 2);
    instrMap.emplace_back(0x74, &CoreSM83::LDtoHLref, 2);
    instrMap.emplace_back(0x75, &CoreSM83::LDtoHLref, 2);
    instrMap.emplace_back(0x76, &CoreSM83::HALT, 1);
    instrMap.emplace_back(0x77, &CoreSM83::LDtoHLref, 2);
    instrMap.emplace_back(0x78, &CoreSM83::LDtoA, 1);
    instrMap.emplace_back(0x79, &CoreSM83::LDtoA, 1);
    instrMap.emplace_back(0x7a, &CoreSM83::LDtoA, 1);
    instrMap.emplace_back(0x7b, &CoreSM83::LDtoA, 1);
    instrMap.emplace_back(0x7c, &CoreSM83::LDtoA, 1);
    instrMap.emplace_back(0x7d, &CoreSM83::LDtoA, 1);
    instrMap.emplace_back(0x7e, &CoreSM83::LDtoA, 2);
    instrMap.emplace_back(0x7f, &CoreSM83::LDtoA, 1);

    // 0x80
    instrMap.emplace_back(0x80, &CoreSM83::ADD8, 1);
    instrMap.emplace_back(0x81, &CoreSM83::ADD8, 1);
    instrMap.emplace_back(0x82, &CoreSM83::ADD8, 1);
    instrMap.emplace_back(0x83, &CoreSM83::ADD8, 1);
    instrMap.emplace_back(0x84, &CoreSM83::ADD8, 1);
    instrMap.emplace_back(0x85, &CoreSM83::ADD8, 1);
    instrMap.emplace_back(0x86, &CoreSM83::ADD8, 2);
    instrMap.emplace_back(0x87, &CoreSM83::ADD8, 1);
    instrMap.emplace_back(0x88, &CoreSM83::ADC, 1);
    instrMap.emplace_back(0x89, &CoreSM83::ADC, 1);
    instrMap.emplace_back(0x8a, &CoreSM83::ADC, 1);
    instrMap.emplace_back(0x8b, &CoreSM83::ADC, 1);
    instrMap.emplace_back(0x8c, &CoreSM83::ADC, 1);
    instrMap.emplace_back(0x8d, &CoreSM83::ADC, 1);
    instrMap.emplace_back(0x8e, &CoreSM83::ADC, 2);
    instrMap.emplace_back(0x8f, &CoreSM83::ADC, 1);

    // 0x90
    instrMap.emplace_back(0x90, &CoreSM83::SUB, 1);
    instrMap.emplace_back(0x91, &CoreSM83::SUB, 1);
    instrMap.emplace_back(0x92, &CoreSM83::SUB, 1);
    instrMap.emplace_back(0x93, &CoreSM83::SUB, 1);
    instrMap.emplace_back(0x94, &CoreSM83::SUB, 1);
    instrMap.emplace_back(0x95, &CoreSM83::SUB, 1);
    instrMap.emplace_back(0x96, &CoreSM83::SUB, 1);
    instrMap.emplace_back(0x97, &CoreSM83::SUB, 2);
    instrMap.emplace_back(0x98, &CoreSM83::SBC, 1);
    instrMap.emplace_back(0x99, &CoreSM83::SBC, 1);
    instrMap.emplace_back(0x9a, &CoreSM83::SBC, 1);
    instrMap.emplace_back(0x9b, &CoreSM83::SBC, 1);
    instrMap.emplace_back(0x9c, &CoreSM83::SBC, 1);
    instrMap.emplace_back(0x9d, &CoreSM83::SBC, 1);
    instrMap.emplace_back(0x9e, &CoreSM83::SBC, 2);
    instrMap.emplace_back(0x9f, &CoreSM83::SBC, 1);

    // 0xa0
    instrMap.emplace_back(0xa0, &CoreSM83::AND, 1);
    instrMap.emplace_back(0xa1, &CoreSM83::AND, 1);
    instrMap.emplace_back(0xa2, &CoreSM83::AND, 1);
    instrMap.emplace_back(0xa3, &CoreSM83::AND, 1);
    instrMap.emplace_back(0xa4, &CoreSM83::AND, 1);
    instrMap.emplace_back(0xa5, &CoreSM83::AND, 1);
    instrMap.emplace_back(0xa6, &CoreSM83::AND, 2);
    instrMap.emplace_back(0xa7, &CoreSM83::AND, 1);
    instrMap.emplace_back(0xa8, &CoreSM83::XOR, 1);
    instrMap.emplace_back(0xa9, &CoreSM83::XOR, 1);
    instrMap.emplace_back(0xaa, &CoreSM83::XOR, 1);
    instrMap.emplace_back(0xab, &CoreSM83::XOR, 1);
    instrMap.emplace_back(0xac, &CoreSM83::XOR, 1);
    instrMap.emplace_back(0xad, &CoreSM83::XOR, 1);
    instrMap.emplace_back(0xae, &CoreSM83::XOR, 2);
    instrMap.emplace_back(0xaf, &CoreSM83::XOR, 1);

    // 0xb0
    instrMap.emplace_back(0xb0, &CoreSM83::OR, 1);
    instrMap.emplace_back(0xb1, &CoreSM83::OR, 1);
    instrMap.emplace_back(0xb2, &CoreSM83::OR, 1);
    instrMap.emplace_back(0xb3, &CoreSM83::OR, 1);
    instrMap.emplace_back(0xb4, &CoreSM83::OR, 1);
    instrMap.emplace_back(0xb5, &CoreSM83::OR, 1);
    instrMap.emplace_back(0xb6, &CoreSM83::OR, 2);
    instrMap.emplace_back(0xb7, &CoreSM83::OR, 1);
    instrMap.emplace_back(0xb8, &CoreSM83::CP, 1);
    instrMap.emplace_back(0xb9, &CoreSM83::CP, 1);
    instrMap.emplace_back(0xba, &CoreSM83::CP, 1);
    instrMap.emplace_back(0xbb, &CoreSM83::CP, 1);
    instrMap.emplace_back(0xbc, &CoreSM83::CP, 1);
    instrMap.emplace_back(0xbd, &CoreSM83::CP, 1);
    instrMap.emplace_back(0xbe, &CoreSM83::CP, 2);
    instrMap.emplace_back(0xbf, &CoreSM83::CP, 1);

    // 0xc0
    instrMap.emplace_back(0xc0, &CoreSM83::RET, 0);
    instrMap.emplace_back(0xc1, &CoreSM83::POP, 3);
    instrMap.emplace_back(0xc2, &CoreSM83::JP, 0);
    instrMap.emplace_back(0xc3, &CoreSM83::JP, 4);
    instrMap.emplace_back(0xc4, &CoreSM83::CALL, 0);
    instrMap.emplace_back(0xc5, &CoreSM83::PUSH, 4);
    instrMap.emplace_back(0xc6, &CoreSM83::ADD8, 2);
    instrMap.emplace_back(0xc7, &CoreSM83::RST, 4);
    instrMap.emplace_back(0xc8, &CoreSM83::RET, 0);
    instrMap.emplace_back(0xc9, &CoreSM83::RET, 4);
    instrMap.emplace_back(0xca, &CoreSM83::JP, 0);
    instrMap.emplace_back(0xcb, &CoreSM83::CB, 1);
    instrMap.emplace_back(0xcc, &CoreSM83::CALL, 0);
    instrMap.emplace_back(0xcd, &CoreSM83::CALL, 6);
    instrMap.emplace_back(0xce, &CoreSM83::ADC, 2);
    instrMap.emplace_back(0xcf, &CoreSM83::RST, 4);

    // 0xd0
    instrMap.emplace_back(0xd0, &CoreSM83::RET, 0);
    instrMap.emplace_back(0xd1, &CoreSM83::POP, 3);
    instrMap.emplace_back(0xd2, &CoreSM83::JP, 0);
    instrMap.emplace_back(0xd3, &CoreSM83::NoInstruction, 0);
    instrMap.emplace_back(0xd4, &CoreSM83::CALL, 0);
    instrMap.emplace_back(0xd5, &CoreSM83::PUSH, 4);
    instrMap.emplace_back(0xd6, &CoreSM83::SUB, 2);
    instrMap.emplace_back(0xd7, &CoreSM83::RST, 4);
    instrMap.emplace_back(0xd8, &CoreSM83::RET, 0);
    instrMap.emplace_back(0xd9, &CoreSM83::RETI, 4);
    instrMap.emplace_back(0xda, &CoreSM83::JP, 0);
    instrMap.emplace_back(0xdb, &CoreSM83::NoInstruction, 0);
    instrMap.emplace_back(0xdc, &CoreSM83::CALL, 0);
    instrMap.emplace_back(0xdd, &CoreSM83::NoInstruction, 0);
    instrMap.emplace_back(0xde, &CoreSM83::SBC, 2);
    instrMap.emplace_back(0xdf, &CoreSM83::RST, 4);

    // 0xe0
    instrMap.emplace_back(0xe0, &CoreSM83::LDH, 3);
    instrMap.emplace_back(0xe1, &CoreSM83::POP, 3);
    instrMap.emplace_back(0xe2, &CoreSM83::LDCref, 2);
    instrMap.emplace_back(0xe3, &CoreSM83::NoInstruction, 0);
    instrMap.emplace_back(0xe4, &CoreSM83::NoInstruction, 0);
    instrMap.emplace_back(0xe5, &CoreSM83::PUSH, 4);
    instrMap.emplace_back(0xe6, &CoreSM83::AND, 2);
    instrMap.emplace_back(0xe7, &CoreSM83::RST, 4);
    instrMap.emplace_back(0xe8, &CoreSM83::ADDSPr8, 4);
    instrMap.emplace_back(0xe9, &CoreSM83::JP, 1);
    instrMap.emplace_back(0xea, &CoreSM83::LDHa16, 4);
    instrMap.emplace_back(0xeb, &CoreSM83::NoInstruction, 0);
    instrMap.emplace_back(0xec, &CoreSM83::NoInstruction, 0);
    instrMap.emplace_back(0xed, &CoreSM83::NoInstruction, 0);
    instrMap.emplace_back(0xee, &CoreSM83::CP, 2);
    instrMap.emplace_back(0xef, &CoreSM83::RST, 4);

    // 0xf0
    instrMap.emplace_back(0xf0, &CoreSM83::LDH, 3);
    instrMap.emplace_back(0xf1, &CoreSM83::POP, 3);
    instrMap.emplace_back(0xf2, &CoreSM83::LDCref, 2);
    instrMap.emplace_back(0xf3, &CoreSM83::DI, 1);
    instrMap.emplace_back(0xf4, &CoreSM83::NoInstruction, 0);
    instrMap.emplace_back(0xf5, &CoreSM83::PUSH, 4);
    instrMap.emplace_back(0xf6, &CoreSM83::OR, 2);
    instrMap.emplace_back(0xf7, &CoreSM83::RST, 4);
    instrMap.emplace_back(0xf8, &CoreSM83::LDHLSPr8, 3);
    instrMap.emplace_back(0xf9, &CoreSM83::LDSPHL, 2);
    instrMap.emplace_back(0xfa, &CoreSM83::LDHa16, 4);
    instrMap.emplace_back(0xfb, &CoreSM83::EI, 1);
    instrMap.emplace_back(0xfc, &CoreSM83::NoInstruction, 0);
    instrMap.emplace_back(0xfd, &CoreSM83::NoInstruction, 0);
    instrMap.emplace_back(0xfe, &CoreSM83::XOR, 2);
    instrMap.emplace_back(0xff, &CoreSM83::RST, 4);
}

/* ***********************************************************************************************************
*
*   GAMEBOY (COLOR) CORESM83 BASIC INSTRUCTION SET
*
*********************************************************************************************************** */

/* ***********************************************************************************************************
    CPU CONTROL
*********************************************************************************************************** */
// no instruction
void CoreSM83::NoInstruction(){
    LOG_ERROR("No instruction");
}

// no operation
void CoreSM83::NOP() {
    return;
}

// stop sm83_instruction
void CoreSM83::STOP() {
    data = mmu_instance->Read8Bit(Regs.PC);
    Regs.PC++;

    if (data) { LOG_ERROR("Wrong STOP opcode"); }

    // TODO
}

// set halt state
void CoreSM83::HALT() {
    halt = true;
}

// flip c
void CoreSM83::CCF() {
    RESET_FLAGS(FLAG_HCARRY | FLAG_SUB, Regs.F);
    Regs.F ^= FLAG_CARRY;
}

// set c
void CoreSM83::SCF() {
    RESET_FLAGS(FLAG_HCARRY | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_CARRY;
}

// disable interrupts
void CoreSM83::DI() {
    ime = false;
}

// eneable interrupts
void CoreSM83::EI() {
    ime = true;
}

// enable CB sm83_instructions for next opcode
void CoreSM83::CB() {
    opcode_cb = true;
}

/* ***********************************************************************************************************
    LOAD
*********************************************************************************************************** */
// load 8/16 bit
void CoreSM83::LDfromAtoRef() {
    switch (opcode) {
    case 0x02:
        mmu_instance->Write8Bit(Regs.A, Regs.BC);
        break;
    case 0x12:
        mmu_instance->Write8Bit(Regs.A, Regs.DE);
        break;
    case 0x22:
        mmu_instance->Write8Bit(Regs.A, Regs.HL);
        Regs.HL++;
        break;
    case 0x32:
        mmu_instance->Write8Bit(Regs.A, Regs.HL);
        Regs.HL--;
        break;
    }
}

void CoreSM83::LDtoAfromRef() {
    switch (opcode) {
    case 0x0A:
        Regs.A = mmu_instance->Read8Bit(Regs.BC);
        break;
    case 0x1A:
        Regs.A = mmu_instance->Read8Bit(Regs.DE);
        break;
    case 0x2A:
        Regs.A = mmu_instance->Read8Bit(Regs.HL);
        Regs.HL++;
        break;
    case 0x3A:
        Regs.A = mmu_instance->Read8Bit(Regs.HL);
        Regs.HL--;
        break;
    }
}

void CoreSM83::LDd8() {
    data = mmu_instance->Read8Bit(Regs.PC);
    Regs.PC++;

    switch (opcode) {
    case 0x06:
        Regs.BC_.B = data;
        break;
    case 0x16:
        Regs.DE_.D = data;
        break;
    case 0x26:
        Regs.HL_.H = data;
        break;
    case 0x36:
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x0E:
        Regs.BC_.C = data;
        break;
    case 0x1E:
        Regs.DE_.E = data;
        break;
    case 0x2E:
        Regs.HL_.L = data;
        break;
    case 0x3E:
        Regs.A = data;
        break;
    }
}

void CoreSM83::LDd16() {
    data = mmu_instance->Read16Bit(Regs.PC);
    Regs.PC += 2;

    switch (opcode) {
    case 0x01:
        Regs.BC = data;
        break;
    case 0x11:
        Regs.DE = data;
        break;
    case 0x21:
        Regs.HL = data;
        break;
    case 0x31:
        Regs.SP = data;
        break;
    }
}

void CoreSM83::LDCref() {
    switch (opcode) {
    case 0xE2:
        mmu_instance->Write8Bit(Regs.A, Regs.BC_.C | 0xFF00);
        break;
    case 0xF2:
        Regs.A = mmu_instance->Read8Bit(Regs.BC_.C | 0xFF00);
        break;
    }
}

void CoreSM83::LDH() {
    data = mmu_instance->Read8Bit(Regs.PC);
    Regs.PC++;

    switch (opcode) {
    case 0xE0:
        mmu_instance->Write8Bit(Regs.A, (data & 0xFF) | 0xFF00);
        break;
    case 0xF0:
        Regs.A = mmu_instance->Read8Bit((data & 0xFF) | 0xFF00);
        break;
    }
}

void CoreSM83::LDHa16() {
    data = mmu_instance->Read16Bit(Regs.PC);
    Regs.PC += 2;

    switch (opcode) {
    case 0xea:
        mmu_instance->Write8Bit(Regs.A, data);
        break;
    case 0xfa:
        Regs.A = mmu_instance->Read8Bit(data);
        break;
    }
}

void CoreSM83::LDSPa16() {
    data = mmu_instance->Read16Bit(Regs.PC);
    Regs.PC += 2;

    mmu_instance->Write16Bit(Regs.SP, data);
}

void CoreSM83::LDtoB() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B = Regs.BC_.B;
        break;
    case 0x01:
        Regs.BC_.B = Regs.BC_.C;
        break;
    case 0x02:
        Regs.BC_.B = Regs.DE_.D;
        break;
    case 0x03:
        Regs.BC_.B = Regs.DE_.E;
        break;
    case 0x04:
        Regs.BC_.B = Regs.HL_.H;
        break;
    case 0x05:
        Regs.BC_.B = Regs.HL_.L;
        break; 
    case 0x06:
        Regs.BC_.B = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.BC_.B = Regs.A;
        break;
    }
}

void CoreSM83::LDtoC() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.C = Regs.BC_.B;
        break;
    case 0x01:
        Regs.BC_.C = Regs.BC_.C;
        break;
    case 0x02:
        Regs.BC_.C = Regs.DE_.D;
        break;
    case 0x03:
        Regs.BC_.C = Regs.DE_.E;
        break;
    case 0x04:
        Regs.BC_.C = Regs.HL_.H;
        break;
    case 0x05:
        Regs.BC_.C = Regs.HL_.L;
        break;
    case 0x06:
        Regs.BC_.C = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.BC_.C = Regs.A;
        break;
    }
}

void CoreSM83::LDtoD() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.DE_.D = Regs.BC_.B;
        break;
    case 0x01:
        Regs.DE_.D = Regs.BC_.C;
        break;
    case 0x02:
        Regs.DE_.D = Regs.DE_.D;
        break;
    case 0x03:
        Regs.DE_.D = Regs.DE_.E;
        break;
    case 0x04:
        Regs.DE_.D = Regs.HL_.H;
        break;
    case 0x05:
        Regs.DE_.D = Regs.HL_.L;
        break;
    case 0x06:
        Regs.DE_.D = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.DE_.D = Regs.A;
        break;
    }
}

void CoreSM83::LDtoE() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.DE_.E = Regs.BC_.B;
        break;
    case 0x01:
        Regs.DE_.E = Regs.BC_.C;
        break;
    case 0x02:
        Regs.DE_.E = Regs.DE_.D;
        break;
    case 0x03:
        Regs.DE_.E = Regs.DE_.E;
        break;
    case 0x04:
        Regs.DE_.E = Regs.HL_.H;
        break;
    case 0x05:
        Regs.DE_.E = Regs.HL_.L;
        break;
    case 0x06:
        Regs.DE_.E = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.DE_.E = Regs.A;
        break;
    }
}

void CoreSM83::LDtoH() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.HL_.H = Regs.BC_.B;
        break;
    case 0x01:
        Regs.HL_.H = Regs.BC_.C;
        break;
    case 0x02:
        Regs.HL_.H = Regs.DE_.D;
        break;
    case 0x03:
        Regs.HL_.H = Regs.DE_.E;
        break;
    case 0x04:
        Regs.HL_.H = Regs.HL_.H;
        break;
    case 0x05:
        Regs.HL_.H = Regs.HL_.L;
        break;
    case 0x06:
        Regs.HL_.H = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.HL_.H = Regs.A;
        break;
    }
}

void CoreSM83::LDtoL() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.HL_.L = Regs.BC_.B;
        break;
    case 0x01:
        Regs.HL_.L = Regs.BC_.C;
        break;
    case 0x02:
        Regs.HL_.L = Regs.DE_.D;
        break;
    case 0x03:
        Regs.HL_.L = Regs.DE_.E;
        break;
    case 0x04:
        Regs.HL_.L = Regs.HL_.H;
        break;
    case 0x05:
        Regs.HL_.L = Regs.HL_.L;
        break;
    case 0x06:
        Regs.HL_.L = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.HL_.L = Regs.A;
        break;
    }
}

void CoreSM83::LDtoHLref() {
    switch (opcode & 0x07) {
    case 0x00:
        mmu_instance->Write8Bit(Regs.BC_.B, Regs.HL);
        break;
    case 0x01:
        mmu_instance->Write8Bit(Regs.BC_.C, Regs.HL);
        break;
    case 0x02:
        mmu_instance->Write8Bit(Regs.DE_.D, Regs.HL);
        break;
    case 0x03:
        mmu_instance->Write8Bit(Regs.DE_.E, Regs.HL);
        break;
    case 0x04:
        mmu_instance->Write8Bit(Regs.HL_.H, Regs.HL);
        break;
    case 0x05:
        mmu_instance->Write8Bit(Regs.HL_.L, Regs.HL);
        break;
    case 0x07:
        mmu_instance->Write8Bit(Regs.A, Regs.HL);
        break;
    }
}

void CoreSM83::LDtoA() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.A = Regs.BC_.B;
        break;
    case 0x01:
        Regs.A = Regs.BC_.C;
        break;
    case 0x02:
        Regs.A = Regs.DE_.D;
        break;
    case 0x03:
        Regs.A = Regs.DE_.E;
        break;
    case 0x04:
        Regs.A = Regs.HL_.H;
        break;
    case 0x05:
        Regs.A = Regs.HL_.L;
        break;
    case 0x06:
        Regs.A = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x07:
        Regs.A = Regs.A;
        break;
    }
}

// ld SP to HL and add signed 8 bit immediate data
void CoreSM83::LDHLSPr8() {
    data = mmu_instance->Read8Bit(Regs.PC);
    Regs.PC++;

    Regs.HL = Regs.SP;
    RESET_ALL_FLAGS(Regs.F);
    ADD_8_FLAGS(Regs.HL, data, Regs.F);
    Regs.HL += *(i8*)&data;
}

void CoreSM83::LDSPHL() {
    Regs.SP = Regs.HL;
}

// push PC to Stack
void CoreSM83::PUSH() {
    Regs.SP -= 2;

    switch (opcode) {
    case 0xc5:
        mmu_instance->Write16Bit(Regs.BC, Regs.SP);
        break;
    case 0xd5:
        mmu_instance->Write16Bit(Regs.DE, Regs.SP);
        break;
    case 0xe5:
        mmu_instance->Write16Bit(Regs.HL, Regs.SP);
        break;
    case 0xf5:
        data = ((u16)Regs.A << 8) | Regs.F;
        mmu_instance->Write16Bit(data, Regs.SP);
        break;
    }
}

// pop PC from Stack
void CoreSM83::POP() {
    switch (opcode) {
    case 0xc5:
        Regs.BC = mmu_instance->Read16Bit(Regs.SP);
        break;
    case 0xd5:
        Regs.DE = mmu_instance->Read16Bit(Regs.SP);
        break;
    case 0xe5:
        Regs.HL = mmu_instance->Read16Bit(Regs.SP);
        break;
    case 0xf5:
        data = mmu_instance->Read16Bit(Regs.SP);
        Regs.A = (data & 0xFF00) >> 8;
        Regs.F = data & 0xFF;
        break;
    }

    Regs.SP += 2;
}

/* ***********************************************************************************************************
    ARITHMETIC/LOGIC INSTRUCTIONS
*********************************************************************************************************** */
// increment 8/16 bit
void CoreSM83::INC8() {
    RESET_FLAGS(FLAG_ZERO | FLAG_SUB | FLAG_HCARRY, Regs.F);

    switch (opcode) {
    case 0x04:
        ADD_8_HC(Regs.BC_.B, 1, Regs.F);
        Regs.BC_.B += 1;
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x0c:
        ADD_8_HC(Regs.BC_.C, 1, Regs.F);
        Regs.BC_.C += 1;
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x14:
        ADD_8_HC(Regs.DE_.D, 1, Regs.F);
        Regs.DE_.D += 1;
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x1c:
        ADD_8_HC(Regs.DE_.E, 1, Regs.F);
        Regs.DE_.E += 1;
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x24:
        ADD_8_HC(Regs.HL_.H, 1, Regs.F);
        Regs.HL_.H += 1;
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x2c:
        ADD_8_HC(Regs.HL_.L, 1, Regs.F);
        Regs.HL_.L += 1;
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x34:
        data = mmu_instance->Read8Bit(Regs.HL);
        ADD_8_HC(data, 1, Regs.F);
        data += 1;
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x3c:
        ADD_8_HC(Regs.A, 1, Regs.F);
        Regs.A += 1;
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

void CoreSM83::INC16() {
    switch (opcode) {
    case 0x03:
        Regs.BC += 1;
        break;
    case 0x13:
        Regs.DE += 1;
        break;
    case 0x23:
        Regs.HL += 1;
        break;
    case 0x33:
        Regs.SP += 1;
        break;
    }
}

// decrement 8/16 bit
void CoreSM83::DEC8() {
    RESET_FLAGS(FLAG_ZERO | FLAG_HCARRY, Regs.F);
    SET_FLAGS(FLAG_SUB, Regs.F);

    switch (opcode) {
    case 0x04:
        SUB_8_HC(Regs.BC_.B, 1, Regs.F);
        Regs.BC_.B -= 1;
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x0c:
        SUB_8_HC(Regs.BC_.C, 1, Regs.F);
        Regs.BC_.C -= 1;
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x14:
        SUB_8_HC(Regs.DE_.D, 1, Regs.F);
        Regs.DE_.D -= 1;
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x1c:
        SUB_8_HC(Regs.DE_.E, 1, Regs.F);
        Regs.DE_.E -= 1;
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x24:
        SUB_8_HC(Regs.HL_.H, 1, Regs.F);
        Regs.HL_.H -= 1;
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x2c:
        SUB_8_HC(Regs.HL_.L, 1, Regs.F);
        Regs.HL_.L -= 1;
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x34:
        data = mmu_instance->Read8Bit(Regs.HL);
        SUB_8_HC(data, 1, Regs.F);
        data -= 1;
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x3c:
        SUB_8_HC(Regs.A, 1, Regs.F);
        Regs.A -= 1;
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

void CoreSM83::DEC16() {
    switch (opcode) {
    case 0x03:
        Regs.BC -= 1;
        break;
    case 0x13:
        Regs.DE -= 1;
        break;
    case 0x23:
        Regs.HL -= 1;
        break;
    case 0x33:
        Regs.SP -= 1;
        break;
    }
}

// add with carry + halfcarry
void CoreSM83::ADD8() {
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode) {
    case 0x80:
        data = Regs.BC_.B;
        break;
    case 0x81:
        data = Regs.BC_.C;
        break;
    case 0x82:
        data = Regs.DE_.D;
        break;
    case 0x83:
        data = Regs.DE_.E;
        break;
    case 0x84:
        data = Regs.HL_.H;
        break;
    case 0x85:
        data = Regs.HL_.L;
        break;
    case 0x86:
        data = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x87:
        data = Regs.A;
        break;
    case 0xc6:
        data = mmu_instance->Read8Bit(Regs.PC);
        Regs.PC++;
        break;
    }

    ADD_8_FLAGS(Regs.A, data, Regs.F);
    Regs.A += data;
    ZERO_FLAG(Regs.A, Regs.F);
}

void CoreSM83::ADDHL() {
    RESET_FLAGS(FLAG_SUB | FLAG_HCARRY | FLAG_CARRY, Regs.F);

    switch (opcode) {
    case 0x09:
        data = Regs.BC;
        break;
    case 0x19:
        data = Regs.DE;
        break;
    case 0x29:
        data = Regs.HL;
        break;
    case 0x39:
        data = Regs.SP;
        break;
    }

    ADD_16_FLAGS(Regs.HL, data, Regs.F);
    Regs.HL += data;
}

// add for SP + signed immediate data r8
void CoreSM83::ADDSPr8() {
    data = mmu_instance->Read8Bit(Regs.PC);
    Regs.PC++;

    RESET_ALL_FLAGS(Regs.F);
    ADD_8_FLAGS(Regs.SP, data, Regs.F);
    Regs.SP += *(i8*)&data;
}

// adc for A + (register or immediate unsigned data d8) + carry
void CoreSM83::ADC() {
    u8 carry = (Regs.F & FLAG_CARRY ? 1 : 0);
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode) {
    case 0x88:
        data = Regs.BC_.B;
        break;
    case 0x89:
        data = Regs.BC_.C;
        break;
    case 0x8a:
        data = Regs.DE_.D;
        break;
    case 0x8b:
        data = Regs.DE_.E;
        break;
    case 0x8c:
        data = Regs.HL_.H;
        break;
    case 0x8d:
        data = Regs.HL_.L;
        break;
    case 0x8e:
        data = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x8f:
        data = Regs.A;
        break;
    case 0xce:
        data = mmu_instance->Read8Bit(Regs.PC);
        Regs.PC++;
        break;
    }

    ADC_8_HC(Regs.A, data, carry, Regs.F);
    ADC_8_C(Regs.A, data, carry, Regs.F);
    Regs.A += data + carry;
    ZERO_FLAG(Regs.A, Regs.F);
}

// sub with carry + halfcarry
void CoreSM83::SUB() {
    Regs.F = FLAG_SUB;

    switch (opcode) {
    case 0x90:
        data = Regs.BC_.B;
        break;
    case 0x91:
        data = Regs.BC_.C;
        break;
    case 0x92:
        data = Regs.DE_.D;
        break;
    case 0x93:
        data = Regs.DE_.E;
        break;
    case 0x94:
        data = Regs.HL_.H;
        break;
    case 0x95:
        data = Regs.HL_.L;
        break;
    case 0x96:
        data = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x97:
        data = Regs.A;
        break;
    case 0xd6:
        data = mmu_instance->Read8Bit(Regs.PC);
        Regs.PC++;
        break;
    }

    SUB_8_FLAGS(Regs.A, data, Regs.F);
    Regs.A -= data;
    ZERO_FLAG(Regs.A, Regs.F);
}

// adc for A + (register or immediate unsigned data d8) + carry
void CoreSM83::SBC() {
    u8 carry = (Regs.F & FLAG_CARRY ? 1 : 0);
    Regs.F = FLAG_SUB;

    switch (opcode) {
    case 0x98:
        data = Regs.BC_.B;
        break;
    case 0x99:
        data = Regs.BC_.C;
        break;
    case 0x9a:
        data = Regs.DE_.D;
        break;
    case 0x9b:
        data = Regs.DE_.E;
        break;
    case 0x9c:
        data = Regs.HL_.H;
        break;
    case 0x9d:
        data = Regs.HL_.L;
        break;
    case 0x9e:
        data = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0x9f:
        data = Regs.A;
        break;
    case 0xde:
        data = mmu_instance->Read8Bit(Regs.PC);
        Regs.PC++;
        break;
    }

    SBC_8_HC(Regs.A, data, carry, Regs.F);
    SBC_8_C(Regs.A, data, carry, Regs.F);
    Regs.A -= (data + carry);
    ZERO_FLAG(Regs.A, Regs.F);
}

// bcd code addition and subtraction
void CoreSM83::DAA() {
    if (Regs.F & FLAG_SUB) {
        if (Regs.F & FLAG_CARRY) { Regs.A -= 0x60; }
        if (Regs.F & FLAG_HCARRY) { Regs.A -= 0x06; }
    }
    else {
        if (Regs.F & FLAG_CARRY || Regs.A > 0x99) {
            Regs.A += 0x60;
            Regs.F |= FLAG_CARRY;
        }
        if (Regs.F & FLAG_HCARRY || (Regs.A & 0x0F) > 0x09) {
            Regs.A += 0x06;
        }
    }
    ZERO_FLAG(Regs.A, Regs.F);
    Regs.F &= ~FLAG_HCARRY;
}

// and with hc=1 and z
void CoreSM83::AND() {
    Regs.F = FLAG_HCARRY;

    switch (opcode) {
    case 0xa0:
        data = Regs.BC_.B;
        break;
    case 0xa1:
        data = Regs.BC_.C;
        break;
    case 0xa2:
        data = Regs.DE_.D;
        break;
    case 0xa3:
        data = Regs.DE_.E;
        break;
    case 0xa4:
        data = Regs.HL_.H;
        break;
    case 0xa5:
        data = Regs.HL_.L;
        break;
    case 0xa6:
        data = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0xa7:
        data = Regs.A;
        break;
    case 0xe6:
        data = mmu_instance->Read8Bit(Regs.PC);
        Regs.PC++;
        break;
    }

    Regs.A &= data;
    ZERO_FLAG(Regs.A, Regs.F);
}

// or with z
void CoreSM83::OR() {
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode) {
    case 0xb0:
        data = Regs.BC_.B;
        break;
    case 0xb1:
        data = Regs.BC_.C;
        break;
    case 0xb2:
        data = Regs.DE_.D;
        break;
    case 0xb3:
        data = Regs.DE_.E;
        break;
    case 0xb4:
        data = Regs.HL_.H;
        break;
    case 0xb5:
        data = Regs.HL_.L;
        break;
    case 0xb6:
        data = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0xb7:
        data = Regs.A;
        break;
    case 0xf6:
        data = mmu_instance->Read8Bit(Regs.PC);
        Regs.PC++;
        break;
    }

    Regs.A |= data;
    ZERO_FLAG(Regs.A, Regs.F);
}

// xor with z
void CoreSM83::XOR() {
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode) {
    case 0xa8:
        data = Regs.BC_.B;
        break;
    case 0xa9:
        data = Regs.BC_.C;
        break;
    case 0xaa:
        data = Regs.DE_.D;
        break;
    case 0xab:
        data = Regs.DE_.E;
        break;
    case 0xac:
        data = Regs.HL_.H;
        break;
    case 0xad:
        data = Regs.HL_.L;
        break;
    case 0xae:
        data = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0xaf:
        data = Regs.A;
        break;
    case 0xee:
        data = mmu_instance->Read8Bit(Regs.PC);
        Regs.PC++;
        break;
    }

    Regs.A ^= data;
    ZERO_FLAG(Regs.A, Regs.F);
}

// compare (subtraction)
void CoreSM83::CP() {
    Regs.F = FLAG_SUB;

    switch (opcode) {
    case 0xb8:
        data = Regs.BC_.B;
        break;
    case 0xb9:
        data = Regs.BC_.C;
        break;
    case 0xba:
        data = Regs.DE_.D;
        break;
    case 0xbb:
        data = Regs.DE_.E;
        break;
    case 0xbc:
        data = Regs.HL_.H;
        break;
    case 0xbd:
        data = Regs.HL_.L;
        break;
    case 0xbe:
        data = mmu_instance->Read8Bit(Regs.HL);
        break;
    case 0xbf:
        data = Regs.A;
        break;
    case 0xfe:
        data = mmu_instance->Read8Bit(Regs.PC);
        Regs.PC++;
        break;
    }

    SUB_8_FLAGS(Regs.A, data, Regs.F);
    Regs.A -= data;
    ZERO_FLAG(Regs.A, Regs.F);
}

// 1's complement of A
void CoreSM83::CPL() {
    SET_FLAGS(FLAG_SUB | FLAG_HCARRY, Regs.F);
    Regs.A = ~(Regs.A);
}

/* ***********************************************************************************************************
    ROTATE AND SHIFT
*********************************************************************************************************** */
// rotate A left with carry
void CoreSM83::RLCA() {
    RESET_ALL_FLAGS(Regs.F);
    Regs.F |= (Regs.A & MSB ? FLAG_CARRY : 0x00);
    Regs.A <<= 1;
    Regs.A |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
}

// rotate A right with carry
void CoreSM83::RRCA() {
    RESET_ALL_FLAGS(Regs.F);
    Regs.F |= (Regs.A & LSB ? FLAG_CARRY : 0x00);
    Regs.A >>= 1;
    Regs.A |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
}

// rotate A left through carry
void CoreSM83::RLA() {
    static bool carry = Regs.F & FLAG_CARRY;
    RESET_ALL_FLAGS(Regs.F);
    Regs.F |= (Regs.A & MSB ? FLAG_CARRY : 0x00);
    Regs.A <<= 1;
    Regs.A |= (carry ? LSB : 0x00);
}

// rotate A right through carry
void CoreSM83::RRA() {
    static bool carry = Regs.F & FLAG_CARRY;
    RESET_ALL_FLAGS(Regs.F);
    Regs.F |= (Regs.A & LSB ? FLAG_CARRY : 0x00);
    Regs.A >>= 1;
    Regs.A |= (carry ? MSB : 0x00);
}

/* ***********************************************************************************************************
    JUMP INSTRUCTIONS
*********************************************************************************************************** */
// jump to memory location
void CoreSM83::JP() {
    static bool carry;
    static bool zero;

    if (opcode == 0xe9) {
        data = Regs.HL;
        jump_jp();
        return;
    }

    data = mmu_instance->Read16Bit(Regs.PC);
    Regs.PC += 2;

    switch (opcode) {
    case 0xCA:
        zero = Regs.F & FLAG_ZERO;
        if (zero) { 
            jump_jp(); 
            machineCycles += 4;
            return;
        }
        break;
    case 0xC2:
        zero = Regs.F & FLAG_ZERO;
        if (!zero) { 
            jump_jp(); 
            machineCycles += 4;
            return;
        }
        break;
    case 0xDA:
        carry = Regs.F & FLAG_CARRY;
        if (carry) { 
            jump_jp(); 
            machineCycles += 4;
            return;
        }
        break;
    case 0xD2:
        carry = Regs.F & FLAG_CARRY;
        if (!carry) { 
            jump_jp(); 
            machineCycles += 4;
            return;
        }
        break;
    default:
        jump_jp();
        machineCycles += 4;
        return;
        break;
    }
    machineCycles += 3;
}

// jump relative to memory lecation
void CoreSM83::JR() {
    static bool carry;
    static bool zero;

    data = mmu_instance->Read16Bit(Regs.PC);
    Regs.PC += 2;

    switch (opcode) {
    case 0x28:
        zero = Regs.F & FLAG_ZERO;
        if (zero) { 
            jump_jr(); 
            machineCycles += 3;
            return;
        }
        break;
    case 0x20:
        zero = Regs.F & FLAG_ZERO;
        if (!zero) { 
            jump_jr(); 
            machineCycles += 3;
            return;
        }
        break;
    case 0x38:
        carry = Regs.F & FLAG_CARRY;
        if (carry) { 
            jump_jr(); 
            machineCycles += 3;
            return;
        }
        break;
    case 0x30:
        carry = Regs.F & FLAG_CARRY;
        if (!carry) { 
            jump_jr();
            machineCycles += 3;
            return;
        }
        break;
    default:
        jump_jr();
        break;
    }
    machineCycles += 2;
}

void CoreSM83::jump_jp() {
    Regs.PC = data;
}

void CoreSM83::jump_jr() {
    Regs.PC += *(i8*)&data;
}

// call routine at memory location
void CoreSM83::CALL() {
    static bool carry;
    static bool zero;

    data = mmu_instance->Read16Bit(Regs.PC);
    Regs.PC += 2;

    switch (opcode) {
    case 0xCC:
        zero = Regs.F & FLAG_ZERO;
        if (zero) { 
            call(); 
            machineCycles += 6;
            return;
        }
        break;
    case 0xC4:
        zero = Regs.F & FLAG_ZERO;
        if (!zero) { 
            call(); 
            machineCycles += 6;
            return;
        }
        break;
    case 0xDC:
        carry = Regs.F & FLAG_CARRY;
        if (carry) { 
            call(); 
            machineCycles += 6;
            return;
        }
        break;
    case 0xD4:
        carry = Regs.F & FLAG_CARRY;
        if (!carry) { 
            call(); 
            machineCycles += 6;
            return;
        }
        break;
    default:
        call();
        machineCycles += 6;
        return;
        break;
    }
    machineCycles += 3;
    return;
}

// call to interrupt vectors
void CoreSM83::RST() {
    switch (opcode) {
    case 0xc7:
        data = 0x00;
        break;
    case 0xd7:
        data = 0x10;
        break;
    case 0xe7:
        data = 0x20;
        break;
    case 0xf7:
        data = 0x30;
        break;
    case 0xcf:
        data = 0x08;
        break;
    case 0xdf:
        data = 0x18;
        break;
    case 0xef:
        data = 0x28;
        break;
    case 0xff:
        data = 0x38;
        break;
    }
    call();
}

void CoreSM83::call() {
    Regs.SP -= 2;
    mmu_instance->Write16Bit(Regs.PC, Regs.SP);
    Regs.PC = data;
}

// return from routine
void CoreSM83::RET() {
    static bool carry;
    static bool zero;

    switch (opcode) {
    case 0xC8:
        zero = Regs.F & FLAG_ZERO;
        if (zero) { 
            ret(); 
            machineCycles += 5;
            return;
        }
        break;
    case 0xC0:
        zero = Regs.F & FLAG_ZERO;
        if (!zero) { 
            ret(); 
            machineCycles += 5;
            return;
        }
        break;
    case 0xD8:
        carry = Regs.F & FLAG_CARRY;
        if (carry) { 
            ret(); 
            machineCycles += 5;
            return;
        }
        break;
    case 0xD0:
        carry = Regs.F & FLAG_CARRY;
        if (!carry) { 
            ret(); 
            machineCycles += 5;
            return;
        }
        break;
    default:
        ret();
        machineCycles += 4;
        return;
        break;
    }
    machineCycles += 2;
}

// return and enable interrupts
void CoreSM83::RETI() {
    ret();
    ime = true;
}

void CoreSM83::ret() {
    Regs.PC = mmu_instance->Read16Bit(Regs.SP);
    Regs.SP += 2;
}

/* ***********************************************************************************************************
*
*   GAMEBOY (COLOR) CORESM83 CB INSTRUCTION LOOKUP TABLE
*
*********************************************************************************************************** */
void CoreSM83::setupLookupTableCB() {
    instrMapCB.clear();

    // Elements: opcode, instruction function, machine cycles
    instrMapCB.emplace_back(0x00, &CoreSM83::RLC, 2);
    instrMapCB.emplace_back(0x01, &CoreSM83::RLC, 2);
    instrMapCB.emplace_back(0x02, &CoreSM83::RLC, 2);
    instrMapCB.emplace_back(0x03, &CoreSM83::RLC, 2);
    instrMapCB.emplace_back(0x04, &CoreSM83::RLC, 2);
    instrMapCB.emplace_back(0x05, &CoreSM83::RLC, 2);
    instrMapCB.emplace_back(0x06, &CoreSM83::RLC, 4);
    instrMapCB.emplace_back(0x07, &CoreSM83::RLC, 2);
    instrMapCB.emplace_back(0x08, &CoreSM83::RRC, 2);
    instrMapCB.emplace_back(0x09, &CoreSM83::RRC, 2);
    instrMapCB.emplace_back(0x0a, &CoreSM83::RRC, 2);
    instrMapCB.emplace_back(0x0b, &CoreSM83::RRC, 2);
    instrMapCB.emplace_back(0x0c, &CoreSM83::RRC, 2);
    instrMapCB.emplace_back(0x0d, &CoreSM83::RRC, 2);
    instrMapCB.emplace_back(0x0e, &CoreSM83::RRC, 4);
    instrMapCB.emplace_back(0x0f, &CoreSM83::RRC, 2);

    instrMapCB.emplace_back(0x10, &CoreSM83::RL, 2);
    instrMapCB.emplace_back(0x11, &CoreSM83::RL, 2);
    instrMapCB.emplace_back(0x12, &CoreSM83::RL, 2);
    instrMapCB.emplace_back(0x13, &CoreSM83::RL, 2);
    instrMapCB.emplace_back(0x14, &CoreSM83::RL, 2);
    instrMapCB.emplace_back(0x15, &CoreSM83::RL, 2);
    instrMapCB.emplace_back(0x16, &CoreSM83::RL, 4);
    instrMapCB.emplace_back(0x17, &CoreSM83::RL, 2);
    instrMapCB.emplace_back(0x18, &CoreSM83::RR, 2);
    instrMapCB.emplace_back(0x19, &CoreSM83::RR, 2);
    instrMapCB.emplace_back(0x1a, &CoreSM83::RR, 2);
    instrMapCB.emplace_back(0x1b, &CoreSM83::RR, 2);
    instrMapCB.emplace_back(0x1c, &CoreSM83::RR, 2);
    instrMapCB.emplace_back(0x1d, &CoreSM83::RR, 2);
    instrMapCB.emplace_back(0x1e, &CoreSM83::RR, 4);
    instrMapCB.emplace_back(0x1f, &CoreSM83::RR, 2);

    instrMapCB.emplace_back(0x20, &CoreSM83::SLA, 2);
    instrMapCB.emplace_back(0x21, &CoreSM83::SLA, 2);
    instrMapCB.emplace_back(0x22, &CoreSM83::SLA, 2);
    instrMapCB.emplace_back(0x23, &CoreSM83::SLA, 2);
    instrMapCB.emplace_back(0x24, &CoreSM83::SLA, 2);
    instrMapCB.emplace_back(0x25, &CoreSM83::SLA, 2);
    instrMapCB.emplace_back(0x26, &CoreSM83::SLA, 4);
    instrMapCB.emplace_back(0x27, &CoreSM83::SLA, 2);
    instrMapCB.emplace_back(0x28, &CoreSM83::SRA, 2);
    instrMapCB.emplace_back(0x29, &CoreSM83::SRA, 2);
    instrMapCB.emplace_back(0x2a, &CoreSM83::SRA, 2);
    instrMapCB.emplace_back(0x2b, &CoreSM83::SRA, 2);
    instrMapCB.emplace_back(0x2c, &CoreSM83::SRA, 2);
    instrMapCB.emplace_back(0x2d, &CoreSM83::SRA, 2);
    instrMapCB.emplace_back(0x2e, &CoreSM83::SRA, 4);
    instrMapCB.emplace_back(0x2f, &CoreSM83::SRA, 2);

    instrMapCB.emplace_back(0x30, &CoreSM83::SWAP, 2);
    instrMapCB.emplace_back(0x31, &CoreSM83::SWAP, 2);
    instrMapCB.emplace_back(0x32, &CoreSM83::SWAP, 2);
    instrMapCB.emplace_back(0x33, &CoreSM83::SWAP, 2);
    instrMapCB.emplace_back(0x34, &CoreSM83::SWAP, 2);
    instrMapCB.emplace_back(0x35, &CoreSM83::SWAP, 2);
    instrMapCB.emplace_back(0x36, &CoreSM83::SWAP, 4);
    instrMapCB.emplace_back(0x37, &CoreSM83::SWAP, 2);
    instrMapCB.emplace_back(0x38, &CoreSM83::SRL, 2);
    instrMapCB.emplace_back(0x39, &CoreSM83::SRL, 2);
    instrMapCB.emplace_back(0x3a, &CoreSM83::SRL, 2);
    instrMapCB.emplace_back(0x3b, &CoreSM83::SRL, 2);
    instrMapCB.emplace_back(0x3c, &CoreSM83::SRL, 2);
    instrMapCB.emplace_back(0x3d, &CoreSM83::SRL, 2);
    instrMapCB.emplace_back(0x3e, &CoreSM83::SRL, 4);
    instrMapCB.emplace_back(0x3f, &CoreSM83::SRL, 2);

    instrMapCB.emplace_back(0x40, &CoreSM83::BIT0, 2);
    instrMapCB.emplace_back(0x41, &CoreSM83::BIT0, 2);
    instrMapCB.emplace_back(0x42, &CoreSM83::BIT0, 2);
    instrMapCB.emplace_back(0x43, &CoreSM83::BIT0, 2);
    instrMapCB.emplace_back(0x44, &CoreSM83::BIT0, 2);
    instrMapCB.emplace_back(0x45, &CoreSM83::BIT0, 2);
    instrMapCB.emplace_back(0x46, &CoreSM83::BIT0, 4);
    instrMapCB.emplace_back(0x47, &CoreSM83::BIT0, 2);
    instrMapCB.emplace_back(0x48, &CoreSM83::BIT1, 2);
    instrMapCB.emplace_back(0x49, &CoreSM83::BIT1, 2);
    instrMapCB.emplace_back(0x4a, &CoreSM83::BIT1, 2);
    instrMapCB.emplace_back(0x4b, &CoreSM83::BIT1, 2);
    instrMapCB.emplace_back(0x4c, &CoreSM83::BIT1, 2);
    instrMapCB.emplace_back(0x4d, &CoreSM83::BIT1, 2);
    instrMapCB.emplace_back(0x4e, &CoreSM83::BIT1, 4);
    instrMapCB.emplace_back(0x4f, &CoreSM83::BIT1, 2);

    instrMapCB.emplace_back(0x50, &CoreSM83::BIT2, 2);
    instrMapCB.emplace_back(0x51, &CoreSM83::BIT2, 2);
    instrMapCB.emplace_back(0x52, &CoreSM83::BIT2, 2);
    instrMapCB.emplace_back(0x53, &CoreSM83::BIT2, 2);
    instrMapCB.emplace_back(0x54, &CoreSM83::BIT2, 2);
    instrMapCB.emplace_back(0x55, &CoreSM83::BIT2, 2);
    instrMapCB.emplace_back(0x56, &CoreSM83::BIT2, 4);
    instrMapCB.emplace_back(0x57, &CoreSM83::BIT2, 2);
    instrMapCB.emplace_back(0x58, &CoreSM83::BIT3, 2);
    instrMapCB.emplace_back(0x59, &CoreSM83::BIT3, 2);
    instrMapCB.emplace_back(0x5a, &CoreSM83::BIT3, 2);
    instrMapCB.emplace_back(0x5b, &CoreSM83::BIT3, 2);
    instrMapCB.emplace_back(0x5c, &CoreSM83::BIT3, 2);
    instrMapCB.emplace_back(0x5d, &CoreSM83::BIT3, 2);
    instrMapCB.emplace_back(0x5e, &CoreSM83::BIT3, 4);
    instrMapCB.emplace_back(0x5f, &CoreSM83::BIT3, 2);

    instrMapCB.emplace_back(0x60, &CoreSM83::BIT4, 2);
    instrMapCB.emplace_back(0x61, &CoreSM83::BIT4, 2);
    instrMapCB.emplace_back(0x62, &CoreSM83::BIT4, 2);
    instrMapCB.emplace_back(0x63, &CoreSM83::BIT4, 2);
    instrMapCB.emplace_back(0x64, &CoreSM83::BIT4, 2);
    instrMapCB.emplace_back(0x65, &CoreSM83::BIT4, 2);
    instrMapCB.emplace_back(0x66, &CoreSM83::BIT4, 4);
    instrMapCB.emplace_back(0x67, &CoreSM83::BIT4, 2);
    instrMapCB.emplace_back(0x68, &CoreSM83::BIT5, 2);
    instrMapCB.emplace_back(0x69, &CoreSM83::BIT5, 2);
    instrMapCB.emplace_back(0x6a, &CoreSM83::BIT5, 2);
    instrMapCB.emplace_back(0x6b, &CoreSM83::BIT5, 2);
    instrMapCB.emplace_back(0x6c, &CoreSM83::BIT5, 2);
    instrMapCB.emplace_back(0x6d, &CoreSM83::BIT5, 2);
    instrMapCB.emplace_back(0x6e, &CoreSM83::BIT5, 4);
    instrMapCB.emplace_back(0x6f, &CoreSM83::BIT5, 2);

    instrMapCB.emplace_back(0x70, &CoreSM83::BIT6, 2);
    instrMapCB.emplace_back(0x71, &CoreSM83::BIT6, 2);
    instrMapCB.emplace_back(0x72, &CoreSM83::BIT6, 2);
    instrMapCB.emplace_back(0x73, &CoreSM83::BIT6, 2);
    instrMapCB.emplace_back(0x74, &CoreSM83::BIT6, 2);
    instrMapCB.emplace_back(0x75, &CoreSM83::BIT6, 2);
    instrMapCB.emplace_back(0x76, &CoreSM83::BIT6, 4);
    instrMapCB.emplace_back(0x77, &CoreSM83::BIT6, 2);
    instrMapCB.emplace_back(0x78, &CoreSM83::BIT7, 2);
    instrMapCB.emplace_back(0x79, &CoreSM83::BIT7, 2);
    instrMapCB.emplace_back(0x7a, &CoreSM83::BIT7, 2);
    instrMapCB.emplace_back(0x7b, &CoreSM83::BIT7, 2);
    instrMapCB.emplace_back(0x7c, &CoreSM83::BIT7, 2);
    instrMapCB.emplace_back(0x7d, &CoreSM83::BIT7, 2);
    instrMapCB.emplace_back(0x7e, &CoreSM83::BIT7, 4);
    instrMapCB.emplace_back(0x7f, &CoreSM83::BIT7, 2);

    instrMapCB.emplace_back(0x80, &CoreSM83::RES0, 2);
    instrMapCB.emplace_back(0x81, &CoreSM83::RES0, 2);
    instrMapCB.emplace_back(0x82, &CoreSM83::RES0, 2);
    instrMapCB.emplace_back(0x83, &CoreSM83::RES0, 2);
    instrMapCB.emplace_back(0x84, &CoreSM83::RES0, 2);
    instrMapCB.emplace_back(0x85, &CoreSM83::RES0, 2);
    instrMapCB.emplace_back(0x86, &CoreSM83::RES0, 4);
    instrMapCB.emplace_back(0x87, &CoreSM83::RES0, 2);
    instrMapCB.emplace_back(0x88, &CoreSM83::RES1, 2);
    instrMapCB.emplace_back(0x89, &CoreSM83::RES1, 2);
    instrMapCB.emplace_back(0x8a, &CoreSM83::RES1, 2);
    instrMapCB.emplace_back(0x8b, &CoreSM83::RES1, 2);
    instrMapCB.emplace_back(0x8c, &CoreSM83::RES1, 2);
    instrMapCB.emplace_back(0x8d, &CoreSM83::RES1, 2);
    instrMapCB.emplace_back(0x8e, &CoreSM83::RES1, 4);
    instrMapCB.emplace_back(0x8f, &CoreSM83::RES1, 2);

    instrMapCB.emplace_back(0x90, &CoreSM83::RES2, 2);
    instrMapCB.emplace_back(0x91, &CoreSM83::RES2, 2);
    instrMapCB.emplace_back(0x92, &CoreSM83::RES2, 2);
    instrMapCB.emplace_back(0x93, &CoreSM83::RES2, 2);
    instrMapCB.emplace_back(0x94, &CoreSM83::RES2, 2);
    instrMapCB.emplace_back(0x95, &CoreSM83::RES2, 2);
    instrMapCB.emplace_back(0x96, &CoreSM83::RES2, 4);
    instrMapCB.emplace_back(0x97, &CoreSM83::RES2, 2);
    instrMapCB.emplace_back(0x98, &CoreSM83::RES3, 2);
    instrMapCB.emplace_back(0x99, &CoreSM83::RES3, 2);
    instrMapCB.emplace_back(0x9a, &CoreSM83::RES3, 2);
    instrMapCB.emplace_back(0x9b, &CoreSM83::RES3, 2);
    instrMapCB.emplace_back(0x9c, &CoreSM83::RES3, 2);
    instrMapCB.emplace_back(0x9d, &CoreSM83::RES3, 2);
    instrMapCB.emplace_back(0x9e, &CoreSM83::RES3, 4);
    instrMapCB.emplace_back(0x9f, &CoreSM83::RES3, 2);

    instrMapCB.emplace_back(0xa0, &CoreSM83::RES4, 2);
    instrMapCB.emplace_back(0xa1, &CoreSM83::RES4, 2);
    instrMapCB.emplace_back(0xa2, &CoreSM83::RES4, 2);
    instrMapCB.emplace_back(0xa3, &CoreSM83::RES4, 2);
    instrMapCB.emplace_back(0xa4, &CoreSM83::RES4, 2);
    instrMapCB.emplace_back(0xa5, &CoreSM83::RES4, 2);
    instrMapCB.emplace_back(0xa6, &CoreSM83::RES4, 4);
    instrMapCB.emplace_back(0xa7, &CoreSM83::RES4, 2);
    instrMapCB.emplace_back(0xa8, &CoreSM83::RES5, 2);
    instrMapCB.emplace_back(0xa9, &CoreSM83::RES5, 2);
    instrMapCB.emplace_back(0xaa, &CoreSM83::RES5, 2);
    instrMapCB.emplace_back(0xab, &CoreSM83::RES5, 2);
    instrMapCB.emplace_back(0xac, &CoreSM83::RES5, 2);
    instrMapCB.emplace_back(0xad, &CoreSM83::RES5, 2);
    instrMapCB.emplace_back(0xae, &CoreSM83::RES5, 4);
    instrMapCB.emplace_back(0xaf, &CoreSM83::RES5, 2);

    instrMapCB.emplace_back(0xb0, &CoreSM83::RES6, 2);
    instrMapCB.emplace_back(0xb1, &CoreSM83::RES6, 2);
    instrMapCB.emplace_back(0xb2, &CoreSM83::RES6, 2);
    instrMapCB.emplace_back(0xb3, &CoreSM83::RES6, 2);
    instrMapCB.emplace_back(0xb4, &CoreSM83::RES6, 2);
    instrMapCB.emplace_back(0xb5, &CoreSM83::RES6, 2);
    instrMapCB.emplace_back(0xb6, &CoreSM83::RES6, 4);
    instrMapCB.emplace_back(0xb7, &CoreSM83::RES6, 2);
    instrMapCB.emplace_back(0xb8, &CoreSM83::RES7, 2);
    instrMapCB.emplace_back(0xb9, &CoreSM83::RES7, 2);
    instrMapCB.emplace_back(0xba, &CoreSM83::RES7, 2);
    instrMapCB.emplace_back(0xbb, &CoreSM83::RES7, 2);
    instrMapCB.emplace_back(0xbc, &CoreSM83::RES7, 2);
    instrMapCB.emplace_back(0xbd, &CoreSM83::RES7, 2);
    instrMapCB.emplace_back(0xbe, &CoreSM83::RES7, 4);
    instrMapCB.emplace_back(0xbf, &CoreSM83::RES7, 2);

    instrMapCB.emplace_back(0xc0, &CoreSM83::SET0, 2);
    instrMapCB.emplace_back(0xc1, &CoreSM83::SET0, 2);
    instrMapCB.emplace_back(0xc2, &CoreSM83::SET0, 2);
    instrMapCB.emplace_back(0xc3, &CoreSM83::SET0, 2);
    instrMapCB.emplace_back(0xc4, &CoreSM83::SET0, 2);
    instrMapCB.emplace_back(0xc5, &CoreSM83::SET0, 2);
    instrMapCB.emplace_back(0xc6, &CoreSM83::SET0, 4);
    instrMapCB.emplace_back(0xc7, &CoreSM83::SET0, 2);
    instrMapCB.emplace_back(0xc8, &CoreSM83::SET1, 2);
    instrMapCB.emplace_back(0xc9, &CoreSM83::SET1, 2);
    instrMapCB.emplace_back(0xca, &CoreSM83::SET1, 2);
    instrMapCB.emplace_back(0xcb, &CoreSM83::SET1, 2);
    instrMapCB.emplace_back(0xcc, &CoreSM83::SET1, 2);
    instrMapCB.emplace_back(0xcd, &CoreSM83::SET1, 2);
    instrMapCB.emplace_back(0xce, &CoreSM83::SET1, 4);
    instrMapCB.emplace_back(0xcf, &CoreSM83::SET1, 2);

    instrMapCB.emplace_back(0xd0, &CoreSM83::SET2, 2);
    instrMapCB.emplace_back(0xd1, &CoreSM83::SET2, 2);
    instrMapCB.emplace_back(0xd2, &CoreSM83::SET2, 2);
    instrMapCB.emplace_back(0xd3, &CoreSM83::SET2, 2);
    instrMapCB.emplace_back(0xd4, &CoreSM83::SET2, 2);
    instrMapCB.emplace_back(0xd5, &CoreSM83::SET2, 2);
    instrMapCB.emplace_back(0xd6, &CoreSM83::SET2, 4);
    instrMapCB.emplace_back(0xd7, &CoreSM83::SET2, 2);
    instrMapCB.emplace_back(0xd8, &CoreSM83::SET3, 2);
    instrMapCB.emplace_back(0xd9, &CoreSM83::SET3, 2);
    instrMapCB.emplace_back(0xda, &CoreSM83::SET3, 2);
    instrMapCB.emplace_back(0xdb, &CoreSM83::SET3, 2);
    instrMapCB.emplace_back(0xdc, &CoreSM83::SET3, 2);
    instrMapCB.emplace_back(0xdd, &CoreSM83::SET3, 2);
    instrMapCB.emplace_back(0xde, &CoreSM83::SET3, 4);
    instrMapCB.emplace_back(0xdf, &CoreSM83::SET3, 2);

    instrMapCB.emplace_back(0xe0, &CoreSM83::SET4, 2);
    instrMapCB.emplace_back(0xe1, &CoreSM83::SET4, 2);
    instrMapCB.emplace_back(0xe2, &CoreSM83::SET4, 2);
    instrMapCB.emplace_back(0xe3, &CoreSM83::SET4, 2);
    instrMapCB.emplace_back(0xe4, &CoreSM83::SET4, 2);
    instrMapCB.emplace_back(0xe5, &CoreSM83::SET4, 2);
    instrMapCB.emplace_back(0xe6, &CoreSM83::SET4, 4);
    instrMapCB.emplace_back(0xe7, &CoreSM83::SET4, 2);
    instrMapCB.emplace_back(0xe8, &CoreSM83::SET5, 2);
    instrMapCB.emplace_back(0xe9, &CoreSM83::SET5, 2);
    instrMapCB.emplace_back(0xea, &CoreSM83::SET5, 2);
    instrMapCB.emplace_back(0xeb, &CoreSM83::SET5, 2);
    instrMapCB.emplace_back(0xec, &CoreSM83::SET5, 2);
    instrMapCB.emplace_back(0xed, &CoreSM83::SET5, 2);
    instrMapCB.emplace_back(0xee, &CoreSM83::SET5, 4);
    instrMapCB.emplace_back(0xef, &CoreSM83::SET5, 2);

    instrMapCB.emplace_back(0xf0, &CoreSM83::SET6, 2);
    instrMapCB.emplace_back(0xf1, &CoreSM83::SET6, 2);
    instrMapCB.emplace_back(0xf2, &CoreSM83::SET6, 2);
    instrMapCB.emplace_back(0xf3, &CoreSM83::SET6, 2);
    instrMapCB.emplace_back(0xf4, &CoreSM83::SET6, 2);
    instrMapCB.emplace_back(0xf5, &CoreSM83::SET6, 2);
    instrMapCB.emplace_back(0xf6, &CoreSM83::SET6, 4);
    instrMapCB.emplace_back(0xf7, &CoreSM83::SET6, 2);
    instrMapCB.emplace_back(0xf8, &CoreSM83::SET7, 2);
    instrMapCB.emplace_back(0xf9, &CoreSM83::SET7, 2);
    instrMapCB.emplace_back(0xfa, &CoreSM83::SET7, 2);
    instrMapCB.emplace_back(0xfb, &CoreSM83::SET7, 2);
    instrMapCB.emplace_back(0xfc, &CoreSM83::SET7, 2);
    instrMapCB.emplace_back(0xfd, &CoreSM83::SET7, 2);
    instrMapCB.emplace_back(0xfe, &CoreSM83::SET7, 4);
    instrMapCB.emplace_back(0xff, &CoreSM83::SET7, 2);
}

/* ***********************************************************************************************************
*
*   GAMEBOY (COLOR) CORESM83 CB INSTRUCTION SET
*
*********************************************************************************************************** */
// rotate left
void CoreSM83::RLC() {
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode & 0x07) {
    case 0x00:
        Regs.F |= (Regs.BC_.B & MSB ? FLAG_CARRY : 0x00);
        Regs.BC_.B <<= 1;
        Regs.BC_.B |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x01:
        Regs.F |= (Regs.BC_.C & MSB ? FLAG_CARRY : 0x00);
        Regs.BC_.C <<= 1;
        Regs.BC_.C |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x02:
        Regs.F |= (Regs.DE_.D & MSB ? FLAG_CARRY : 0x00);
        Regs.DE_.D <<= 1;
        Regs.DE_.D |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x03:
        Regs.F |= (Regs.DE_.E & MSB ? FLAG_CARRY : 0x00);
        Regs.DE_.E <<= 1;
        Regs.DE_.E |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x04:
        Regs.F |= (Regs.HL_.H & MSB ? FLAG_CARRY : 0x00);
        Regs.HL_.H <<= 1;
        Regs.HL_.H |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x05:
        Regs.F |= (Regs.HL_.L & MSB ? FLAG_CARRY : 0x00);
        Regs.HL_.L <<= 1;
        Regs.HL_.L |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        Regs.F |= (data & MSB ? FLAG_CARRY : 0x00);
        data = (data << 1) & 0xFF;
        data |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.F |= (Regs.A & MSB ? FLAG_CARRY : 0x00);
        Regs.A <<= 1;
        Regs.A |= (Regs.F & FLAG_CARRY ? LSB : 0x00);
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// rotate right
void CoreSM83::RRC() {
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode & 0x07) {
    case 0x00:
        Regs.F |= (Regs.BC_.B & LSB ? FLAG_CARRY : 0x00);
        Regs.BC_.B >>= 1;
        Regs.BC_.B |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x01:
        Regs.F |= (Regs.BC_.C & LSB ? FLAG_CARRY : 0x00);
        Regs.BC_.C >>= 1;
        Regs.BC_.C |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x02:
        Regs.F |= (Regs.DE_.D & LSB ? FLAG_CARRY : 0x00);
        Regs.DE_.D >>= 1;
        Regs.DE_.D |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x03:
        Regs.F |= (Regs.DE_.E & LSB ? FLAG_CARRY : 0x00);
        Regs.DE_.E >>= 1;
        Regs.DE_.E |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x04:
        Regs.F |= (Regs.HL_.H & LSB ? FLAG_CARRY : 0x00);
        Regs.HL_.H >>= 1;
        Regs.HL_.H |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x05:
        Regs.F |= (Regs.HL_.L & LSB ? FLAG_CARRY : 0x00);
        Regs.HL_.L >>= 1;
        Regs.HL_.L |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        Regs.F |= (data & LSB ? FLAG_CARRY : 0x00);
        data = (data >> 1) & 0xFF;
        data |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.F |= (Regs.A & LSB ? FLAG_CARRY : 0x00);
        Regs.A >>= 1;
        Regs.A |= (Regs.F & FLAG_CARRY ? MSB : 0x00);
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// rotate left through carry
void CoreSM83::RL() {
    static bool carry = Regs.F & FLAG_CARRY;
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode & 0x07) {
    case 0x00:
        Regs.F |= (Regs.BC_.B & MSB ? FLAG_CARRY : 0x00);
        Regs.BC_.B <<= 1;
        Regs.BC_.B |= (carry ? LSB : 0x00);
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x01:
        Regs.F |= (Regs.BC_.C & MSB ? FLAG_CARRY : 0x00);
        Regs.BC_.C <<= 1;
        Regs.BC_.C |= (carry ? LSB : 0x00);
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x02:
        Regs.F |= (Regs.DE_.D & MSB ? FLAG_CARRY : 0x00);
        Regs.DE_.D <<= 1;
        Regs.DE_.D |= (carry ? LSB : 0x00);
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x03:
        Regs.F |= (Regs.DE_.E & MSB ? FLAG_CARRY : 0x00);
        Regs.DE_.E <<= 1;
        Regs.DE_.E |= (carry ? LSB : 0x00);
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x04:
        Regs.F |= (Regs.HL_.H & MSB ? FLAG_CARRY : 0x00);
        Regs.HL_.H <<= 1;
        Regs.HL_.H |= (carry ? LSB : 0x00);
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x05:
        Regs.F |= (Regs.HL_.L & MSB ? FLAG_CARRY : 0x00);
        Regs.HL_.L <<= 1;
        Regs.HL_.L |= (carry ? LSB : 0x00);
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        Regs.F |= (data & MSB ? FLAG_CARRY : 0x00);
        data = (data << 1) & 0xFF;
        data |= (carry ? LSB : 0x00);
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.F |= (Regs.A & MSB ? FLAG_CARRY : 0x00);
        Regs.A <<= 1;
        Regs.A |= (carry ? LSB : 0x00);
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// rotate right through carry
void CoreSM83::RR() {
    static bool carry = Regs.F & FLAG_CARRY;
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode & 0x07) {
    case 0x00:
        Regs.F |= (Regs.BC_.B & LSB ? FLAG_CARRY : 0x00);
        Regs.BC_.B >>= 1;
        Regs.BC_.B |= (carry ? MSB : 0x00);
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x01:
        Regs.F |= (Regs.BC_.C & LSB ? FLAG_CARRY : 0x00);
        Regs.BC_.C >>= 1;
        Regs.BC_.C |= (carry ? MSB : 0x00);
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x02:
        Regs.F |= (Regs.DE_.D & LSB ? FLAG_CARRY : 0x00);
        Regs.DE_.D >>= 1;
        Regs.DE_.D |= (carry ? MSB : 0x00);
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x03:
        Regs.F |= (Regs.DE_.E & LSB ? FLAG_CARRY : 0x00);
        Regs.DE_.E >>= 1;
        Regs.DE_.E |= (carry ? MSB : 0x00);
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x04:
        Regs.F |= (Regs.HL_.H & LSB ? FLAG_CARRY : 0x00);
        Regs.HL_.H >>= 1;
        Regs.HL_.H |= (carry ? MSB : 0x00);
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x05:
        Regs.F |= (Regs.HL_.L & LSB ? FLAG_CARRY : 0x00);
        Regs.HL_.L >>= 1;
        Regs.HL_.L |= (carry ? MSB : 0x00);
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        Regs.F |= (data & LSB ? FLAG_CARRY : 0x00);
        data = (data >> 1) & 0xFF;
        data |= (carry ? MSB : 0x00);
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.F |= (Regs.A & LSB ? FLAG_CARRY : 0x00);
        Regs.A >>= 1;
        Regs.A |= (carry ? MSB : 0x00);
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// shift left arithmetic (multiply by 2)
void CoreSM83::SLA() {
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode & 0x07) {
    case 0x00:
        Regs.F |= (Regs.BC_.B & MSB ? FLAG_CARRY : 0x00);
        Regs.BC_.B <<= 1;
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x01:
        Regs.F |= (Regs.BC_.C & MSB ? FLAG_CARRY : 0x00);
        Regs.BC_.C <<= 1;
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x02:
        Regs.F |= (Regs.DE_.D & MSB ? FLAG_CARRY : 0x00);
        Regs.DE_.D <<= 1;
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x03:
        Regs.F |= (Regs.DE_.E & MSB ? FLAG_CARRY : 0x00);
        Regs.DE_.E <<= 1;
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x04:
        Regs.F |= (Regs.HL_.H & MSB ? FLAG_CARRY : 0x00);
        Regs.HL_.H <<= 1;
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x05:
        Regs.F |= (Regs.HL_.L & MSB ? FLAG_CARRY : 0x00);
        Regs.HL_.L <<= 1;
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        Regs.F |= (data & MSB ? FLAG_CARRY : 0x00);
        data = (data << 1) & 0xFF;
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.F |= (Regs.A & MSB ? FLAG_CARRY : 0x00);
        Regs.A <<= 1;
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// shift right arithmetic
void CoreSM83::SRA() {
    RESET_ALL_FLAGS(Regs.F);
    static bool msb;

    switch (opcode & 0x07) {
    case 0x00:
        msb = (Regs.BC_.B & MSB);
        Regs.BC_.B >>= 1;
        Regs.BC_.B |= (msb ? MSB : 0x00);
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x01:
        msb = (Regs.BC_.C & MSB);
        Regs.BC_.C >>= 1;
        Regs.BC_.C |= (msb ? MSB : 0x00);
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x02:
        msb = (Regs.DE_.D & MSB);
        Regs.DE_.D >>= 1;
        Regs.DE_.D |= (msb ? MSB : 0x00);
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x03:
        msb = (Regs.DE_.E & MSB);
        Regs.DE_.E >>= 1;
        Regs.DE_.E |= (msb ? MSB : 0x00);
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x04:
        msb = (Regs.HL_.H & MSB);
        Regs.HL_.H >>= 1;
        Regs.HL_.H |= (msb ? MSB : 0x00);
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x05:
        msb = (Regs.HL_.L & MSB);
        Regs.HL_.L >>= 1;
        Regs.HL_.L |= (msb ? MSB : 0x00);
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        msb = (data & MSB);
        data = (data >> 1) & 0xFF;
        data |= (msb ? MSB : 0x00);
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        msb = (Regs.A & MSB);
        Regs.A >>= 1;
        Regs.A |= (msb ? MSB : 0x00);
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// swap lo<->hi nibble
void CoreSM83::SWAP() {
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B = (Regs.BC_.B >> 4) | (Regs.BC_.B << 4);
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x01:
        Regs.BC_.C = (Regs.BC_.C >> 4) | (Regs.BC_.C << 4);
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x02:
        Regs.DE_.D = (Regs.DE_.D >> 4) | (Regs.DE_.D << 4);
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x03:
        Regs.DE_.E = (Regs.DE_.E >> 4) | (Regs.DE_.E << 4);
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x04:
        Regs.HL_.H = (Regs.HL_.H >> 4) | (Regs.HL_.H << 4);
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x05:
        Regs.HL_.L = (Regs.HL_.L >> 4) | (Regs.HL_.L << 4);
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        data = (data >> 4) | ((data << 4) & 0xF0);
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A = (Regs.A >> 4) | (Regs.A << 4);
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// shift right logical
void CoreSM83::SRL() {
    RESET_ALL_FLAGS(Regs.F);

    switch (opcode & 0x07) {
    case 0x00:
        Regs.F |= (Regs.BC_.B & LSB ? FLAG_CARRY : 0x00);
        Regs.BC_.B >>= 1;
        ZERO_FLAG(Regs.BC_.B, Regs.F);
        break;
    case 0x01:
        Regs.F |= (Regs.BC_.C & LSB ? FLAG_CARRY : 0x00);
        Regs.BC_.C >>= 1;
        ZERO_FLAG(Regs.BC_.C, Regs.F);
        break;
    case 0x02:
        Regs.F |= (Regs.DE_.D & LSB ? FLAG_CARRY : 0x00);
        Regs.DE_.D >>= 1;
        ZERO_FLAG(Regs.DE_.D, Regs.F);
        break;
    case 0x03:
        Regs.F |= (Regs.DE_.E & LSB ? FLAG_CARRY : 0x00);
        Regs.DE_.E >>= 1;
        ZERO_FLAG(Regs.DE_.E, Regs.F);
        break;
    case 0x04:
        Regs.F |= (Regs.HL_.H & LSB ? FLAG_CARRY : 0x00);
        Regs.HL_.H >>= 1;
        ZERO_FLAG(Regs.HL_.H, Regs.F);
        break;
    case 0x05:
        Regs.F |= (Regs.HL_.L & LSB ? FLAG_CARRY : 0x00);
        Regs.HL_.L >>= 1;
        ZERO_FLAG(Regs.HL_.L, Regs.F);
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        Regs.F |= (data & LSB ? FLAG_CARRY : 0x00);
        data = (data >> 1) & 0xFF;
        ZERO_FLAG(data, Regs.F);
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.F |= (Regs.A & LSB ? FLAG_CARRY : 0x00);
        Regs.A >>= 1;
        ZERO_FLAG(Regs.A, Regs.F);
        break;
    }
}

// test bit 0
void CoreSM83::BIT0() {
    RESET_FLAGS(FLAG_ZERO | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_HCARRY;

    switch (opcode & 0x07) {
    case 0x00:
        ZERO_FLAG(Regs.BC_.B & 0x01, Regs.F);
        break;
    case 0x01:
        ZERO_FLAG(Regs.BC_.C & 0x01, Regs.F);
        break;
    case 0x02:
        ZERO_FLAG(Regs.DE_.D & 0x01, Regs.F);
        break;
    case 0x03:
        ZERO_FLAG(Regs.DE_.E & 0x01, Regs.F);
        break;
    case 0x04:
        ZERO_FLAG(Regs.HL_.H & 0x01, Regs.F);
        break;
    case 0x05:
        ZERO_FLAG(Regs.HL_.L & 0x01, Regs.F);
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x01, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x01, Regs.F);
        break;
    }
}

// test bit 1
void CoreSM83::BIT1() {
    RESET_FLAGS(FLAG_ZERO | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_HCARRY;

    switch (opcode & 0x07) {
    case 0x00:
        ZERO_FLAG(Regs.BC_.B & 0x02, Regs.F);
        break;
    case 0x01:
        ZERO_FLAG(Regs.BC_.C & 0x02, Regs.F);
        break;
    case 0x02:
        ZERO_FLAG(Regs.DE_.D & 0x02, Regs.F);
        break;
    case 0x03:
        ZERO_FLAG(Regs.DE_.E & 0x02, Regs.F);
        break;
    case 0x04:
        ZERO_FLAG(Regs.HL_.H & 0x02, Regs.F);
        break;
    case 0x05:
        ZERO_FLAG(Regs.HL_.L & 0x02, Regs.F);
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x02, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x02, Regs.F);
        break;
    }
}

// test bit 2
void CoreSM83::BIT2() {
    RESET_FLAGS(FLAG_ZERO | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_HCARRY;

    switch (opcode & 0x07) {
    case 0x00:
        ZERO_FLAG(Regs.BC_.B & 0x04, Regs.F);
        break;
    case 0x01:
        ZERO_FLAG(Regs.BC_.C & 0x04, Regs.F);
        break;
    case 0x02:
        ZERO_FLAG(Regs.DE_.D & 0x04, Regs.F);
        break;
    case 0x03:
        ZERO_FLAG(Regs.DE_.E & 0x04, Regs.F);
        break;
    case 0x04:
        ZERO_FLAG(Regs.HL_.H & 0x04, Regs.F);
        break;
    case 0x05:
        ZERO_FLAG(Regs.HL_.L & 0x04, Regs.F);
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x04, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x04, Regs.F);
        break;
    }
}

// test bit 3
void CoreSM83::BIT3() {
    RESET_FLAGS(FLAG_ZERO | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_HCARRY;

    switch (opcode & 0x07) {
    case 0x00:
        ZERO_FLAG(Regs.BC_.B & 0x08, Regs.F);
        break;
    case 0x01:
        ZERO_FLAG(Regs.BC_.C & 0x08, Regs.F);
        break;
    case 0x02:
        ZERO_FLAG(Regs.DE_.D & 0x08, Regs.F);
        break;
    case 0x03:
        ZERO_FLAG(Regs.DE_.E & 0x08, Regs.F);
        break;
    case 0x04:
        ZERO_FLAG(Regs.HL_.H & 0x08, Regs.F);
        break;
    case 0x05:
        ZERO_FLAG(Regs.HL_.L & 0x08, Regs.F);
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x08, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x08, Regs.F);
        break;
    }
}

// test bit 4
void CoreSM83::BIT4() {
    RESET_FLAGS(FLAG_ZERO | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_HCARRY;

    switch (opcode & 0x07) {
    case 0x00:
        ZERO_FLAG(Regs.BC_.B & 0x10, Regs.F);
        break;
    case 0x01:
        ZERO_FLAG(Regs.BC_.C & 0x10, Regs.F);
        break;
    case 0x02:
        ZERO_FLAG(Regs.DE_.D & 0x10, Regs.F);
        break;
    case 0x03:
        ZERO_FLAG(Regs.DE_.E & 0x10, Regs.F);
        break;
    case 0x04:
        ZERO_FLAG(Regs.HL_.H & 0x10, Regs.F);
        break;
    case 0x05:
        ZERO_FLAG(Regs.HL_.L & 0x10, Regs.F);
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x10, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x10, Regs.F);
        break;
    }
}

// test bit 5
void CoreSM83::BIT5() {
    RESET_FLAGS(FLAG_ZERO | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_HCARRY;

    switch (opcode & 0x07) {
    case 0x00:
        ZERO_FLAG(Regs.BC_.B & 0x20, Regs.F);
        break;
    case 0x01:
        ZERO_FLAG(Regs.BC_.C & 0x20, Regs.F);
        break;
    case 0x02:
        ZERO_FLAG(Regs.DE_.D & 0x20, Regs.F);
        break;
    case 0x03:
        ZERO_FLAG(Regs.DE_.E & 0x20, Regs.F);
        break;
    case 0x04:
        ZERO_FLAG(Regs.HL_.H & 0x20, Regs.F);
        break;
    case 0x05:
        ZERO_FLAG(Regs.HL_.L & 0x20, Regs.F);
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x20, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x20, Regs.F);
        break;
    }
}

// test bit 6
void CoreSM83::BIT6() {
    RESET_FLAGS(FLAG_ZERO | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_HCARRY;

    switch (opcode & 0x07) {
    case 0x00:
        ZERO_FLAG(Regs.BC_.B & 0x40, Regs.F);
        break;
    case 0x01:
        ZERO_FLAG(Regs.BC_.C & 0x40, Regs.F);
        break;
    case 0x02:
        ZERO_FLAG(Regs.DE_.D & 0x40, Regs.F);
        break;
    case 0x03:
        ZERO_FLAG(Regs.DE_.E & 0x40, Regs.F);
        break;
    case 0x04:
        ZERO_FLAG(Regs.HL_.H & 0x40, Regs.F);
        break;
    case 0x05:
        ZERO_FLAG(Regs.HL_.L & 0x40, Regs.F);
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x40, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x40, Regs.F);
        break;
    }
}

// test bit 7
void CoreSM83::BIT7() {
    RESET_FLAGS(FLAG_ZERO | FLAG_SUB, Regs.F);
    Regs.F |= FLAG_HCARRY;

    switch (opcode & 0x07) {
    case 0x00:
        ZERO_FLAG(Regs.BC_.B & 0x80, Regs.F);
        break;
    case 0x01:
        ZERO_FLAG(Regs.BC_.C & 0x80, Regs.F);
        break;
    case 0x02:
        ZERO_FLAG(Regs.DE_.D & 0x80, Regs.F);
        break;
    case 0x03:
        ZERO_FLAG(Regs.DE_.E & 0x80, Regs.F);
        break;
    case 0x04:
        ZERO_FLAG(Regs.HL_.H & 0x80, Regs.F);
        break;
    case 0x05:
        ZERO_FLAG(Regs.HL_.L & 0x80, Regs.F);
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        ZERO_FLAG(data & 0x80, Regs.F);
        break;
    case 0x07:
        ZERO_FLAG(Regs.A & 0x80, Regs.F);
        break;
    }
}

// reset bit 0
void CoreSM83::RES0() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B &= ~0x01;
        break;
    case 0x01:
        Regs.BC_.C &= ~0x01;
        break;
    case 0x02:
        Regs.DE_.D &= ~0x01;
        break;
    case 0x03:
        Regs.DE_.E &= ~0x01;
        break;
    case 0x04:
        Regs.HL_.H &= ~0x01;
        break;
    case 0x05:
        Regs.HL_.L &= ~0x01;
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        data &= ~0x01;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x01;
        break;
    }
}

// reset bit 1
void CoreSM83::RES1() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B &= ~0x02;
        break;
    case 0x01:
        Regs.BC_.C &= ~0x02;
        break;
    case 0x02:
        Regs.DE_.D &= ~0x02;
        break;
    case 0x03:
        Regs.DE_.E &= ~0x02;
        break;
    case 0x04:
        Regs.HL_.H &= ~0x02;
        break;
    case 0x05:
        Regs.HL_.L &= ~0x02;
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        data &= ~0x02;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x02;
        break;
    }
}

// reset bit 2
void CoreSM83::RES2() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B &= ~0x04;
        break;
    case 0x01:
        Regs.BC_.C &= ~0x04;
        break;
    case 0x02:
        Regs.DE_.D &= ~0x04;
        break;
    case 0x03:
        Regs.DE_.E &= ~0x04;
        break;
    case 0x04:
        Regs.HL_.H &= ~0x04;
        break;
    case 0x05:
        Regs.HL_.L &= ~0x04;
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        data &= ~0x04;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x04;
        break;
    }
}

// reset bit 3
void CoreSM83::RES3() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B &= ~0x08;
        break;
    case 0x01:
        Regs.BC_.C &= ~0x08;
        break;
    case 0x02:
        Regs.DE_.D &= ~0x08;
        break;
    case 0x03:
        Regs.DE_.E &= ~0x08;
        break;
    case 0x04:
        Regs.HL_.H &= ~0x08;
        break;
    case 0x05:
        Regs.HL_.L &= ~0x08;
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        data &= ~0x08;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x08;
        break;
    }
}

// reset bit 4
void CoreSM83::RES4() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B &= ~0x10;
        break;
    case 0x01:
        Regs.BC_.C &= ~0x10;
        break;
    case 0x02:
        Regs.DE_.D &= ~0x10;
        break;
    case 0x03:
        Regs.DE_.E &= ~0x10;
        break;
    case 0x04:
        Regs.HL_.H &= ~0x10;
        break;
    case 0x05:
        Regs.HL_.L &= ~0x10;
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        data &= ~0x10;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x10;
        break;
    }
}

// reset bit 5
void CoreSM83::RES5() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B &= ~0x20;
        break;
    case 0x01:
        Regs.BC_.C &= ~0x20;
        break;
    case 0x02:
        Regs.DE_.D &= ~0x20;
        break;
    case 0x03:
        Regs.DE_.E &= ~0x20;
        break;
    case 0x04:
        Regs.HL_.H &= ~0x20;
        break;
    case 0x05:
        Regs.HL_.L &= ~0x20;
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        data &= ~0x20;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x20;
        break;
    }
}

// reset bit 6
void CoreSM83::RES6() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B &= ~0x40;
        break;
    case 0x01:
        Regs.BC_.C &= ~0x40;
        break;
    case 0x02:
        Regs.DE_.D &= ~0x40;
        break;
    case 0x03:
        Regs.DE_.E &= ~0x40;
        break;
    case 0x04:
        Regs.HL_.H &= ~0x40;
        break;
    case 0x05:
        Regs.HL_.L &= ~0x40;
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        data &= ~0x40;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x40;
        break;
    }
}

// reset bit 7
void CoreSM83::RES7() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B &= ~0x80;
        break;
    case 0x01:
        Regs.BC_.C &= ~0x80;
        break;
    case 0x02:
        Regs.DE_.D &= ~0x80;
        break;
    case 0x03:
        Regs.DE_.E &= ~0x80;
        break;
    case 0x04:
        Regs.HL_.H &= ~0x80;
        break;
    case 0x05:
        Regs.HL_.L &= ~0x80;
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        data &= ~0x80;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A &= ~0x80;
        break;
    }
}

// set bit 0
void CoreSM83::SET0() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B |= 0x01;
        break;
    case 0x01:
        Regs.BC_.C |= 0x01;
        break;
    case 0x02:
        Regs.DE_.D |= 0x01;
        break;
    case 0x03:
        Regs.DE_.E |= 0x01;
        break;
    case 0x04:
        Regs.HL_.H |= 0x01;
        break;
    case 0x05:
        Regs.HL_.L |= 0x01;
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        data |= 0x01;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x01;
        break;
    }
}

// set bit 1
void CoreSM83::SET1() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B |= 0x02;
        break;
    case 0x01:
        Regs.BC_.C |= 0x02;
        break;
    case 0x02:
        Regs.DE_.D |= 0x02;
        break;
    case 0x03:
        Regs.DE_.E |= 0x02;
        break;
    case 0x04:
        Regs.HL_.H |= 0x02;
        break;
    case 0x05:
        Regs.HL_.L |= 0x02;
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        data |= 0x02;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x02;
        break;
    }
}

// set bit 2
void CoreSM83::SET2() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B |= 0x04;
        break;
    case 0x01:
        Regs.BC_.C |= 0x04;
        break;
    case 0x02:
        Regs.DE_.D |= 0x04;
        break;
    case 0x03:
        Regs.DE_.E |= 0x04;
        break;
    case 0x04:
        Regs.HL_.H |= 0x04;
        break;
    case 0x05:
        Regs.HL_.L |= 0x04;
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        data |= 0x04;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x04;
        break;
    }
}

// set bit 3
void CoreSM83::SET3() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B |= 0x08;
        break;
    case 0x01:
        Regs.BC_.C |= 0x08;
        break;
    case 0x02:
        Regs.DE_.D |= 0x08;
        break;
    case 0x03:
        Regs.DE_.E |= 0x08;
        break;
    case 0x04:
        Regs.HL_.H |= 0x08;
        break;
    case 0x05:
        Regs.HL_.L |= 0x08;
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        data |= 0x08;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x08;
        break;
    }
}

// set bit 4
void CoreSM83::SET4() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B |= 0x10;
        break;
    case 0x01:
        Regs.BC_.C |= 0x10;
        break;
    case 0x02:
        Regs.DE_.D |= 0x10;
        break;
    case 0x03:
        Regs.DE_.E |= 0x10;
        break;
    case 0x04:
        Regs.HL_.H |= 0x10;
        break;
    case 0x05:
        Regs.HL_.L |= 0x10;
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        data |= 0x10;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x10;
        break;
    }
}

// set bit 5
void CoreSM83::SET5() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B |= 0x20;
        break;
    case 0x01:
        Regs.BC_.C |= 0x20;
        break;
    case 0x02:
        Regs.DE_.D |= 0x20;
        break;
    case 0x03:
        Regs.DE_.E |= 0x20;
        break;
    case 0x04:
        Regs.HL_.H |= 0x20;
        break;
    case 0x05:
        Regs.HL_.L |= 0x20;
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        data |= 0x20;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x20;
        break;
    }
}

// set bit 6
void CoreSM83::SET6() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B |= 0x40;
        break;
    case 0x01:
        Regs.BC_.C |= 0x40;
        break;
    case 0x02:
        Regs.DE_.D |= 0x40;
        break;
    case 0x03:
        Regs.DE_.E |= 0x40;
        break;
    case 0x04:
        Regs.HL_.H |= 0x40;
        break;
    case 0x05:
        Regs.HL_.L |= 0x40;
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        data |= 0x40;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x40;
        break;
    }
}

// set bit 7
void CoreSM83::SET7() {
    switch (opcode & 0x07) {
    case 0x00:
        Regs.BC_.B |= 0x80;
        break;
    case 0x01:
        Regs.BC_.C |= 0x80;
        break;
    case 0x02:
        Regs.DE_.D |= 0x80;
        break;
    case 0x03:
        Regs.DE_.E |= 0x80;
        break;
    case 0x04:
        Regs.HL_.H |= 0x80;
        break;
    case 0x05:
        Regs.HL_.L |= 0x80;
        break;
    case 0x06:
        data = mmu_instance->Read8Bit(Regs.HL);
        data |= 0x80;
        mmu_instance->Write8Bit(data, Regs.HL);
        break;
    case 0x07:
        Regs.A |= 0x80;
        break;
    }
}