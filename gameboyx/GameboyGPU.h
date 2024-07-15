#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	The GameboyGPU GPU class (in detail a PPU(Pixel Processing Unit), not directly a GPU in a modern context).
*	This class is purely there to emulate this GPU.
*/

#include "BaseGPU.h"
#include "GameboyMEM.h"
#include "GameboyCPU.h"
#include <atomic>

namespace Emulation {
	namespace Gameboy {
		class GameboyGPU : public BaseGPU {
		public:
			friend class BaseGPU;
			// constructor
			GameboyGPU(std::shared_ptr<BaseCartridge> _cartridge);
			~GameboyGPU() override;
			void Init() override;

			// members
			void ProcessGPU(const int& _ticks) override;

			void VRAMDMANextBlock();
			void OAMDMANextBlock();

			void SetMode(const int& _mode);

			void SetHardwareMode(const console_ids& _id);

			std::vector<std::tuple<int, std::string, bool>> GetGraphicsDebugSettings() override;
			void SetGraphicsDebugSetting(const bool& _val, const int& _id) override;

		private:
			int GetDelayTime() const override;
			int GetTicksPerFrame(const float& _clock) const override;

			// memory access
			std::weak_ptr<GameboyMEM> m_MemInstance;
			std::weak_ptr<GameboyCPU> m_CoreInstance;
			graphics_context* graphicsCtx = nullptr;
			machine_context* machineCtx = nullptr;

			// members
			void EnterMode2();
			void EnterMode3();
			void EnterMode0();
			void EnterMode1();

			typedef void (GameboyGPU::* ppu_function)(const u8& _ly);
			ppu_function DrawScanline;
			ppu_function SearchOam;
			void DrawScanlineDMG(const u8& _ly);
			void DrawScanlineCGB(const u8& _ly);

			void DrawBackgroundDMG(const u8& _ly);
			void DrawWindowDMG(const u8& _ly);
			void DrawObjectsDMG(const u8& _ly, const int* _objects, const int& _num_objects, const bool& _no_prio);

			void DrawTileOBJ(const int& _x, const int& _y, const u32* _color_palette, const bool& _no_prio, const bool& _x_flip);
			void DrawTileBGWINDMG(const int& _x, const int& _y, const u32* _color_palette);

			void DrawBackgroundCGB(const u8& _ly);
			void DrawWindowCGB(const u8& _ly);
			void DrawObjectsCGB(const u8& _ly, const int* _objects, const int& _num_objects, const bool& _no_prio);

			void DrawTileBGWINCGB(const int& _x, const int& _y, const u32* _color_palette, const bool& _x_flip, const bool& _prio);

			int OAMPrio0[10];
			int numOAMPrio0 = 0;
			std::vector<bool> objNoPrio = std::vector<bool>(PPU_SCREEN_X, false);
			int OAMPrio1[10];
			int numOAMPrio1 = 0;
			std::vector<bool> bgwinPrio = std::vector<bool>(PPU_SCREEN_X, false);

			int oamOffset = 0;

			bool statSignal = false;
			bool statSignalPrev = false;

			void SearchOAMDMG(const u8& _ly);
			void SearchOAMCGB(const u8& _ly);
			void FetchTileDataOBJ(u8& _tile_offset, const int& _tile_sub_offset, const int& _bank);

			void FetchTileDataBGWIN(u8& _tile_offset, const int& _tile_sub_offset, const int& _bank);

			u8 tileDataCur[PPU_TILE_SIZE_SCANLINE];

			bool drawWindow = false;

			int mode3Dots;

			alignas(64) std::atomic<bool> presentObjPrio0 = true;
			alignas(64) std::atomic<bool> presentObjPrio1 = true;
			alignas(64) std::atomic<bool> presentBackground = true;
			alignas(64) std::atomic<bool> presentWindow = true;

			bool presentObjPrio0Set = true;
			bool presentObjPrio1Set = true;
			bool presentBackgroundSet = true;
			bool presentWindowSet = true;
		};
	}
}