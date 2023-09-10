#pragma once

#include "CoreBase.h"
#include "Cartridge.h"
#include "registers.h"
#include "defs.h"

#include <vector>

class CoreSM83 : protected Core
{
public:
	friend class Core;

private:
	// constructor
	explicit CoreSM83(const Cartridge& _cart_obj);
	// destructor
	~CoreSM83() = default;

	// instruction data
	u8 opcode;
	u16 data;

	// internals
	gbc_registers Regs = gbc_registers();
	void InitCpu(const Cartridge& _cart_obj) override;
	void InitRegisterStates() override;

	bool isCgb = false;

	// cpu states
	bool halt = false;
	bool ime = false;
	bool opcode_cb = false;

	// instruction members ****************
	// lookup table
	typedef void (CoreSM83::* instruction)();

	// current instruction context
	std::tuple<u8, instruction, int>* instrPtr = nullptr;
	int machineCycles = 0;

	// basic instructions
	std::vector<std::tuple<u8, instruction, int>> instrMap;
	void setupLookupTable();

	// CB instructions
	std::vector<std::tuple<u8, instruction, int>> instrMapCB;
	void setupLookupTableCB();
	 
	// basic instruction set *****
	void NoInstruction();
	
	// cpu control
	void NOP();
	void STOP();
	void HALT();
	void CCF();
	void SCF();
	void DI();
	void EI();
	void CB();

	// load
	void LDfromAtoRef();
	void LDtoAfromRef();

	void LDd8();
	void LDd16();

	void LDCref();
	void LDH();
	void LDHa16();
	void LDSPa16();
	void LDSPHL();

	void LDtoB();
	void LDtoC();
	void LDtoD();
	void LDtoE();
	void LDtoH();
	void LDtoL();
	void LDtoHLref();
	void LDtoA();

	void LDregs();

	void LDHLSPr8();

	void PUSH();
	void POP();

	// arithmetic/logic
	void INC8();
	void INC16();
	void DEC8();
	void DEC16();
	void ADD8();
	void ADDHL();
	void ADDSPr8();
	void ADC();
	void SUB();
	void SBC();
	void DAA();
	void AND();
	void OR();
	void XOR();
	void CP();
	void CPL();

	// rotate and shift
	void RLCA();
	void RRCA();
	void RLA();
	void RRA();

	// jump
	void JP();
	void JR();
	void CALL();
	void RST();
	void RET();
	void RETI();

	// instruction helpers
	void jump_jp();
	void jump_jr();
	void call();
	void ret();

	// CB instruction set *****
	// shift/rotate
	void RLC();
	void RRC();
	void RL();
	void RR();
	void SLA();
	void SRA();
	void SWAP();
	void SRL();

	// bit test
	void BIT0();
	void BIT1();
	void BIT2();
	void BIT3();
	void BIT4();
	void BIT5();
	void BIT6();
	void BIT7();

	// reset bit
	void RES0();
	void RES1();
	void RES2();
	void RES3();
	void RES4();
	void RES5();
	void RES6();
	void RES7();

	// set bit
	void SET0();
	void SET1();
	void SET2();
	void SET3();
	void SET4();
	void SET5();
	void SET6();
	void SET7();
};