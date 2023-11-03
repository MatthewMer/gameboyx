#pragma once

#include "CoreBase.h"
#include "Cartridge.h"
#include "defs.h"
#include "MemorySM83.h"

#include <vector>

/* ***********************************************************************************************************
	STRUCTS
*********************************************************************************************************** */
struct registers {

	u8 F;
	u8 A;

	union {
		u16 BC = 0;
		struct {
			u8 C;
			u8 B;
		}BC_;
	};

	union {
		u16 DE = 0;
		struct {
			u8 E;
			u8 D;
		}DE_;
	};

	union {
		u16 HL = 0;
		struct {
			u8 L;
			u8 H;
		}HL_;
	};

	u16 SP = 0;						// stack pointer
	u16 PC = 0;						// program counter
};

/* ***********************************************************************************************************
	CLASSES FOR INSTRUCTION IN/OUTPUT POINTERS
*********************************************************************************************************** */
enum cgb_data_types {
	NO_DATA,
	BC,
	BC_ref,
	DE,
	DE_ref,
	HL,
	HL_ref,
	HL_INC_ref,
	HL_DEC_ref,
	SP,
	SP_r8,
	PC,
	AF,
	A,
	F,
	B,
	C,
	C_ref,
	D,
	E,
	H,
	L,
	IE,
	IF,
	d8,
	d16,
	a8,
	a8_ref,
	a16,
	a16_ref,
	r8
};

enum cgb_flag_types {
	FLAG_Z,
	FLAG_N,
	FLAG_C,
	FLAG_H,
	FLAG_IME,
	INT_VBLANK,
	INT_STAT,
	INT_TIMER,
	INT_SERIAL,
	INT_JOYPAD
};

/* ***********************************************************************************************************
	CoreSM83 CLASS DECLARATION
*********************************************************************************************************** */
class CoreSM83 : protected CoreBase
{
public:
	friend class CoreBase;

	void RunCycles() override;
	void RunCycle() override;
	void GetCurrentHardwareState() const override;
	void GetStartupHardwareInfo() const override;
	bool CheckStep() override;
	void ResetStep() override;
	u32 GetCurrentClockCycles() override;

	void GetCurrentProgramCounter() override;
	void InitMessageBufferProgram() override;
	void InitMessageBufferProgramTmp() override;
	void GetCurrentRegisterValues() const  override;
	void GetCurrentFlagsAndISR() const override;

private:
	// constructor
	explicit CoreSM83(machine_information& _machine_info);
	// destructor
	~CoreSM83() = default;

	// instruction data
	u8 opcode;
	u16 data;

	void RunCpu() override;
	void ExecuteInstruction() override;
	void CheckInterrupts() override;
	void TickTimers();
	void IncrementTIMA();

	void FetchOpCode();
	void Fetch8Bit();
	void Fetch16Bit();

	void Write8Bit(const u8& _data, const u16& _addr) override;
	void Write16Bit(const u16& _data, const u16& _addr) override;
	u8 Read8Bit(const u16& _addr) override;
	u16 Read16Bit(const u16& _addr) override;

	u16 curPC;

	// internals
	registers Regs = registers();
	void InitRegisterStates() override;
	void InitCpu() override;

	// cpu states and checks
	bool halted = false;
	bool stopped = false;
	bool ime = false;

	bool timaEnAndDivOverflowPrev = false;
	bool timaEnAndDivOverflowCur = false;

	// ISR
	void isr_push(const u16& _isr_handler);

	// instruction members ****************
	// lookup table
	typedef void (CoreSM83::* instruction)();
	// raw data, decoded data

	// current instruction context
	using instr_tuple = std::tuple <const u8, const instruction, const int, const std::string, const cgb_data_types, const cgb_data_types>;
	instr_tuple* instrPtr = nullptr;
	instruction functionPtr = nullptr;
	int machineCycleScanlineCounter = 0;
	int currentMachineCycles = 0;
	int GetDelayTime() override;
	int GetStepsPerFrame() override;
	void GetCurrentInstruction() const override;
	void DecodeRomBankContent(ScrollableTableBuffer<debug_instr_data>& _program_buffer, const std::pair<int, std::vector<u8>>& _bank_data, const int& _bank_num);
	void DecodeBankContent(ScrollableTableBuffer<debug_instr_data>& _program_buffer, const std::pair<int, std::vector<u8>>& _bank_data, const int& _bank_num, const std::string& _bank_name);

	machine_state_context* machine_ctx;
	MemorySM83* mem_instance;

	// basic instructions
	std::vector<instr_tuple> instrMap;
	void setupLookupTable();

	// CB instructions
	std::vector<instr_tuple> instrMapCB;
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

	void LDHLSPr8();

	void PUSH();
	void POP();

	void stack_push(const u16& _data);
	u16 stack_pop();

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