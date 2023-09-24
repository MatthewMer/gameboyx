#include "GraphicsUnitSM83.h"

#include "gameboy_defines.h"
#include "logger.h"

void GraphicsUnitSM83::NextFrame() {
	if (graphics_ctx->LCDC & PPU_LCDC_ENABLE) {
		isrFlags = 0x00;

		// generate frame data

		// TODO: render result to texture or prepare data for vulkan renderpass

		// everything from here just pretends to have processed the scanlines (horizontal lines) of the LCD *****
		// request mode 0 to 2 interrupts (STAT)
		if (graphics_ctx->STAT & (PPU_STAT_MODE0_EN | PPU_STAT_MODE1_EN | PPU_STAT_MODE2_EN)) {
			isrFlags |= ISR_LCD_STAT;
		}

		// request ly compare interrupt (STAT) and set flag
		if (graphics_ctx->LYC <= LCD_SCANLINES_VBLANK) {
			graphics_ctx->STAT |= PPU_STAT_LYC_FLAG;
			if (graphics_ctx->STAT & PPU_STAT_LYC_SOURCE) {
				isrFlags |= ISR_LCD_STAT;
			}
		}
		else {
			graphics_ctx->STAT &= ~PPU_STAT_LYC_FLAG;
		}

		mem_instance->RequestInterrupts(isrFlags | ISR_VBLANK);
		graphics_ctx->LY = PPU_VBLANK;
	}
}

// draw tilemaps BG and WIN
void GraphicsUnitSM83::DrawTileMapBackground() {
	int bg_offset = graphics_ctx->bg_tilemap_offset;
	int scx = graphics_ctx->SCX;
	int scy = graphics_ctx->SCY;

	for (int x = 0; x < PPU_TILES_HORIZONTAL; x++) {
		for (int y = 0; y < PPU_TILES_VERTICAL; y++) {
			u8 tile_index = graphics_ctx->VRAM_N[0][bg_offset + ((scy + (y * PPU_PIXELS_TILE_Y)) % PPU_SCREEN_Y) * PPU_SCREEN_X + (scx + (x * PPU_PIXELS_TILE_X)) % PPU_SCREEN_X];
			ReadTileDataBGWIN(tile_index);
			DrawTileBGWIN(x, y);
		}
	}
}

void GraphicsUnitSM83::DrawTileMapWindow() {
	u8 win_offset = graphics_ctx->win_tilemap_offset;
	int wx = graphics_ctx->WX;
	int wy = graphics_ctx->WY;

	for (int x = (wx > 7 ? wx : -wx + 7); x < (wx > 7 ? PPU_SCREEN_X - (wx - 7) : PPU_SCREEN_X); x += PPU_PIXELS_TILE_X) {
		for (int y = 0; y < (graphics_ctx->WY > 0 ? PPU_SCREEN_Y - graphics_ctx->WY : PPU_SCREEN_Y); y += PPU_PIXELS_TILE_Y) {
			u8 tile_index = graphics_ctx->VRAM_N[0][win_offset + (y * PPU_SCREEN_X) + x];
			ReadTileDataBGWIN(tile_index);
			DrawTileBGWIN(x, y);
		}
	}
}

// copy tile data (pixels) for BG and WIN
void GraphicsUnitSM83::ReadTileDataBGWIN(const u8& _offset) {
	if (graphics_ctx->LCDC & PPU_LCDC_WINBG_TILEDATA) {
		memcpy(currentTile, &graphics_ctx->VRAM_N[graphics_ctx->VRAM_BANK][_offset * PPU_VRAM_TILE_SIZE], PPU_VRAM_TILE_SIZE);
	}
	else {
		memcpy(currentTile, &graphics_ctx->VRAM_N[graphics_ctx->VRAM_BANK][PPU_VRAM_BASEPTR_8800 - VRAM_N_OFFSET + (*(i8*)&_offset * PPU_VRAM_TILE_SIZE)], PPU_VRAM_TILE_SIZE);
	}
}

// draw the objects to the screen
void GraphicsUnitSM83::DrawObjectTiles() {
	for (int i = 0; i < PPU_OBJ_ATTR_SIZE; i+= PPU_OBJ_ATTR_BYTES) {
		ReadObjectAttributes(i);

		bool mode16 = graphics_ctx->LCDC & PPU_LCDC_OBJ_SIZE;
		ReadTileDataObject(objAttributes[OBJ_ATTR_INDEX], mode16);

		DrawObject((int)objAttributes[OBJ_ATTR_X] - 8, (int)objAttributes[OBJ_ATTR_Y] - 16, mode16);
	}
}

// copy object attributes
void GraphicsUnitSM83::ReadObjectAttributes(const int& _offset){
	memcpy(objAttributes, &graphics_ctx->OAM[_offset], PPU_OBJ_ATTR_BYTES);
}

// copy tile data (pixels) for OBJ
void GraphicsUnitSM83::ReadTileDataObject(const u8& _offset, const bool& _mode16) {
	int vram_bank;

	switch (graphics_ctx->isCgb) {
	case true:
		vram_bank = (objAttributes[OBJ_ATTR_FLAGS] & OBJ_ATTR_VRAM_BANK ? 1 : 0);
		break;
	case false:
		vram_bank = 0;
		break;
	}

	memcpy(currentTile, &graphics_ctx->VRAM_N[vram_bank][_offset * PPU_VRAM_TILE_SIZE], PPU_VRAM_TILE_SIZE);
	if(_mode16){
		memcpy(currentTile16, &graphics_ctx->VRAM_N[vram_bank][(_offset + 1) * PPU_VRAM_TILE_SIZE], PPU_VRAM_TILE_SIZE);
	}
}

// draw stored tile to screen
void GraphicsUnitSM83::DrawTileBGWIN(const int& _pos_x, const int& _pos_y) {
	for (int y = 0; y < PPU_VRAM_TILE_SIZE / 2; y+=2) {
		for (int x = 0; x < 8; x++) {
			pixel = (currentTile[y] & (0x80 >> x) ? 0x01 : 0x00);
			pixel |= (currentTile[y + 1] & (0x80 >> x) ? 0x02 : 0x00);
			DrawPixel(_pos_x + x, _pos_y + y);
		}
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
}