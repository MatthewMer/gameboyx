#include "GameboyGPU.h"

#include "gameboy_defines.h"
#include "logger.h"
#include "general_config.h"

#include <format>

#include "vulkan/vulkan.h"

using namespace std;

// VK_FORMAT_R8G8B8A8_UNORM, gray scales

#define MC_PER_SCANLINE		(((((BASE_CLOCK_CPU / pow(10, 6)) / 4) * 1000000) / DISPLAY_FREQUENCY) / LCD_SCANLINES_TOTAL)

// return delta t per frame in nanoseconds
int GameboyGPU::GetDelayTime() const {
	return (int)((1.f / DISPLAY_FREQUENCY) * pow(10, 6));
}

int GameboyGPU::GetTicksPerFrame(const float& _clock) const {
	return (int)(_clock / DISPLAY_FREQUENCY);
}

void GameboyGPU::ProcessGPU(const int& _ticks) {
	if (graphicsCtx->ppu_enable) {
		tickCounter += _ticks;

		u8& ly = memInstance->GetIO(LY_ADDR);
		const u8& lyc = memInstance->GetIO(LYC_ADDR);
		u8& stat = memInstance->GetIO(STAT_ADDR);
		bool ly_lyc = false;

		if (ly == lyc) {
			stat |= PPU_STAT_LYC_FLAG;
			ly_lyc = graphicsCtx->lyc_ly_int_sel;
		} else {
			stat &= ~PPU_STAT_LYC_FLAG;
		}

		switch (graphicsCtx->mode) {
		case PPU_MODE_2:
			statSignal = ly_lyc || graphicsCtx->mode_2_int_sel;
			SearchOAM(ly);
			SearchOAM(ly);

			if (tickCounter == PPU_DOTS_MODE_2) {
				EnterMode3();
			}
			break;
		case PPU_MODE_3:
			statSignal = ly_lyc;

			if (tickCounter > modeTickTarget) {
				(this->*DrawScanline)(ly);
				EnterMode0();
			}
			break;
		case PPU_MODE_0:
			statSignal = ly_lyc || graphicsCtx->mode_0_int_sel;

			if (tickCounter == PPU_DOTS_PER_SCANLINE) {
				tickCounter = 0;
				ly++;

				if (ly == LCD_SCANLINES_VBLANK) {
					HardwareMgr::UpdateGpuData();
					frameCounter++;
					EnterMode1();
				} else {
					EnterMode2();
				}
			}
			break;
		case PPU_MODE_1:
			statSignal = ly_lyc || graphicsCtx->mode_1_int_sel || graphicsCtx->mode_2_int_sel;
			
			if (tickCounter == PPU_DOTS_PER_SCANLINE) {
				tickCounter = 0;
				ly++;
				if (ly == LCD_SCANLINES_TOTAL) {
					ly = 0x00;
					EnterMode2();
				}
			}
			break;
		}

		if (statSignal && !statSignalPrev) {
			memInstance->RequestInterrupts(IRQ_LCD_STAT);
		}
		statSignalPrev = statSignal;

		graphicsCtx->vblank_if_write = false;

		//LOG_INFO("LY: ", format("{:02x}", ly), " -> ", format("{:d}: {:d}", graphicsCtx->mode, tickCounter));
	} else {
		statSignal = false;
		statSignalPrev = false;
	}
}


#define SET_MODE(stat, mode) stat = (stat & PPU_STAT_WRITEABLE_BITS) | mode 

void GameboyGPU::EnterMode2() {
	memset(objPrio1DMG, false, PPU_SCREEN_X);

	u8& stat = memInstance->GetIO(STAT_ADDR);

	graphicsCtx->mode = PPU_MODE_2;
	SET_MODE(stat, PPU_MODE_2);

	modeTickCounter = 0;
	oamOffset = 0;

	numOAMEntriesPrio0DMG = 0;
	numOAMEntriesPrio1DMG = 0;
}

void GameboyGPU::EnterMode3() {
	u8& stat = memInstance->GetIO(STAT_ADDR);
	u8& ly = memInstance->GetIO(LY_ADDR);

	graphicsCtx->mode = PPU_MODE_3;
	SET_MODE(stat, PPU_MODE_3);

	//mode3scxPause = memInstance->GetIO(SCX_ADDR) % 8;

	modeTickTarget = (tickCounter + 168 + (numOAMEntriesPrio0DMG + numOAMEntriesPrio1DMG) * 10) - 1;
}

void GameboyGPU::EnterMode0() {
	u8& stat = memInstance->GetIO(STAT_ADDR);

	graphicsCtx->mode = PPU_MODE_0;
	SET_MODE(stat, PPU_MODE_0);

	CheckHBlankDma();
}

void GameboyGPU::CheckHBlankDma() {
	if (graphicsCtx->dma_hblank) {
		int bank;

		u16& source_addr = graphicsCtx->dma_source_addr;
		u16& dest_addr = graphicsCtx->dma_dest_addr;

		switch (graphicsCtx->dma_source_mem) {
		case ROM0:
			memcpy(&graphicsCtx->VRAM_N[machineCtx->vram_bank_selected][dest_addr], &memInstance->GetBank(ROM0, 0)[source_addr], 0x10);
			break;
		case ROMn:
			bank = machineCtx->rom_bank_selected;
			memcpy(&graphicsCtx->VRAM_N[machineCtx->vram_bank_selected][dest_addr], &memInstance->GetBank(ROMn, bank)[source_addr], 0x10);
			break;
		case RAMn:
			bank = machineCtx->ram_bank_selected;
			memcpy(&graphicsCtx->VRAM_N[machineCtx->vram_bank_selected][dest_addr], &memInstance->GetBank(RAMn, bank)[source_addr], 0x10);
			break;
		case WRAM0:
			memcpy(&graphicsCtx->VRAM_N[machineCtx->vram_bank_selected][dest_addr], &memInstance->GetBank(WRAM0, 0)[source_addr], 0x10);
			break;
		case WRAMn:
			bank = machineCtx->wram_bank_selected;
			memcpy(&graphicsCtx->VRAM_N[machineCtx->vram_bank_selected][dest_addr], &memInstance->GetBank(WRAMn, bank)[source_addr], 0x10);
			break;
		}

		u8& hdma5 = memInstance->GetIO(CGB_HDMA5_ADDR);
		int& dma_length = graphicsCtx->dma_length;

		//LOG_INFO("LY: ", std::format("{:d}", memInstance->GetIO(LY_ADDR)), "; source: ", std::format("0x{:04x}", source_addr), "; dest: ", std::format("0x{:04x}", dest_addr), "; remaining: ", (dma_length - 1) * 0x10);

		--dma_length;
		if (dma_length > 0) {
			source_addr += 0x10;
			dest_addr += 0x10;
			--hdma5;
		} else {
			graphicsCtx->dma_hblank = false;
			hdma5 = 0xFF;
			LOG_ERROR("finish");
		}
	}
}

void GameboyGPU::EnterMode1() {
	u8& stat = memInstance->GetIO(STAT_ADDR);

	graphicsCtx->mode = PPU_MODE_1;
	SET_MODE(stat, PPU_MODE_1);

	memInstance->RequestInterrupts(IRQ_VBLANK);

	drawWindow = false;
}

void GameboyGPU::DrawScanlineDMG(const u8& _ly) {
	if (graphicsCtx->obj_enable) {
		DrawObjectsDMG(_ly, OAMPrio1DMG, numOAMEntriesPrio1DMG, true);
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
void GameboyGPU::DrawBackgroundDMG(const u8& _ly) {
	auto tilemap_offset = (int)graphicsCtx->bg_tilemap_offset;
	int ly = _ly;

	auto scx = (int)memInstance->GetIO(SCX_ADDR);
	auto scy = (int)memInstance->GetIO(SCY_ADDR);
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

		FetchTileDataBGWIN(tile_offset, y_clip * 2, 0);
		DrawTileBGWINDMG(x - x_clip, ly, graphicsCtx->dmg_bgp_color_palette);
	}
}

void GameboyGPU::DrawWindowDMG(const u8& _ly) {
	int ly = _ly;

	auto wx = ((int)memInstance->GetIO(WX_ADDR));
	auto wy = (int)memInstance->GetIO(WY_ADDR);
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

			FetchTileDataBGWIN(tile_offset, y_clip * 2, 0);
			DrawTileBGWINDMG(x, ly, graphicsCtx->dmg_bgp_color_palette);
		}
	}
}

void GameboyGPU::DrawObjectsDMG(const u8& _ly, const int* _objects, const int& _num_objects, const bool& _prio) {
	// TODO: due to drawing objects stored in _objects from last to first element, priority ('z fighting', even though there is no z-axis) gets resolved like on CGB.
	// on DMG this is actually done by comparing the x coordinate of the objects in oam. the smaller value wins

	for (int i = _num_objects - 1; i > -1; i--) {
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

		FetchTileDataOBJ(tile_offset, y_clip * 2, 0);
		DrawTileOBJDMG(x_pos - 8, ly, palette, _prio, x_flip);
	}
}

void GameboyGPU::DrawTileOBJDMG(const int& _x, const int& _y, const u32* _color_palette, const bool& _prio, const bool& _x_flip) {
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
						imageData[image_offset_y + image_offset_x + k] = (u8)((color & color_mask) >> j);
						color_mask >>= 8;
					}

					if (_prio) { objPrio1DMG[x] = true; }
				}
			}

			bit_mask <<= 1;
			x++;
		}
	} else {
		for (int i = 7; i > -1; --i) {
			if (x > -1 && x < PPU_SCREEN_X) {
				color_index = (((tileDataCur[1] & bit_mask) >> i) << 1) | ((tileDataCur[0] & bit_mask) >> i);

				if (color_index > 0 || _prio) {
					color = _color_palette[color_index];
					image_offset_x = x * TEX2D_CHANNELS;

					u32 color_mask = 0xFF000000;
					for (int k = 0, j = 3 * 8; j > -1; j -= 8, k++) {
						imageData[image_offset_y + image_offset_x + k] = (u8)((color & color_mask) >> j);
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

void GameboyGPU::DrawTileBGWINDMG(const int& _x, const int& _y, const u32* _color_palette) {
	int image_offset_y = _y * PPU_SCREEN_X * TEX2D_CHANNELS;
	int image_offset_x;
	int color_index = 0;
	u32 color;

	u8 bit_mask = 0x80;
	int x = _x;
	for (int i = 7; i > -1; --i) {
		if (x > -1 && x < PPU_SCREEN_X) {
			color_index = (((tileDataCur[1] & bit_mask) >> i) << 1) | ((tileDataCur[0] & bit_mask) >> i);

			if (!objPrio1DMG[x] || color_index != 0) {
				color = _color_palette[color_index];

				image_offset_x = x * TEX2D_CHANNELS;

				u32 color_mask = 0xFF000000;
				for (int k = 0, j = 3 * 8; j > -1; j -= 8, k++) {
					imageData[image_offset_y + image_offset_x + k] = (u8)((color & color_mask) >> j);
					color_mask >>= 8;
				}
			}
		}

		bit_mask >>= 1;
		x++;
	}
}

void GameboyGPU::DrawScanlineCGB(const u8& _ly) {
	if (graphicsCtx->obj_enable) {
		DrawObjectsDMG(_ly, OAMPrio1DMG, numOAMEntriesPrio1DMG, true);
	}

	//if (graphicsCtx->bg_win_enable) {
	DrawBackgroundCGB(_ly);

		if (graphicsCtx->win_enable) {
			//DrawWindowCGB(_ly);
		}
	//}

	if (graphicsCtx->obj_enable) {
		DrawObjectsDMG(_ly, OAMPrio0DMG, numOAMEntriesPrio0DMG, false);
	}
}

void GameboyGPU::DrawBackgroundCGB(const u8& _ly) {
	auto tilemap_offset = (int)graphicsCtx->bg_tilemap_offset;
	int ly = _ly;

	auto scx = (int)memInstance->GetIO(SCX_ADDR);
	auto scy = (int)memInstance->GetIO(SCY_ADDR);
	int scy_ = (scy + ly) % PPU_TILEMAP_SIZE_1D_PIXELS;
	int scx_;

	//mode3Dots += scx % 8;

	int tilemap_offset_y = ((scy_ / PPU_TILE_SIZE_Y) * PPU_TILEMAP_SIZE_1D) % (PPU_TILEMAP_SIZE_1D * PPU_TILEMAP_SIZE_1D);
	int tilemap_offset_x;

	int y_clip = (scy + ly) % PPU_TILE_SIZE_Y;
	int x_clip = scx % PPU_TILE_SIZE_X;

	int bank;
	int palette_index;
	bool x_flip;
	bool y_flip;
	bool prio;

	int y_clip_;

	u8 tile_offset;
	u8 tile_attr;
	for (int x = 0; x < PPU_SCREEN_X + x_clip; x += PPU_TILE_SIZE_X) {
		scx_ = (scx + x) % PPU_TILEMAP_SIZE_1D_PIXELS;
		tilemap_offset_x = (scx_ / PPU_TILE_SIZE_X) % PPU_TILEMAP_SIZE_1D;

		int index = tilemap_offset + tilemap_offset_y + tilemap_offset_x;
		tile_offset = graphicsCtx->VRAM_N[0][index];
		tile_attr = graphicsCtx->VRAM_N[1][index];

		palette_index = tile_attr & BG_ATTR_PALETTE_CGB;
		bank = (tile_attr & BG_ATTR_VRAM_BANK_CGB) >> 2;
		x_flip = (tile_attr & BG_ATTR_FLIP_HORIZONTAL ? true : false);
		y_flip = (tile_attr & BG_ATTR_FLIP_VERTICAL ? true : false);
		prio = (tile_attr & BG_ATTR_OAM_PRIORITY ? true : false);			// NOT USED CURRENTLY !!!

		if (y_flip) {
			y_clip_ = (PPU_TILE_SIZE_Y - 1) - y_clip;
		} else {
			y_clip_ = y_clip;
		}

		FetchTileDataBGWIN(tile_offset, y_clip_ * 2, bank);
		DrawTileBGWINCGB(x - x_clip, ly, graphicsCtx->cgb_bgp_color_palettes[palette_index], x_flip);
	}
}

void GameboyGPU::DrawWindowCGB(const u8& _ly) {
	int ly = _ly;

	auto wx = ((int)memInstance->GetIO(WX_ADDR));
	auto wy = (int)memInstance->GetIO(WY_ADDR);
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

			FetchTileDataBGWIN(tile_offset, y_clip * 2, 0);
			DrawTileBGWINCGB(x, ly, graphicsCtx->dmg_bgp_color_palette, false);
		}
	}
}

void GameboyGPU::DrawTileBGWINCGB(const int& _x, const int& _y, const u32* _color_palette, const bool& _x_flip) {
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

				if (!objPrio1DMG[x] || color_index != 0) {
					color = _color_palette[color_index];

					image_offset_x = x * TEX2D_CHANNELS;

					u32 color_mask = 0xFF000000;
					for (int k = 0, j = 3 * 8; j > -1; j -= 8, k++) {
						imageData[image_offset_y + image_offset_x + k] = (u8)((color & color_mask) >> j);
						color_mask >>= 8;
					}
				}
			}

			bit_mask <<= 1;
			x++;
		}
	} else {
		for (int i = 7; i > -1; --i) {
			if (x > -1 && x < PPU_SCREEN_X) {
				color_index = (((tileDataCur[1] & bit_mask) >> i) << 1) | ((tileDataCur[0] & bit_mask) >> i);

				if (!objPrio1DMG[x] || color_index != 0) {
					color = _color_palette[color_index];

					image_offset_x = x * TEX2D_CHANNELS;

					u32 color_mask = 0xFF000000;
					for (int k = 0, j = 3 * 8; j > -1; j -= 8, k++) {
						imageData[image_offset_y + image_offset_x + k] = (u8)((color & color_mask) >> j);
						color_mask >>= 8;
					}
				}
			}

			bit_mask >>= 1;
			x++;
		}
	}
}

void GameboyGPU::SearchOAM(const u8& _ly) {
	if (graphicsCtx->obj_enable) {
		if (oamOffset < OAM_SIZE) {
			int y_pos;
			int ly = _ly;
			bool prio;
			bool add_entry = false;
		
			y_pos = (int)graphicsCtx->OAM[oamOffset + OBJ_ATTR_Y];

			add_entry =
				(graphicsCtx->obj_size_16 && ((y_pos - ly) > 0 && (y_pos - _ly) < (16 + 1))) ||
				((y_pos - ly) > 8 && (y_pos - _ly) < (16 + 1));

			if (add_entry) {
				prio = (graphicsCtx->OAM[oamOffset + OBJ_ATTR_FLAGS] & OBJ_ATTR_PRIO) ? true : false;

				if (numOAMEntriesPrio1DMG + numOAMEntriesPrio0DMG < 10) {
					if (prio) {
						OAMPrio1DMG[numOAMEntriesPrio1DMG] = oamOffset;
						numOAMEntriesPrio1DMG++;
					} else {
						OAMPrio0DMG[numOAMEntriesPrio0DMG] = oamOffset;
						numOAMEntriesPrio0DMG++;
					}
				}
			}

			oamOffset += PPU_OBJ_ATTR_BYTES;
		}
	}
}

void GameboyGPU::FetchTileDataOBJ(u8& _tile_offset, const int& _tile_sub_offset, const int& _bank) {
	if (graphicsCtx->obj_size_16) {
		int tile_sub_index = (PPU_VRAM_BASEPTR_8000 - VRAM_N_OFFSET) + ((_tile_offset & 0xFE) * 0x10) + _tile_sub_offset;
		tileDataCur[0] = graphicsCtx->VRAM_N[_bank][tile_sub_index];
		tileDataCur[1] = graphicsCtx->VRAM_N[_bank][tile_sub_index + 1];
	} else {
		int tile_sub_index = (PPU_VRAM_BASEPTR_8000 - VRAM_N_OFFSET) + (_tile_offset * 0x10) + _tile_sub_offset;
		tileDataCur[0] = graphicsCtx->VRAM_N[_bank][tile_sub_index];
		tileDataCur[1] = graphicsCtx->VRAM_N[_bank][tile_sub_index + 1];
	}
}

void GameboyGPU::FetchTileDataBGWIN(u8& _tile_offset, const int& _tile_sub_offset, const int& _bank) {
	if (graphicsCtx->bg_win_addr_mode_8000) {
		int tile_sub_index = (PPU_VRAM_BASEPTR_8000 - VRAM_N_OFFSET) + (_tile_offset * 0x10) + _tile_sub_offset;
		tileDataCur[0] = graphicsCtx->VRAM_N[_bank][tile_sub_index];
		tileDataCur[1] = graphicsCtx->VRAM_N[_bank][tile_sub_index + 1];
	} else {
		int tile_sub_index = (PPU_VRAM_BASEPTR_8800 - VRAM_N_OFFSET) + (*(i8*)&_tile_offset * 0x10) + _tile_sub_offset;
		tileDataCur[0] = graphicsCtx->VRAM_N[_bank][tile_sub_index];
		tileDataCur[1] = graphicsCtx->VRAM_N[_bank][tile_sub_index + 1];
	}
}