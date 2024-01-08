#include "GraphicsUnitSM83.h"

#include "gameboy_defines.h"
#include "logger.h"
#include "general_config.h"

#include <format>

#include "vulkan/vulkan.h"

using namespace std;

// VK_FORMAT_R8G8B8A8_UNORM, gray scales

#define MC_PER_SCANLINE		(((BASE_CLOCK_CPU / 4) * 1000000) / DISPLAY_FREQUENCY) / LCD_SCANLINES_TOTAL

void GraphicsUnitSM83::SetGraphicsParameters() {
	graphicsInfo.is2d = graphicsInfo.en2d = true;
	graphicsInfo.image_data = vector<u8>(PPU_SCREEN_X * PPU_SCREEN_Y * TEX2D_CHANNELS);
	graphicsInfo.aspect_ratio = LCD_ASPECT_RATIO;
	graphicsInfo.lcd_width = PPU_SCREEN_X;
	graphicsInfo.lcd_height = PPU_SCREEN_Y;

	graphicsCtx->current_substeps = MC_PER_SCANLINE;

	if (machineCtx->isCgb) {
		DrawScanline = &GraphicsUnitSM83::DrawScanlineCGB;
	} else {
		DrawScanline = &GraphicsUnitSM83::DrawScanlineDMG;
	}
}

#define SET_MODE(stat, mode) stat = (stat & PPU_STAT_WRITEABLE_BITS) | mode 

bool GraphicsUnitSM83::ProcessGPU(const int& _substep) {
	if (graphicsCtx->ppu_enable) {
		u8 interrupts = 0;

		u8& ly = memInstance->GetIORef(LY_ADDR);
		const u8& lyc = memInstance->GetIORef(LYC_ADDR);
		u8& stat = memInstance->GetIORef(STAT_ADDR);

		bool next_frame = false;

		switch (graphicsCtx->mode) {
			// DRAWING PIXELS -----
		case PPU_MODE_2:
			graphicsCtx->mode = PPU_MODE_3;
			SET_MODE(stat, PPU_MODE_3);

			mode3Dots = PPU_DOTS_MODE_3_MIN;
			(this->*DrawScanline)(ly);
			graphicsCtx->current_substeps = (MC_PER_SCANLINE / PPU_DOTS_PER_SCANLINE) * PPU_DOTS_MODE_3;
			break;
			// HBLANK -----
		case PPU_MODE_3:
			graphicsCtx->mode = PPU_MODE_0;
			SET_MODE(stat, PPU_MODE_0);

			if (graphicsCtx->mode_0_int_sel) {
				interrupts |= IRQ_LCD_STAT;
			}

			graphicsCtx->current_substeps = (MC_PER_SCANLINE / PPU_DOTS_PER_SCANLINE) * PPU_DOTS_MODE_0;
			break;
			// VBLANK or OAM Scan
		case PPU_MODE_0:
			ly++;

			if (ly == LCD_SCANLINES_VBLANK) {
				graphicsCtx->mode = PPU_MODE_1;
				SET_MODE(stat, PPU_MODE_1);

				if (graphicsCtx->mode_1_int_sel) {
					interrupts |= (IRQ_VBLANK | IRQ_LCD_STAT);
				} else {
					interrupts |= IRQ_VBLANK;
				}

				drawWindow = false;
				next_frame = true;

				graphicsCtx->current_substeps = MC_PER_SCANLINE;
			} else {
				graphicsCtx->mode = PPU_MODE_2;
				SET_MODE(stat, PPU_MODE_2);

				if (graphicsCtx->mode_2_int_sel) {
					interrupts |= IRQ_LCD_STAT;
				}

				SearchOAM(ly);
				graphicsCtx->current_substeps = (MC_PER_SCANLINE / PPU_DOTS_PER_SCANLINE) * PPU_DOTS_MODE_2;
			}
			break;
			// OAM Scan (at scanline 153 -> 0 transition)
		case PPU_MODE_1:
			if (_substep == LCD_MODES_PER_SCANLINE - 1) {
				ly++;

				if (ly == LCD_SCANLINES_TOTAL) {
					ly = 0x00;

					graphicsCtx->mode = PPU_MODE_2;
					SET_MODE(stat, PPU_MODE_2);

					if (graphicsCtx->mode_2_int_sel) {
						interrupts |= IRQ_LCD_STAT;
					}

					SearchOAM(ly);
					graphicsCtx->current_substeps = (MC_PER_SCANLINE / PPU_DOTS_PER_SCANLINE) * PPU_DOTS_MODE_2;
				}
			} else if (_substep == 0){
				graphicsCtx->current_substeps = MC_PER_SCANLINE;
			} else {
				graphicsCtx->current_substeps = 0;
			}
			break;
		}

		if (ly == lyc) {
			stat |= PPU_STAT_LYC_FLAG;

			if (graphicsCtx->lyc_ly_int_sel) {
				interrupts |= (IRQ_LCD_STAT);
			}
		} else {
			stat &= ~PPU_STAT_LYC_FLAG;
		}

		memInstance->RequestInterrupts(interrupts);
		return next_frame;
	} else {
		return false;
	}
}

void GraphicsUnitSM83::DrawScanlineDMG(const u8& _ly) {
	if (graphicsCtx->obj_enable) {
		DrawObjectsDMG(_ly, OAMPrio1DMG, numOAMEntriesPrio1DMG, true);
	} else {
		numOAMEntriesPrio0DMG = 0;
		numOAMEntriesPrio1DMG = 0;
	}

	if (graphicsCtx->bg_win_enable) {
		DrawBackgroundDMG(_ly);

		if (graphicsCtx->win_enable) {
			DrawWindowDMG(_ly);
		}
	}

	if (graphicsCtx->obj_enable) {
		DrawObjectsDMG(_ly, OAMPrio0DMG, numOAMEntriesPrio0DMG, false);
	}
}

// TODO: scx is actually reread on each tile fetch
void GraphicsUnitSM83::DrawBackgroundDMG(const u8& _ly) {
	auto tilemap_offset = (int)graphicsCtx->bg_tilemap_offset;
	int ly = _ly;

	auto scx = (int)memInstance->GetIORef(SCX_ADDR);
	auto scy = (int)memInstance->GetIORef(SCY_ADDR);
	int scy_ = (scy + ly) % PPU_TILEMAP_SIZE_1D_PIXELS;
	int scx_;

	//mode3Dots += scx % 8;

	int tilemap_offset_y = ((scy_ / PPU_TILE_SIZE_Y) * PPU_TILEMAP_SIZE_1D) % (PPU_TILEMAP_SIZE_1D * PPU_TILEMAP_SIZE_1D);
	int tilemap_offset_x;

	int y_clip = (scy + ly) % PPU_TILE_SIZE_Y;
	int x_clip = scx % PPU_TILE_SIZE_X;

	u8 tile_offset;
	for (int x = 0; x < PPU_SCREEN_X + x_clip; x += PPU_TILE_SIZE_X) {
		scx_ = (scx + x) % PPU_TILEMAP_SIZE_1D_PIXELS;
		tilemap_offset_x = (scx_ / PPU_TILE_SIZE_X) % PPU_TILEMAP_SIZE_1D;

		tile_offset = graphicsCtx->VRAM_N[0][tilemap_offset + tilemap_offset_y + tilemap_offset_x];

		FetchTileDataBGWIN(tile_offset, y_clip * 2);
		DrawTileBGWINDMG(x - x_clip, ly, graphicsCtx->dmg_bgp_color_palette);
	}
}

void GraphicsUnitSM83::DrawWindowDMG(const u8& _ly) {
	int ly = _ly;

	auto wx = ((int)memInstance->GetIORef(WX_ADDR));
	auto wy = (int)memInstance->GetIORef(WY_ADDR);
	auto wx_ = wx - 7;

	if (!drawWindow && (wy == ly) && (wx_ > -8 && wx_ < PPU_SCREEN_X)) {
		drawWindow = true;
	}

	if (drawWindow) {
		auto tilemap_offset = (int)graphicsCtx->win_tilemap_offset;

		int tilemap_offset_y = ((ly - wy) / PPU_TILE_SIZE_Y) * PPU_TILEMAP_SIZE_1D;
		int tilemap_offset_x;

		int y_clip = (ly - wy) % PPU_TILE_SIZE_Y;
		int x_clip = wx_ % PPU_TILE_SIZE_X;

		//mode3Dots += PPU_DOTS_MODE_3_WIN_FETCH;

		u8 tile_offset;
		for (int x = 0; x < PPU_SCREEN_X + x_clip; x += PPU_TILE_SIZE_X) {
			tilemap_offset_x = (x - wx_) / PPU_TILE_SIZE_X;

			tile_offset = graphicsCtx->VRAM_N[0][tilemap_offset + tilemap_offset_y + tilemap_offset_x];

			FetchTileDataBGWIN(tile_offset, y_clip * 2);
			DrawTileBGWINDMG(x, ly, graphicsCtx->dmg_bgp_color_palette);
		}
	}
}

void GraphicsUnitSM83::DrawObjectsDMG(const u8& _ly, const int* _objects, const int& _num_objects, const bool& _prio) {
	for (int i = 0; i < _num_objects; i++) {
		int oam_offset = _objects[i];

		int y_pos = (int)graphicsCtx->OAM[oam_offset + OBJ_ATTR_Y];
		int x_pos = (int)graphicsCtx->OAM[oam_offset + OBJ_ATTR_X];
		u8 tile_offset = graphicsCtx->OAM[oam_offset + OBJ_ATTR_INDEX];
		u8 flags = graphicsCtx->OAM[oam_offset + OBJ_ATTR_FLAGS];

		int ly = _ly;

		int y_clip;
		if (graphicsCtx->obj_size_16) {
			if (flags & OBJ_ATTR_Y_FLIP) {
				y_clip = y_pos - ly - 1;
			} else {
				y_clip = PPU_TILE_SIZE_Y_16 - (y_pos - ly);
			}
		} else {
			if (flags & OBJ_ATTR_Y_FLIP) {
				y_clip = (y_pos - ly) - (PPU_TILE_SIZE_Y + 1);
			} else {
				y_clip = PPU_TILE_SIZE_Y - ((y_pos - ly) - PPU_TILE_SIZE_Y);
			}
		}

		u32* palette;
		if (flags & OBJ_ATTR_PALETTE_DMG) {
			palette = graphicsCtx->dmg_obp1_color_palette;
		} else {
			palette = graphicsCtx->dmg_obp0_color_palette;
		}

		bool x_flip = (flags & OBJ_ATTR_X_FLIP) ? true : false;

		FetchTileDataOBJ(tile_offset, y_clip * 2);
		DrawTileOBJDMG(x_pos - 8, ly, palette, _prio, x_flip);
	}
}

void GraphicsUnitSM83::DrawTileOBJDMG(const int& _x, const int& _y, const u32* _color_palette, const bool& _prio, const bool& _x_flip) {
	int image_offset_y = _y * PPU_SCREEN_X * TEX2D_CHANNELS;
	int image_offset_x;
	int color_index = 0;
	u32 color;

	u8 bit_mask = _x_flip ? 0x01 : 0x80;
	int x = _x;

	if (_x_flip) {
		for (int i = 0; i < 8; i++) {
			if (x > -1 && x < PPU_SCREEN_X) {
				color_index = (((tileDataCur[1] & bit_mask) >> i) << 1) | ((tileDataCur[0] & bit_mask) >> i);

				if (color_index > 0 || _prio) {
					color = _color_palette[color_index];
					image_offset_x = x * TEX2D_CHANNELS;

					u32 color_mask = 0xFF000000;
					for (int k = 0, j = 3 * 8; j > -1; j -= 8, k++) {
						graphicsInfo.image_data[image_offset_y + image_offset_x + k] = (u8)((color & color_mask) >> j);
						color_mask >>= 8;
					}

					if (_prio) { objPrio1DMG[x] = true; }
				}
			}

			bit_mask <<= 1;
			x++;
		}
	} else {
		for (int i = 7; i > -1; i--) {
			if (x > -1 && x < PPU_SCREEN_X) {
				color_index = (((tileDataCur[1] & bit_mask) >> i) << 1) | ((tileDataCur[0] & bit_mask) >> i);

				if (color_index > 0 || _prio) {
					color = _color_palette[color_index];
					image_offset_x = x * TEX2D_CHANNELS;

					u32 color_mask = 0xFF000000;
					for (int k = 0, j = 3 * 8; j > -1; j -= 8, k++) {
						graphicsInfo.image_data[image_offset_y + image_offset_x + k] = (u8)((color & color_mask) >> j);
						color_mask >>= 8;
					}

					if (_prio) { objPrio1DMG[x] = true; }
				}
			}

			bit_mask >>= 1;
			x++;
		}
	}
}

void GraphicsUnitSM83::DrawTileBGWINDMG(const int& _x, const int& _y, const u32* _color_palette) {
	int image_offset_y = _y * PPU_SCREEN_X * TEX2D_CHANNELS;
	int image_offset_x;
	int color_index = 0;
	u32 color;

	u8 bit_mask = 0x80;
	int x = _x;
	for (int i = 7; i > -1; i--) {
		if (x > -1 && x < PPU_SCREEN_X) {
			color_index = (((tileDataCur[1] & bit_mask) >> i) << 1) | ((tileDataCur[0] & bit_mask) >> i);

			if (!objPrio1DMG[x] || color_index != 0) {
				color = _color_palette[color_index];

				image_offset_x = x * TEX2D_CHANNELS;

				u32 color_mask = 0xFF000000;
				for (int k = 0, j = 3 * 8; j > -1; j -= 8, k++) {
					graphicsInfo.image_data[image_offset_y + image_offset_x + k] = (u8)((color & color_mask) >> j);
					color_mask >>= 8;
				}
			}
		}

		bit_mask >>= 1;
		x++;
	}
}

void GraphicsUnitSM83::DrawScanlineCGB(const u8& _ly) {
	if (graphicsCtx->obj_enable) {
		DrawObjectsDMG(_ly, OAMPrio1DMG, numOAMEntriesPrio1DMG, true);
	} else {
		numOAMEntriesPrio0DMG = 0;
		numOAMEntriesPrio1DMG = 0;
	}

	//if (graphicsCtx->bg_win_enable) {
		DrawBackgroundDMG(_ly);

		if (graphicsCtx->win_enable) {
			DrawWindowDMG(_ly);
		}
	//}

	if (graphicsCtx->obj_enable) {
		DrawObjectsDMG(_ly, OAMPrio0DMG, numOAMEntriesPrio0DMG, false);
	}
}

void GraphicsUnitSM83::SearchOAM(const u8& _ly) {
	numOAMEntriesPrio0DMG = 0;
	numOAMEntriesPrio1DMG = 0;

	memset(objPrio1DMG, false, PPU_SCREEN_X);

	if (graphicsCtx->obj_enable) {
		int y_pos;
		int ly = _ly;
		bool prio;
		bool add_entry = false;

		for (int oam_offset = 0; oam_offset < OAM_SIZE; oam_offset += PPU_OBJ_ATTR_BYTES) {
			y_pos = (int)graphicsCtx->OAM[oam_offset + OBJ_ATTR_Y];

			add_entry =
				(graphicsCtx->obj_size_16 && ((y_pos - ly) > 0 && (y_pos - _ly) < (16 + 1))) ||
				((y_pos - ly) > 8 && (y_pos - _ly) < (16 + 1));

			if (add_entry) {
				prio = (graphicsCtx->OAM[oam_offset + OBJ_ATTR_FLAGS] & OBJ_ATTR_PRIO) ? true : false;

				if (prio && numOAMEntriesPrio1DMG < 10) {
					OAMPrio1DMG[numOAMEntriesPrio1DMG] = oam_offset;
					numOAMEntriesPrio1DMG++;
				} else if (numOAMEntriesPrio0DMG < 10) {
					OAMPrio0DMG[numOAMEntriesPrio0DMG] = oam_offset;
					numOAMEntriesPrio0DMG++;
				}
			}
		}
	}
}

void GraphicsUnitSM83::FetchTileDataOBJ(u8& _tile_offset, const int& _tile_sub_offset) {
	if (graphicsCtx->obj_size_16) {
		int tile_sub_index = (PPU_VRAM_BASEPTR_8000 - VRAM_N_OFFSET) + ((_tile_offset & 0xFE) * 0x10) + _tile_sub_offset;
		tileDataCur[0] = graphicsCtx->VRAM_N[0][tile_sub_index];
		tileDataCur[1] = graphicsCtx->VRAM_N[0][tile_sub_index + 1];
	} else {
		int tile_sub_index = (PPU_VRAM_BASEPTR_8000 - VRAM_N_OFFSET) + (_tile_offset * 0x10) + _tile_sub_offset;
		tileDataCur[0] = graphicsCtx->VRAM_N[0][tile_sub_index];
		tileDataCur[1] = graphicsCtx->VRAM_N[0][tile_sub_index + 1];
	}
}

void GraphicsUnitSM83::FetchTileDataBGWIN(u8& _tile_offset, const int& _tile_sub_offset) {
	if (graphicsCtx->bg_win_addr_mode_8000) {
		int tile_sub_index = (PPU_VRAM_BASEPTR_8000 - VRAM_N_OFFSET) + (_tile_offset * 0x10) + _tile_sub_offset;
		tileDataCur[0] = graphicsCtx->VRAM_N[0][tile_sub_index];
		tileDataCur[1] = graphicsCtx->VRAM_N[0][tile_sub_index + 1];
	} else {
		int tile_sub_index = (PPU_VRAM_BASEPTR_8800 - VRAM_N_OFFSET) + (*(i8*)&_tile_offset * 0x10) + _tile_sub_offset;
		tileDataCur[0] = graphicsCtx->VRAM_N[0][tile_sub_index];
		tileDataCur[1] = graphicsCtx->VRAM_N[0][tile_sub_index + 1];
	}
}