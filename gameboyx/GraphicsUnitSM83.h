#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	The GraphicsUnitSM83 GPU class (in detail a PPU(Pixel Processing Unit), not directly a GPU in a modern context).
*	This class is purely there to emulate this GPU.
*/

#include "GraphicsUnitBase.h"
#include "MemorySM83.h"
#include "VulkanMgr.h"

class GraphicsUnitSM83 : protected GraphicsUnitBase {
public:
	friend class GraphicsUnitBase;

	// members
	void ProcessGPU(const int& _ticks) override;

private:
	// constructor
	GraphicsUnitSM83(graphics_information& _graphics_info, VulkanMgr* _graphics_mgr) : graphicsInfo(_graphics_info) {
		memInstance = MemorySM83::getInstance();
		graphicsCtx = memInstance->GetGraphicsContext();
		machineCtx = memInstance->GetMachineContext();
		graphicsMgr = _graphics_mgr;

		SetGraphicsParameters();
	}
	// destructor
	~GraphicsUnitSM83() = default;

	int GetDelayTime() const override;
	int GetTicksPerFrame(const float& _clock) const override;
	int GetFrames() override;
	void SetGraphicsParameters();

	// memory access
	MemorySM83* memInstance;
	graphics_context* graphicsCtx;
	machine_context* machineCtx;
	graphics_information& graphicsInfo;

	// members
	void EnterMode2();
	void EnterMode3();
	void EnterMode0();
	void EnterMode1();

	typedef void (GraphicsUnitSM83::* scanline_draw_function)(const u8& _ly);
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

	int mode3scxPause = 0;
	bool mode3scxPauseEn = false;

	void SearchOAM(const u8& _ly);
	void FetchTileDataOBJ(u8& _tile_offset, const int& _tile_sub_offset);

	void FetchTileDataBGWIN(u8& _tile_offset, const int& _tile_sub_offset);

	u8 tileDataCur[PPU_TILE_SIZE_SCANLINE];

	bool drawWindow = false;

	int mode3Dots;
};