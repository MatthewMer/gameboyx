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

class GraphicsUnitSM83 : protected GraphicsUnitBase
{
public:
	friend class GraphicsUnitBase;

	// members
	void NextFrame() override;
	bool ProcessGPU() override;

private:
	// constructor
	GraphicsUnitSM83(VulkanMgr* _graphics_mgr, graphics_information& _graphics_info) : graphicsMgr(_graphics_mgr), graphicsInfo(_graphics_info) {
		memInstance = MemorySM83::getInstance();
		graphicsCtx = memInstance->GetGraphicsContext();

		SetGraphicsParameters();
	}
	// destructor
	~GraphicsUnitSM83() = default;

	void SetGraphicsParameters();

	// memory access
	MemorySM83* memInstance;
	graphics_context* graphicsCtx;
	graphics_information& graphicsInfo;
	VulkanMgr* graphicsMgr;

	// members
	void DrawTileMapBackground();
	void ReadBGMapAttributes(const u8& _offset);
	void DrawTileBackground(const int& _pos_x, const int& _pos_y);

	void DrawTileMapWindow();
	void DrawTileWindow(const int& _pos_x, const int& _pos_y);

	void ReadTileDataBGWIN(const u8& _addr);

	void DrawObjectTiles();
	void ReadObjectAttributes(const int& _offset);
	void ReadTileDataObject(const u8& _addr, const bool& _mode16);
	void DrawObject(const int& _pos_x, const int& _pos_y, const bool& _mode16);

	void DrawPixel(const int& _pos_x, const int& _pos_y);

	u8 objAttributes[PPU_OBJ_ATTR_BYTES];
	u8 bgAttributes;

	// 8x8 pixel
	u8 currentTile[PPU_VRAM_TILE_SIZE];
	u8 currentTile16[PPU_VRAM_TILE_SIZE];
	u8 pixel;
};