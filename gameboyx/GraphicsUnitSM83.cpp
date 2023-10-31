#include "GraphicsUnitSM83.h"

#include "gameboy_defines.h"
#include "logger.h"

#include "vulkan/vulkan.h"

// VK_FORMAT_R8G8B8A8_UNORM, gray scales
const u32 DMG_COLOR_PALETTE[] = {
	0x000000FF,
	0x555555FF,
	0xAAAAAAFF,
	0xFFFFFFFF
};

void GraphicsUnitSM83::NextFrame() {
	if (graphics_ctx->ppu_enable) {
		isrFlags = 0x00;

		// generate frame data
		if (graphics_ctx->isCgb) {
			if (graphics_ctx->bg_win_enable) {
				DrawTileMapBackground();
			}
			if (graphics_ctx->win_enable) {
				DrawTileMapWindow();
			}
		}
		else {
			if (graphics_ctx->bg_win_enable) {
				DrawTileMapBackground();
				if (graphics_ctx->win_enable) {
					DrawTileMapWindow();
				}
			}
		}
		if (graphics_ctx->obj_enable) {
			DrawObjectTiles();
		}
		/*
		// everything from here just pretends to have processed the scanlines (horizontal lines) of the LCD *****
		// request mode 0 to 2 interrupts (STAT)
		if (*graphics_ctx->STAT & (PPU_STAT_MODE0_EN | PPU_STAT_MODE1_EN | PPU_STAT_MODE2_EN)) {
			isrFlags |= IRQ_LCD_STAT;
		}

		// request ly compare interrupt (STAT) and set flag
		if (*graphics_ctx->LYC <= LCD_SCANLINES_TOTAL) {
			*graphics_ctx->STAT |= PPU_STAT_LYC_FLAG;
			if (*graphics_ctx->STAT & PPU_STAT_LYC_SOURCE) {
				isrFlags |= IRQ_LCD_STAT;
			}
		}
		else {
			*graphics_ctx->STAT &= ~PPU_STAT_LYC_FLAG;
		}

		mem_instance->RequestInterrupts(isrFlags | IRQ_VBLANK);*/

		// TODO: let cpu run for machine cycles per scan line, render frame after last scanline, set LY to 0x90 after first 17500 machine cycles (via increment)
		// currently directly set to 0x90 only for testing with blargg's instruction tests
		mem_instance->SetIOValue(0x90, LY_ADDR);//LCD_VBLANK_THRESHOLD;
	}
}

// draw tilemaps BG and WIN
void GraphicsUnitSM83::DrawTileMapBackground() {
	int scx = mem_instance->GetIOValue(SCX_ADDR);
	int scy = mem_instance->GetIOValue(SCY_ADDR);

	for (int x = 0; x < PPU_SCREEN_X; x+= PPU_PIXELS_TILE_X) {
		for (int y = 0; y < PPU_SCREEN_Y; y+= PPU_PIXELS_TILE_Y) {
			u8 tile_index = graphics_ctx->VRAM_N[0][
				graphics_ctx->bg_tilemap_offset +
				((scy + y) % PPU_TILEMAP_SIZE_1D) * PPU_TILEMAP_SIZE_1D + 
				(scx + x) % PPU_TILEMAP_SIZE_1D
			];
			ReadTileDataBGWIN(tile_index);
			if (graphics_ctx->isCgb) {
				ReadBGMapAttributes(tile_index);
			}
			DrawTileBackground(scx + x, scy + y);
		}
	}
}

// in CGB mode: copy attributes
void GraphicsUnitSM83::ReadBGMapAttributes(const u8& _offset) {
	if (graphics_ctx->bg_win_8800_addr_mode) {
		bgAttributes = graphics_ctx->VRAM_N[1][PPU_VRAM_BASEPTR_8800 - VRAM_N_OFFSET + (*(i8*)&_offset * PPU_VRAM_TILE_SIZE)];
	}
	else {
		bgAttributes = graphics_ctx->VRAM_N[1][_offset * PPU_VRAM_TILE_SIZE];
	}
}

// draw stored tile to screen
void GraphicsUnitSM83::DrawTileBackground(const int& _pos_x, const int& _pos_y) {
	// TODO: check attributes in CGB mode

	for (int y = 0; y < PPU_VRAM_TILE_SIZE / 2; y += 2) {
		for (int x = 0; x < 8; x++) {
			pixel = (currentTile[y] & (0x80 >> x) ? 0x01 : 0x00);
			pixel |= (currentTile[y + 1] & (0x80 >> x) ? 0x02 : 0x00);
			DrawPixel(_pos_x + x, _pos_y + y);
		}
	}
}

// draw window (window pos at top left corner is (WX-7/WY) !)
void GraphicsUnitSM83::DrawTileMapWindow() {
	u8 win_offset = graphics_ctx->win_tilemap_offset;
	int wx = (int)mem_instance->GetIOValue(WX_ADDR) - 7;
	int wy = mem_instance->GetIOValue(WY_ADDR);

	for (int x = 0; x < PPU_TILES_HORIZONTAL - wx; x+=PPU_PIXELS_TILE_X) {
		for (int y = 0; y < PPU_TILES_VERTICAL - wy; y+=PPU_PIXELS_TILE_Y) {
			u8 tile_index = graphics_ctx->VRAM_N[0][win_offset + (y * PPU_TILEMAP_SIZE_1D) + x],
			ReadTileDataBGWIN(tile_index);
			DrawTileWindow(x, y);
		}
	}
}

// draw stored tile to screen
void GraphicsUnitSM83::DrawTileWindow(const int& _pos_x, const int& _pos_y) {
	for (int y = 0; y < PPU_VRAM_TILE_SIZE / 2; y += 2) {
		for (int x = 0; x < 8; x++) {
			pixel = (currentTile[y] & (0x80 >> x) ? 0x01 : 0x00);
			pixel |= (currentTile[y + 1] & (0x80 >> x) ? 0x02 : 0x00);
			DrawPixel(_pos_x + x, _pos_y + y);
		}
	}
}

// copy tile data (pixels) for BG and WIN
void GraphicsUnitSM83::ReadTileDataBGWIN(const u8& _offset) {
	if (graphics_ctx->bg_win_8800_addr_mode) {
		memcpy(currentTile, &graphics_ctx->VRAM_N[0][PPU_VRAM_BASEPTR_8800 - VRAM_N_OFFSET + (*(i8*)&_offset * PPU_VRAM_TILE_SIZE)], PPU_VRAM_TILE_SIZE);
	}
	else {
		memcpy(currentTile, &graphics_ctx->VRAM_N[0][_offset * PPU_VRAM_TILE_SIZE], PPU_VRAM_TILE_SIZE);
	}
}

// draw the objects to the screen
void GraphicsUnitSM83::DrawObjectTiles() {
	for (int i = 0; i < PPU_OBJ_ATTR_SIZE; i += PPU_OBJ_ATTR_BYTES) {
		ReadObjectAttributes(i);
		ReadTileDataObject(objAttributes[OBJ_ATTR_INDEX], graphics_ctx->obj_size_16);
		DrawObject((int)objAttributes[OBJ_ATTR_X] - 8, (int)objAttributes[OBJ_ATTR_Y] - 16, graphics_ctx->obj_size_16);
	}
}

// copy object attributes
void GraphicsUnitSM83::ReadObjectAttributes(const int& _offset){
	memcpy(objAttributes, &graphics_ctx->OAM[_offset], PPU_OBJ_ATTR_BYTES);
}

// copy tile data (pixels) for OBJ
void GraphicsUnitSM83::ReadTileDataObject(const u8& _offset, const bool& _mode16) {
	int vram_bank_selected;

	switch (graphics_ctx->isCgb) {
	case true:
		vram_bank_selected = (objAttributes[OBJ_ATTR_FLAGS] & OBJ_ATTR_VRAM_BANK ? 1 : 0);
		break;
	case false:
		vram_bank_selected = 0;
		break;
	}

	memcpy(currentTile, &graphics_ctx->VRAM_N[vram_bank_selected][_offset * PPU_VRAM_TILE_SIZE], PPU_VRAM_TILE_SIZE);
	if(_mode16){
		memcpy(currentTile16, &graphics_ctx->VRAM_N[vram_bank_selected][(_offset + 1) * PPU_VRAM_TILE_SIZE], PPU_VRAM_TILE_SIZE);
	}
}

// draw stored object to screen
void GraphicsUnitSM83::DrawObject(const int& _pos_x, const int& _pos_y, const bool& _mode16) {
	for (int y = 0; y < PPU_VRAM_TILE_SIZE / 2; y += 2) {
		for (int x = 0; x < 8; x++) {
			pixel = (currentTile[y] & (0x80 >> x) ? 0x01 : 0x00);
			pixel |= (currentTile[y + 1] & (0x80 >> x) ? 0x02 : 0x00);
			DrawPixel(_pos_x + x, _pos_y + y);
		}
	}

	if (_mode16) {
		for (int y = 0; y < PPU_VRAM_TILE_SIZE / 2; y += 2) {
			for (int x = 0; x < 8; x++) {
				pixel = (currentTile16[y] & (0x80 >> x) ? 0x01 : 0x00);
				pixel |= (currentTile16[y + 1] & (0x80 >> x) ? 0x02 : 0x00);
				DrawPixel(_pos_x + x, _pos_y + PPU_PIXELS_TILE_Y + y);
			}
		}
	}
}

// draw pixel to screen
void GraphicsUnitSM83::DrawPixel(const int& _pos_x, const int& _pos_y) {
	// TODO: draw pixel to screen based on color palette
	if (graphics_ctx->isCgb) {

	}
	else {
		DMG_COLOR_PALETTE[pixel];
	}
}