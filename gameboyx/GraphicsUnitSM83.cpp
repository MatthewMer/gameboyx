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

// search oam for objects
void GraphicsUnitSM83::ReadTileDataBGWIN(const u8& _offset) {
	if (graphics_ctx->LCDC & PPU_LCDC_WINBG_TILEDATA) {
		memcpy(currentTile, &graphics_ctx->VRAM_N[graphics_ctx->VRAM_BANK][_offset], PPU_VRAM_TILE_SIZE);
	}
	else {
		memcpy(currentTile, &graphics_ctx->VRAM_N[graphics_ctx->VRAM_BANK][PPU_VRAM_BASEPTR_8800 - VRAM_N_OFFSET + *(i8*)&_offset], PPU_VRAM_TILE_SIZE);
	}
}

void GraphicsUnitSM83::ReadTileDataObject(const u8& _offset) {
	memcpy(currentTile, &graphics_ctx->VRAM_N[graphics_ctx->VRAM_BANK][_offset], PPU_VRAM_TILE_SIZE);
}

void GraphicsUnitSM83::DrawSprite() {
	for (int i = 0; i < PPU_VRAM_TILE_SIZE / 2; i+=2) {
		for (int j = 0; j < 8; j++) {
			pixel = (currentTile[i] & (0x80 >> j) ? 0x01 : 0x00);
			pixel |= (currentTile[i + 1] & (0x80 >> j) ? 0x02 : 0x00);
		}
	}
}

void GraphicsUnitSM83::DrawTileMapBackground() {
	u8 bg_offset = graphics_ctx->bg_tilemap_offset;
	u8 scx = graphics_ctx->SCX;
	u8 scy = graphics_ctx->SCY;
	
	for (int x = 0; x < PPU_SCREEN_X; x++) {
		for (int y = 0; y < PPU_SCREEN_Y; y++) {
			u8 tile_index = graphics_ctx->VRAM_N[0][bg_offset + ((scy + y) * PPU_SCREEN_X) % PPU_SCREEN_Y + (scx + x) % PPU_SCREEN_X];
			ReadTileDataBGWIN(tile_index);
			DrawSprite();
		}
	}
}

void GraphicsUnitSM83::DrawTileMapWindow() {
	u8 win_offset = graphics_ctx->win_tilemap_offset;
	u8 wx = graphics_ctx->WX;
	u8 wy = graphics_ctx->WY;

	for (int x = wx + 7; x < PPU_SCREEN_X; x++) {
		for (int y = wy; y < PPU_SCREEN_Y; y++) {
			u8 tile_index = graphics_ctx->VRAM_N[0][win_offset + (y * PPU_SCREEN_X) + x];
			ReadTileDataBGWIN(tile_index);
			DrawSprite();
		}
	}
}