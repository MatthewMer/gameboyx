#pragma once

#include "GraphicsUnitBase.h"
#include "MemorySM83.h"

class GraphicsUnitSM83 : protected GraphicsUnitBase
{
public:
	friend class GraphicsUnitBase;

	// members
	void NextFrame() override;

private:
	// constructor
	GraphicsUnitSM83() {
		mem_instance = MemorySM83::getInstance();
		graphics_ctx = mem_instance->GetGraphicsContext();
	}
	// destructor
	~GraphicsUnitSM83() = default;

	// memory access
	MemorySM83* mem_instance;
	graphics_context* graphics_ctx;

	// members
	void DrawTileMapBackground();
	void DrawTileMapWindow();
	void ReadTileDataBGWIN(const u8& _addr);
	void DrawTileBGWIN(const int& _pos_x, const int& _pos_y);

	void DrawObjectTiles();
	void ReadObjectAttributes(const int& _offset);
	void ReadTileDataObject(const u8& _addr, const bool& _mode16);
	void DrawObject(const int& _pos_x, const int& _pos_y, const bool& _mode16);

	void DrawPixel(const int& _pos_x, const int& _pos_y);

	u8 objAttributes[PPU_OBJ_ATTR_BYTES];

	// 8x8 pixel
	u8 currentTile[PPU_VRAM_TILE_SIZE];
	u8 currentTile16[PPU_VRAM_TILE_SIZE];
	u8 pixel;
};