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

/* ***********************************************************************************************************
    IO REGISTER DEFINES
*********************************************************************************************************** */
// joypad
#define JOYP_ADDR                   0xFF00              // Mixed

// timers
#define DIV_ADDR                    0xFF04              // RW
#define TIMA_ADDR                   0xFF05              // RW
#define TMA_ADDR                    0xFF06              // RW
#define TAC_ADDR                    0xFF07              // RW
#define TAC_CLOCK_SELECT            0x03
#define TAC_CLOCK_ENABLE            0x04

// interrupt flags
#define IF_ADDR                     0xFF0F              // RW

#define ISR_VBLANK                  0x01
#define ISR_LCD_STAT                0x02
#define ISR_TIMER                   0x04
#define ISR_SERIAL                  0x08
#define ISR_JOYPAD                  0x10

#define ISR_VBLANK_HANDLER          0x40
#define ISR_LCD_STAT_HANDLER        0x48
#define ISR_TIMER_HANDLER           0x50
#define ISR_SERIAL_HANDLER          0x58
#define ISR_JOYPAD_HANDLER          0x60

// sound
#define NR10_ADDR                   0xFF10              // RW
#define NR11_ADDR                   0xFF11              // Mixed
#define NR12_ADDR                   0xFF12              // RW
#define NR13_ADDR                   0xFF13              // W
#define NR14_ADDR                   0xFF14              // Mixed

#define NR21_ADDR                   0xFF16              // Mixed
#define NR22_ADDR                   0xFF17              // RW
#define NR23_ADDR                   0xFF18              // W
#define NR24_ADDR                   0xFF19              // Mixed

#define NR30_ADDR                   0xFF1A              // RW
#define NR31_ADDR                   0xFF1B              // W
#define NR32_ADDR                   0xFF1C              // RW
#define NR33_ADDR                   0xFF1D              // W
#define NR34_ADDR                   0xFF1E              // Mixed

#define NR41_ADDR                   0xFF20              // W
#define NR42_ADDR                   0xFF21              // RW
#define NR43_ADDR                   0xFF22              // RW
#define NR44_ADDR                   0xFF23              // Mixed

#define NR50_ADDR                   0xFF24              // RW
#define NR51_ADDR                   0xFF25              // RW
#define NR52_ADDR                   0xFF26              // Mixed

#define WAVE_RAM_ADDR               0xFF30              // RW
#define WAVE_RAM_SIZE               0x10

// LCD
#define LCDC_ADDR                   0xFF40              // RW
#define STAT_ADDR                   0xFF41              // Mixed
#define SCY_ADDR                    0xFF42              // RW
#define SCX_ADDR                    0xFF43              // RW
#define LY_ADDR                     0xFF44              // R
#define LYC_ADDR                    0xFF45              // RW

// OAM DMA
#define OAM_DMA_ADDR                0xFF46              // RW
#define OAM_DMA_SOURCE_MAX          0xDF
#define OAM_DMA_LENGTH              0xA0

// BG/OBJ PALETTES
#define BGP_ADDR                    0xFF47              // RW
#define OBP0_ADDR                   0xFF48              // RW
#define OBP1_ADDR                   0xFF49              // RW

// WINDOW POS
#define WY_ADDR                     0xFF4A              // RW
#define WX_ADDR                     0xFF4B              // RW

// SPEED SWITCH
#define CGB_SPEED_SWITCH_ADDR       0xFF4D              // Mixed, affects: CPU 1MHz -> 2.1MHz, timer/divider, OAM DMA(not needed)
#define SPEED                       0x80
#define NORMAL_SPEED                0x00
#define DOUBLE_SPEED                0x80
#define PREPARE_SPEED_SWITCH        0x01

// VRAM BANK SELECT
#define CGB_VRAM_SELECT_ADDR        0xFF4F              // RW

// VRAM DMA
#define CGB_HDMA1_ADDR              0xFF51              // W
#define CGB_HDMA2_ADDR              0xFF52              // W
#define CGB_HDMA3_ADDR              0xFF53              // W
#define CGB_HDMA4_ADDR              0xFF54              // W
#define CGB_HDMA5_ADDR              0xFF55              // RW
#define DMA_MODE_BIT                0x80
#define DMA_MODE_GENERAL            0x00
#define DMA_MODE_HBLANK             0x80

// BG/OBJ PALETTES SPECS
#define BCPS_BGPI_ADDR              0xFF68              // RW
#define BCPD_BGPD_ADDR              0xFF69              // RW
#define OCPS_OBPI_ADDR              0xFF6A              // RW
#define OCPD_OBPD_ADDR              0xFF6B              // RW

// OBJECT PRIORITY MODE
#define CGB_OBJ_PRIO_ADDR           0xFF6C              // RW
#define PRIO_MODE                   0x01
#define PRIO_OAM                    0x00
#define PRIO_COORD                  0x01

// WRAM BANK SELECT
#define CGB_WRAM_SELECT_ADDR        0xFF70              // RW

// DIGITAL AUDIO OUT
#define PCM12_ADDR                  0xFF76              // R
#define PCM34_ADDR                  0xFF77              // R

/* ***********************************************************************************************************
    FREQUENCIES
*********************************************************************************************************** */
#define DIV_FREQUENCY               16384               // Hz
#define BASE_CLOCK_CPU              4.20f               // MHz
#define DISPLAY_FREQUENCY           60                  // Hz

/* ***********************************************************************************************************
    GRAPHICS DEFINES
*********************************************************************************************************** */
#define PPU_VBLANK                  0x91

// LCD CONTROL
#define PPU_LCDC_ENABLE             0x80
#define PPU_LCDC_WIN_TILEMAP        0x40
#define PPU_LCDC_WIN_ENABLE         0x20
#define PPU_LCDC_WINBG_TILEDATA     0x10
#define PPU_LCDC_BG_TILEMAP         0x08
#define PPU_LCDC_OBJ_SIZE           0x04
#define PPU_LCDC_OBJ_ENABLE         0x02
#define PPU_LCDC_WINBG_EN_PRIO      0x01

// LCD STAT
#define PPU_STAT_MODE               0x03
//#define PPU_STAT_MODE0_HBLANK       0x00
#define PPU_STAT_MODE1_VBLANK       0x01
//#define PPU_STAT_MODE2_OAMSEARCH    0x02
//#define PPU_STAT_MODE3_LCDCTRL      0x03

#define PPU_STAT_LYC_FLAG           0x04
#define PPU_STAT_MODE0_EN           0x08                // isr STAT hblank
#define PPU_STAT_MODE1_EN           0x10                // isr STAT vblank
#define PPU_STAT_MODE2_EN           0x20                // isr STAT oam
#define PPU_STAT_LYC_SOURCE         0x40

// lcd
#define LCD_SCANLINES               144
#define LCD_SCANLINES_VBLANK        153

// VRAM Tile data
#define PPU_VRAM_TILE_SIZE          16                  // Bytes
#define PPU_VRAM_BASEPTR_8800       0x9000

// tile maps
#define PPU_TILE_MAP0               0x9800
#define PPU_TILE_MAP1               0x9C00

#define PPU_OBJ_ATTRIBUTES          0xFE00
#define PPU_OBJ_ATTR_SIZE           0xA0
#define PPU_OBJ_ATTR_BYTES          4

#define OBJ_ATTR_Y                  0
#define OBJ_ATTR_X                  1
#define OBJ_ATTR_INDEX              2
#define OBJ_ATTR_FLAGS              3

#define OBJ_ATTR_PALETTE            0x07
#define OBJ_ATTR_VRAM_BANK          0x08

// screen
#define PPU_TILES_HORIZONTAL        20
#define PPU_TILES_VERTICAL          18
#define PPU_PIXELS_TILE_X           8
#define PPU_PIXELS_TILE_Y           8
#define PPU_SCREEN_X                160
#define PPU_SCREEN_Y                144