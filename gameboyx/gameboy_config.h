#pragma once

/* ***********************************************************************************************************
    PROCESSOR AND ROM DEFINES
*********************************************************************************************************** */
// ROM HEADER *****
#define ROM_HEAD_ADDR				0x0100
#define ROM_HEAD_SIZE				0x0050

#define ROM_HEAD_TITLE				0x0134
#define ROM_HEAD_CGBFLAG			0x0143				// until here; 0x80 = supports CGB, 0xC0 = CGB only		(bit 6 gets ignored)
#define ROM_HEAD_NEWLIC				0x0144				// 2 char ascii; only if oldlic = 0x33
#define ROM_HEAD_SGBFLAG			0x0146				// 0x03 = en, else not
#define ROM_HEAD_HW_TYPE			0x0147				// hardware on cartridge
#define ROM_HEAD_ROMSIZE			0x0148
#define ROM_HEAD_RAMSIZE			0x0149
#define ROM_HEAD_DEST				0x014A				// 0x00 = japan, 0x01 = overseas
#define ROM_HEAD_OLDLIC				0x014B
#define ROM_HEAD_VERSION			0x014C
#define ROM_HEAD_CHKSUM				0x014D

#define ROM_HEAD_TITLE_SIZE_CGB		15
#define ROM_HEAD_TITLE_SIZE_GB		16

// EXTERNAL HARDWARE *****
#define ROM_BASE_SIZE               0x8000              // 32 KiB
#define RAM_BASE_SIZE               0x2000              // 8 KiB 

// CPU DEFINES *****

// initial register values (CGB in CGB and DMG mode)
#define CGB_AF                      0x1180
#define CGB_BC                      0x0000
#define CGB_SP                      0xFFFE
#define CGB_PC                      0x0100

#define CGB_CGB_DE                  0xFF56
#define CGB_CGB_HL                  0x000D

#define CGB_DMG_DE                  0x0008
#define CGB_DMG_HL                  0x007C

// SM83 MEMORY MAPPING *****
#define ROM_BANK_0_OFFSET           0x0000
#define ROM_BANK_0_SIZE             0x4000

#define ROM_BANK_N_OFFSET           0x4000
#define ROM_BANK_N_SIZE             0x4000

#define VRAM_N_OFFSET               0x8000              // CGB mode: switchable (0-1)
#define VRAM_N_SIZE                 0x2000

#define RAM_BANK_N_OFFSET           0xA000              // external (Cartridge)
#define RAM_BANK_N_SIZE             0x2000

#define WRAM_0_OFFSET               0xC000
#define WRAM_0_SIZE                 0x1000

#define WRAM_N_OFFSET               0xD000              // CGB mode: switchable (1-7)
#define WRAM_N_SIZE                 0x1000

#define MIRROR_WRAM_OFFSET          0xE000              // not used (mirror of WRAM 0)
#define MIRROR_WRAM_SIZE            0x1E00

#define OAM_OFFSET                  0xFE00              // object attribute memory
#define OAM_SIZE                    0x00A0

#define NOT_USED_MEMORY_OFFSET      0xFEA0              // prohibited
#define NOT_USED_MEMORY_SIZE        0x0060

#define IO_REGISTERS_OFFSET         0xFF00              // peripherals
#define IO_REGISTERS_SIZE           0x0080

#define HRAM_OFFSET                 0xFF80              // stack, etc.
#define HRAM_SIZE                   0x007F

#define IE_OFFSET                   0xFFFF