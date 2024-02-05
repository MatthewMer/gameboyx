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
#include "GraphicsMgr.h"

class GameboyGPU : protected BaseGPU {
public:
	friend class BaseGPU;

	// members
	void ProcessGPU(const int& _ticks) override;

private:
	// constructor
	GameboyGPU(graphics_information& _graphics_info, GraphicsMgr* _graphics_mgr) : graphicsInfo(_graphics_info) {
		memInstance = GameboyMEM::getInstance();
		graphicsCtx = memInstance->GetGraphicsContext();
		machineCtx = memInstance->GetMachineContext();
		graphicsMgr = _graphics_mgr;

		graphicsInfo.is2d = graphicsInfo.en2d = true;
		graphicsInfo.image_data = std::vector<u8>(PPU_SCREEN_X * PPU_SCREEN_Y * TEX2D_CHANNELS);
		graphicsInfo.aspect_ratio = LCD_ASPECT_RATIO;
		graphicsInfo.lcd_width = PPU_SCREEN_X;
		graphicsInfo.lcd_height = PPU_SCREEN_Y;

		if (machineCtx->isCgb) {
			DrawScanline = &GameboyGPU::DrawScanlineCGB;
		} else {
			DrawScanline = &GameboyGPU::DrawScanlineDMG;
		}

		graphicsMgr->Init2dGraphicsBackend();
	}
	// destructor
	~GameboyGPU() override {
		graphicsMgr->Destroy2dGraphicsBackend();
	}

	int GetDelayTime() const override;
	int GetTicksPerFrame(const float& _clock) const override;
	int GetFrames() override;
	void SetGraphicsParameters();

	// memory access
	GameboyMEM* memInstance = nullptr;
	graphics_context* graphicsCtx = nullptr;
	machine_context* machineCtx = nullptr;
	graphics_information& graphicsInfo;

	// members
	void EnterMode2();
	void EnterMode3();
	void EnterMode0();
	void EnterMode1();

	typedef void (GameboyGPU::* scanline_draw_function)(const u8& _ly);
	scanline_draw_function DrawScanline;
	void DrawScanlineDMG(const u8& _ly);
	void DrawScanlineCGB(const u8& _ly);

	void DrawBackgroundDMG(const u8& _ly);
	void DrawWindowDMG(const u8& _ly);
	void DrawObjectsDMG(const u8& _ly, const int* _objects, const int& _num_objects, const bool& _prio);

	void DrawTileOBJDMG(const int& _x, const int& _y, const u32* _color_palette, const bool& _prio, const bool& _x_flip);
	void DrawTileBGWINDMG(const int& _x, const int& _y, const u32* _color_palette);

	int OAMPrio1DMG[10];
	int numOAMEntriesPrio1DMG = 0;
	bool objPrio1DMG[PPU_SCREEN_X];
	int OAMPrio0DMG[10];
	int numOAMEntriesPrio0DMG = 0;

	int modeTickCounter = 0;
	int modeTickTarget = 0;
	int oamOffset = 0;

	bool statSignal = false;
	bool statSignalPrev = false;

	int mode3scxPause = 0;
	bool mode3scxPauseEn = false;

	void SearchOAM(const u8& _ly);
	void FetchTileDataOBJ(u8& _tile_offset, const int& _tile_sub_offset);

	void FetchTileDataBGWIN(u8& _tile_offset, const int& _tile_sub_offset);

	u8 tileDataCur[PPU_TILE_SIZE_SCANLINE];

	bool drawWindow = false;

	int mode3Dots;
};