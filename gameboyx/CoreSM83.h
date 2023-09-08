#pragma once

#include "CoreBase.h"
#include "Cartridge.h"
#include "registers.h"
#include "defs.h"

class CoreSM83 : protected Core
{
public:
	friend class Core;

private:
	// constructor
	explicit CoreSM83(const Cartridge& _cart_obj);
	// destructor
	~CoreSM83() = default;

	// members
	u8 opcode;
	u16 data;
	gbc_registers Regs = gbc_registers();

	// cpu states
	bool halt = false;
	bool ime = false;
	bool opcode_cb = false;

	// instruction members ****************
	// cpu control
	void InstrNOP();
	void InstrSTOP();
	void InstrHALT();
	void InstrCCF();
	void InstrSCF();
	void InstrDI();
	void InstrEI();
	void InstrCB();

	// load
	void InstrLDfromA();
	void InstrLDtoAfromRef();
	void InstrLDd8();
	void InstrLDd16();
	void InstrLDHLref();
	void InstrLDCref();
	void InstrLDH();
	void InstrLDtoB();
	void InstrLDtoC();
	void InstrLDtoD();
	void InstrLDtoE();
	void InstrLDtoH();
	void InstrLDtoL();
	void InstrLDtoHLref();
	void InstrLDtoA();
	void InstrLDregs();
	void InstrHLSPr8();
	void InstrPUSH();
	void InstrPOP();

	// arithmetic/logic
	void InstrINC8();
	void InstrINC16();
	void InstrDEC8();
	void InstrDEC16();
	void InstrADD8();
	void InstrADDHL();
	void InstrADDSPr8();
	void InstrADC();
	void InstrSUB();
	void InstrSBC();
	void InstrDAA();
	void InstrAND();
	void InstrOR();
	void InstrXOR();
	void InstrCP();
	void InstrCPL();

	// rotate and shift
	void InstrRLCA();
	void InstrRRCA();
	void InstrRLA();
	void InstrRRA();

	// jump
	void InstrJP();
	void InstrJR();
	void InstrCALL();
	void InstrRST();
	void InstrRET();
	void InstrRETI();

	// instruction helpers
	void jump_jp();
	void jump_jr();
	void call();
	void ret();
};