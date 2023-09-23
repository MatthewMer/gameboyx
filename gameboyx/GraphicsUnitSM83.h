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
	void ReadTileDataBGWIN(const u8& _addr);
	void ReadTileDataObject(const u8& _addr);

	void DrawSprite();

	void DrawTileMapBackground();
	void DrawTileMapWindow();

	// 8x8 pixel
	u8 currentTile[PPU_VRAM_TILE_SIZE];
	u8 pixel;
};