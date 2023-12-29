#include "GraphicsUnitSM83.h"

#include "gameboy_defines.h"
#include "logger.h"
#include <format>

#include "vulkan/vulkan.h"

using namespace std;

// VK_FORMAT_R8G8B8A8_UNORM, gray scales
const u32 DMG_COLOR_PALETTE[] = {
	0x000000FF,
	0x555555FF,
	0xAAAAAAFF,
	0xFFFFFFFF
};

void GraphicsUnitSM83::SetGraphicsParameters() {
	graphicsInfo.is2d = graphicsInfo.en2d = true;
	graphicsInfo.lcd_width = PPU_SCREEN_X;
	graphicsInfo.lcd_height = PPU_SCREEN_Y;
	graphicsInfo.image_data = vector<u8>(PPU_SCREEN_X * PPU_SCREEN_Y * 4);
	graphicsInfo.aspect_ratio = LCD_ASPECT_RATIO;
}

bool GraphicsUnitSM83::ProcessGPU() {
	if (graphicsCtx->ppu_enable) {
		u8 interrupts = 0x00;
		u8& ly = memInstance->GetIORef(LY_ADDR);
		u8& lyc = memInstance->GetIORef(LYC_ADDR);
		u8& stat = memInstance->GetIORef(STAT_ADDR);

		ly++;

		switch (ly) {
		case LCD_SCANLINES_VBLANK:
			SetMode(MODE_1, stat, interrupts);
			break;
		case LCD_SCANLINES_TOTAL:
			ly = 0x00;
			SetMode(MODE_0, stat, interrupts);
			break;
		}

		if (ly == lyc) {
			stat |= PPU_STAT_LYC_FLAG;

			if (graphicsCtx->lyc_ly_int_sel) {
				interrupts |= IRQ_LCD_STAT;
			}
		} else {
			stat &= ~PPU_STAT_LYC_FLAG;
		}

		memInstance->RequestInterrupts(interrupts);
		return ly == LCD_SCANLINES_VBLANK;
	}
	else {
		return false;
	}
}

void GraphicsUnitSM83::NextFrame() {
	
		/*
		isrFlags = 0x00;

		// generate frame data
		if (graphicsCtx->isCgb) {
			if (graphicsCtx->bg_win_enable) {
				DrawTileMapBackground();
			}
			if (graphicsCtx->win_enable) {
				DrawTileMapWindow();
			}
		}
		else {
			if (graphicsCtx->bg_win_enable) {
				DrawTileMapBackground();
				if (graphicsCtx->win_enable) {
					DrawTileMapWindow();
				}
			}
		}
		if (graphicsCtx->obj_enable) {
			DrawObjectTiles();
		}
		*/
		/*
		// everything from here just pretends to have processed the scanlines (horizontal lines) of the LCD *****
		// request mode 0 to 2 interrupts (STAT)
		if (*graphicsCtx->STAT & (PPU_STAT_MODE0_EN | PPU_STAT_MODE1_EN | PPU_STAT_MODE2_EN)) {
			isrFlags |= IRQ_LCD_STAT;
		}

		// request ly compare interrupt (STAT) and set flag
		if (*graphicsCtx->LYC <= LCD_SCANLINES_TOTAL) {
			*graphicsCtx->STAT |= PPU_STAT_LYC_FLAG;
			if (*graphicsCtx->STAT & PPU_STAT_LYC_SOURCE) {
				isrFlags |= IRQ_LCD_STAT;
			}
		}
		else {
			*graphicsCtx->STAT &= ~PPU_STAT_LYC_FLAG;
		}

		memInstance->RequestInterrupts(isrFlags | IRQ_VBLANK);*/

		// TODO: let cpu run for machine cycles per scan line, render frame after last scanline, set LY to 0x90 after first 17500 machine cycles (via increment)
		// currently directly set to 0x90 only for testing with blargg's instruction tests
		//memInstance->SetIOValue(INIT_CGB_LY, LY_ADDR);//LCD_VBLANK_THRESHOLD;

	graphicsMgr->UpdateGpuData();
}

void GraphicsUnitSM83::SetMode(const modes _mode, u8& _stat_reg, u8& _int) {
	graphicsCtx->mode = _mode;
	_stat_reg = (_stat_reg & ~PPU_STAT_WRITEABLE_BITS) | _mode;

	switch (_mode) {
	case MODE_0:
		if (graphicsCtx->mode_0_int_sel) {
			_int |= IRQ_LCD_STAT;
		}
		break;
	case MODE_1:
		_int |= IRQ_VBLANK;
		if (graphicsCtx->mode_1_int_sel) {
			_int |= IRQ_LCD_STAT;
		}
		break;
	case MODE_2:
		if (graphicsCtx->mode_2_int_sel) {
			_int |= IRQ_LCD_STAT;
		}
		break;
	}
}





// draw tilemaps BG and WIN
void GraphicsUnitSM83::DrawTileMapBackground() {
	int scx = memInstance->GetIORef(SCX_ADDR);
	int scy = memInstance->GetIORef(SCY_ADDR);

	for (int x = 0; x < PPU_SCREEN_X; x+= PPU_PIXELS_TILE_X) {
		for (int y = 0; y < PPU_SCREEN_Y; y+= PPU_PIXELS_TILE_Y) {
			u8 tile_index = graphicsCtx->VRAM_N[0][
				graphicsCtx->bg_tilemap_offset +
				((scy + y) % PPU_TILEMAP_SIZE_1D) * PPU_TILEMAP_SIZE_1D + 
				(scx + x) % PPU_TILEMAP_SIZE_1D
			];
			ReadTileDataBGWIN(tile_index);
			if (graphicsCtx->isCgb) {
				ReadBGMapAttributes(tile_index);
			}
			DrawTileBackground(scx + x, scy + y);
		}
	}
}

// in CGB mode: copy attributes
void GraphicsUnitSM83::ReadBGMapAttributes(const u8& _offset) {
	/*
	if (graphicsCtx->bg_win_8800_addr_mode) {
		bgAttributes = graphicsCtx->VRAM_N[1][PPU_VRAM_BASEPTR_8800 - VRAM_N_OFFSET + (*(i8*)&_offset * PPU_VRAM_TILE_SIZE)];
	}
	else {
		bgAttributes = graphicsCtx->VRAM_N[1][_offset * PPU_VRAM_TILE_SIZE];
	}
	*/
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
	u8 win_offset = graphicsCtx->win_tilemap_offset;
	int wx = (int)memInstance->GetIORef(WX_ADDR) - 7;
	int wy = memInstance->GetIORef(WY_ADDR);

	for (int x = 0; x < PPU_TILES_HORIZONTAL - wx; x+=PPU_PIXELS_TILE_X) {
		for (int y = 0; y < PPU_TILES_VERTICAL - wy; y+=PPU_PIXELS_TILE_Y) {
			u8 tile_index = graphicsCtx->VRAM_N[0][win_offset + (y * PPU_TILEMAP_SIZE_1D) + x],
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
	/*
	if (graphicsCtx->bg_win_8800_addr_mode) {
		memcpy(currentTile, &graphicsCtx->VRAM_N[0][PPU_VRAM_BASEPTR_8800 - VRAM_N_OFFSET + (*(i8*)&_offset * PPU_VRAM_TILE_SIZE)], PPU_VRAM_TILE_SIZE);
	}
	else {
		memcpy(currentTile, &graphicsCtx->VRAM_N[0][_offset * PPU_VRAM_TILE_SIZE], PPU_VRAM_TILE_SIZE);
	}
	*/
}

// draw the objects to the screen
void GraphicsUnitSM83::DrawObjectTiles() {
	for (int i = 0; i < PPU_OBJ_ATTR_SIZE; i += PPU_OBJ_ATTR_BYTES) {
		ReadObjectAttributes(i);
		ReadTileDataObject(objAttributes[OBJ_ATTR_INDEX], graphicsCtx->obj_size_16);
		DrawObject((int)objAttributes[OBJ_ATTR_X] - 8, (int)objAttributes[OBJ_ATTR_Y] - 16, graphicsCtx->obj_size_16);
	}
}

// copy object attributes
void GraphicsUnitSM83::ReadObjectAttributes(const int& _offset){
	memcpy(objAttributes, &graphicsCtx->OAM[_offset], PPU_OBJ_ATTR_BYTES);
}

// copy tile data (pixels) for OBJ
void GraphicsUnitSM83::ReadTileDataObject(const u8& _offset, const bool& _mode16) {
	int vram_bank_selected;

	switch (graphicsCtx->isCgb) {
	case true:
		vram_bank_selected = (objAttributes[OBJ_ATTR_FLAGS] & OBJ_ATTR_VRAM_BANK ? 1 : 0);
		break;
	case false:
		vram_bank_selected = 0;
		break;
	}

	memcpy(currentTile, &graphicsCtx->VRAM_N[vram_bank_selected][_offset * PPU_VRAM_TILE_SIZE], PPU_VRAM_TILE_SIZE);
	if(_mode16){
		memcpy(currentTile16, &graphicsCtx->VRAM_N[vram_bank_selected][(_offset + 1) * PPU_VRAM_TILE_SIZE], PPU_VRAM_TILE_SIZE);
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
	if (graphicsCtx->isCgb) {

	}
	else {
		DMG_COLOR_PALETTE[pixel];
	}
}