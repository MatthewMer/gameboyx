#include "GameboyGPU.h"

#include "gameboy_defines.h"
#include "logger.h"
#include "general_config.h"

#include <format>

using namespace std;

// VK_FORMAT_R8G8B8A8_UNORM, gray scales

namespace Emulation {
	namespace Gameboy {
#define MC_PER_SCANLINE		(((((BASE_CLOCK_CPU / pow(10, 6)) / 4) * 1000000) / DISPLAY_FREQUENCY) / LCD_SCANLINES_TOTAL)

		// return delta t per frame in microseconds
		int GameboyGPU::GetDelayTime() const {
			return (int)((1.f / DISPLAY_FREQUENCY) * pow(10, 6));
		}

		int GameboyGPU::GetTicksPerFrame(const float& _clock) const {
			return (int)(_clock / DISPLAY_FREQUENCY);
		}

		std::vector<std::tuple<int, std::string, bool>> GameboyGPU::GetGraphicsDebugSettings() {
			graphicsDebugSettings.clear();
			graphicsDebugSettings.emplace_back(&presentObjPrio0);
			graphicsDebugSettings.emplace_back(&presentObjPrio1);
			graphicsDebugSettings.emplace_back(&presentBackground);
			graphicsDebugSettings.emplace_back(&presentWindow);

			std::vector<std::tuple<int, std::string, bool>> settings;
			settings.emplace_back(0, "Objects Priority 0", graphicsDebugSettings[0]->load());
			settings.emplace_back(1, "Objects Priority 1", graphicsDebugSettings[1]->load());
			settings.emplace_back(2, "Background", graphicsDebugSettings[2]->load());
			settings.emplace_back(3, "Window", graphicsDebugSettings[3]->load());

			return settings;
		}

		void GameboyGPU::SetGraphicsDebugSetting(const bool& _val, const int& _id) {
			if (_id < graphicsDebugSettings.size()) {
				graphicsDebugSettings[_id]->store(_val);
			}
		}

		void GameboyGPU::ProcessGPU(const int& _ticks) {
			OAMDMANextBlock();

			if (graphicsCtx->ppu_enable) {
				int current_ticks = _ticks / machineCtx->currentSpeed;

				u8& ly = memInstance->GetIO(LY_ADDR);
				const u8& lyc = memInstance->GetIO(LYC_ADDR);
				u8& stat = memInstance->GetIO(STAT_ADDR);

				for (; current_ticks > 0; current_ticks -= 2) {

					tickCounter += 2;

					switch (graphicsCtx->mode) {
					case PPU_MODE_2:
						statSignal = graphicsCtx->mode_2_int_sel;
						(this->*SearchOam)(ly);

						if (tickCounter >= PPU_DOTS_MODE_2) {
							EnterMode3();
						}
						break;
					case PPU_MODE_3:
						statSignal = false;

						if (tickCounter >= PPU_DOTS_MODE_3_MIN + PPU_DOTS_MODE_2) {
							int offset_y = ly * PPU_SCREEN_X * TEX2D_CHANNELS;
							memset(&imageData[offset_y], 0xFF, PPU_SCREEN_X * TEX2D_CHANNELS);
							(this->*DrawScanline)(ly);
							EnterMode0();
						}
						break;
					case PPU_MODE_0:
						statSignal = graphicsCtx->mode_0_int_sel;

						if (tickCounter >= PPU_DOTS_PER_SCANLINE) {
							tickCounter = 0;
							ly++;

							if (ly >= LCD_SCANLINES_VBLANK) {
								Backend::HardwareMgr::UpdateTexture2d();
								frameCounter++;
								EnterMode1();
							} else {
								EnterMode2();
							}
						}
						break;
					case PPU_MODE_1:
						statSignal = graphicsCtx->mode_1_int_sel || graphicsCtx->mode_2_int_sel;

						if (tickCounter >= PPU_DOTS_PER_SCANLINE) {
							tickCounter = 0;
							ly++;
							if (ly == LCD_SCANLINES_TOTAL) {
								ly = 0x00;
								EnterMode2();

								presentObjPrio0Set = presentObjPrio0.load();
								presentObjPrio1Set = presentObjPrio1.load();
								presentBackgroundSet = presentBackground.load();
								presentWindowSet = presentWindow.load();
							}
						}
						break;
					}

					if (ly == lyc) {
						stat |= PPU_STAT_LYC_FLAG;
						statSignal = statSignal || graphicsCtx->lyc_ly_int_sel;
					} else {
						stat &= ~PPU_STAT_LYC_FLAG;
					}

					if (statSignal && !statSignalPrev) {
						memInstance->RequestInterrupts(IRQ_LCD_STAT);
					}
					statSignalPrev = statSignal;
				}

				graphicsCtx->vblank_if_write = false;

				//LOG_INFO("LY: ", format("{:02x}", ly), " -> ", format("{:d}: {:d}", graphicsCtx->mode, tickCounter));
			} else {
				statSignal = false;
				statSignalPrev = false;
			}
		}


#define SET_MODE(stat, mode) stat = (stat & PPU_STAT_WRITEABLE_BITS) | mode 

		void GameboyGPU::EnterMode2() {
			std::fill(objNoPrio.begin(), objNoPrio.end(), false);
			std::fill(bgwinPrio.begin(), bgwinPrio.end(), false);

			SetMode(PPU_MODE_2);

			oamOffset = 0;

			numOAMPrio1 = 0;
			numOAMPrio0 = 0;
		}

		void GameboyGPU::EnterMode3() {
			u8& ly = memInstance->GetIO(LY_ADDR);

			SetMode(PPU_MODE_3);
		}

		void GameboyGPU::EnterMode0() {
			SetMode(PPU_MODE_0);

			VRAMDMANextBlock();
		}

		void GameboyGPU::EnterMode1() {
			SetMode(PPU_MODE_1);

			memInstance->RequestInterrupts(IRQ_VBLANK);

			drawWindow = false;
		}

		void GameboyGPU::SetMode(const int& _mode) {
			u8& stat = memInstance->GetIO(STAT_ADDR);

			graphicsCtx->mode = _mode;
			SET_MODE(stat, _mode);
		}



		void GameboyGPU::DrawScanlineDMG(const u8& _ly) {
			if (graphicsCtx->obj_enable) {
				if (presentObjPrio0Set) { DrawObjectsDMG(_ly, OAMPrio0, numOAMPrio0, true); }
			}

			if (graphicsCtx->bg_win_enable) {
				if (presentBackgroundSet) { DrawBackgroundDMG(_ly); }
				if (graphicsCtx->win_enable) {
					if (presentWindowSet) { DrawWindowDMG(_ly); }
				}
			}

			if (graphicsCtx->obj_enable) {
				if (presentObjPrio1Set) { DrawObjectsDMG(_ly, OAMPrio1, numOAMPrio1, false); }
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

		void GameboyGPU::DrawObjectsDMG(const u8& _ly, const int* _objects, const int& _num_objects, const bool& _no_prio) {
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
				DrawTileOBJ(x_pos - 8, ly, palette, _no_prio, x_flip);
			}
		}

		void GameboyGPU::DrawTileOBJ(const int& _x, const int& _y, const u32* _color_palette, const bool& _no_prio, const bool& _x_flip) {
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

						if (color_index > 0 && !bgwinPrio[x]) {
							color = _color_palette[color_index];
							image_offset_x = x * TEX2D_CHANNELS;

							u32 color_mask = 0xFF000000;
							for (int k = 0, j = 3 * 8; j > -1; j -= 8, k++) {
								imageData[image_offset_y + image_offset_x + k] = (u8)((color & color_mask) >> j);
								color_mask >>= 8;
							}

							if (_no_prio) { objNoPrio[x] = true; }
						}
					}

					bit_mask <<= 1;
					x++;
				}
			} else {
				for (int i = 7; i > -1; i--) {
					if (x > -1 && x < PPU_SCREEN_X) {
						color_index = (((tileDataCur[1] & bit_mask) >> i) << 1) | ((tileDataCur[0] & bit_mask) >> i);

						if (color_index > 0 && !bgwinPrio[x]) {
							color = _color_palette[color_index];
							image_offset_x = x * TEX2D_CHANNELS;

							u32 color_mask = 0xFF000000;
							for (int k = 0, j = 3 * 8; j > -1; j -= 8, k++) {
								imageData[image_offset_y + image_offset_x + k] = (u8)((color & color_mask) >> j);
								color_mask >>= 8;
							}

							if (_no_prio) { objNoPrio[x] = true; }
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
			for (int i = 7; i > -1; i--) {
				if (x > -1 && x < PPU_SCREEN_X) {
					color_index = (((tileDataCur[1] & bit_mask) >> i) << 1) | ((tileDataCur[0] & bit_mask) >> i);

					bool draw = false;
					if (objNoPrio[x]) {
						draw = color_index != 0;
					} else {
						draw = true;
					}

					if (draw) {
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
				if (presentObjPrio0Set) { DrawObjectsCGB(_ly, OAMPrio0, numOAMPrio0, true); }
			}

			if (presentBackgroundSet) { DrawBackgroundCGB(_ly); }

			if (graphicsCtx->win_enable) {
				if (presentWindowSet) { DrawWindowCGB(_ly); }
			}

			if (graphicsCtx->obj_enable) {
				if (presentObjPrio1Set) { DrawObjectsCGB(_ly, OAMPrio1, numOAMPrio1, false); }
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
				bank = (tile_attr & BG_ATTR_VRAM_BANK_CGB ? 1 : 0);
				x_flip = (tile_attr & BG_ATTR_FLIP_HORIZONTAL ? true : false);
				y_flip = (tile_attr & BG_ATTR_FLIP_VERTICAL ? true : false);
				prio = (tile_attr & BG_ATTR_OAM_PRIORITY ? true : false);

				if (y_flip) {
					y_clip_ = (PPU_TILE_SIZE_Y - 1) - y_clip;
				} else {
					y_clip_ = y_clip;
				}

				FetchTileDataBGWIN(tile_offset, y_clip_ * 2, bank);
				DrawTileBGWINCGB(x - x_clip, ly, graphicsCtx->cgb_bgp_color_palettes[palette_index], x_flip, prio);
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
				int bank;
				int palette_index;
				bool x_flip;
				bool y_flip;
				bool prio;

				auto tilemap_offset = (int)graphicsCtx->win_tilemap_offset;

				int tilemap_offset_y = ((ly - wy) / PPU_TILE_SIZE_Y) * PPU_TILEMAP_SIZE_1D;
				int tilemap_offset_x;

				int y_clip = (ly - wy) % PPU_TILE_SIZE_Y;
				int x_clip = wx_ % PPU_TILE_SIZE_X;

				int y_clip_;

				u8 tile_offset;
				u8 tile_attr;
				for (int x = 0; x < PPU_SCREEN_X + x_clip; x += PPU_TILE_SIZE_X) {
					tilemap_offset_x = (x - wx_) / PPU_TILE_SIZE_X;

					tile_offset = graphicsCtx->VRAM_N[0][tilemap_offset + tilemap_offset_y + tilemap_offset_x];

					int index = tilemap_offset + tilemap_offset_y + tilemap_offset_x;
					tile_offset = graphicsCtx->VRAM_N[0][index];
					tile_attr = graphicsCtx->VRAM_N[1][index];

					palette_index = tile_attr & BG_ATTR_PALETTE_CGB;
					bank = (tile_attr & BG_ATTR_VRAM_BANK_CGB ? 1 : 0);
					x_flip = (tile_attr & BG_ATTR_FLIP_HORIZONTAL ? true : false);
					y_flip = (tile_attr & BG_ATTR_FLIP_VERTICAL ? true : false);
					prio = (tile_attr & BG_ATTR_OAM_PRIORITY ? true : false);

					if (y_flip) {
						y_clip_ = (PPU_TILE_SIZE_Y - 1) - y_clip;
					} else {
						y_clip_ = y_clip;
					}

					FetchTileDataBGWIN(tile_offset, y_clip_ * 2, bank);
					DrawTileBGWINCGB(x, ly, graphicsCtx->dmg_bgp_color_palette, x_flip, prio);
				}
			}
		}

		void GameboyGPU::DrawTileBGWINCGB(const int& _x, const int& _y, const u32* _color_palette, const bool& _x_flip, const bool& _prio) {
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

						bool draw = false;

						if (objNoPrio[x]) {
							draw = color_index != 0;
						} else {
							draw = true;
						}

						if (draw) {
							color = _color_palette[color_index];

							image_offset_x = x * TEX2D_CHANNELS;

							u32 color_mask = 0xFF000000;
							for (int k = 0, j = 3 * 8; j > -1; j -= 8, k++) {
								imageData[image_offset_y + image_offset_x + k] = (u8)((color & color_mask) >> j);
								color_mask >>= 8;
							}

							if (_prio && color_index != 0) { bgwinPrio[x] = true; }
						}


					}

					bit_mask <<= 1;
					x++;
				}
			} else {
				for (int i = 7; i > -1; i--) {
					if (x > -1 && x < PPU_SCREEN_X) {
						color_index = (((tileDataCur[1] & bit_mask) >> i) << 1) | ((tileDataCur[0] & bit_mask) >> i);

						bool draw = false;

						if (objNoPrio[x]) {
							draw = color_index != 0;
						} else {
							draw = true;
						}

						if (draw) {
							color = _color_palette[color_index];

							image_offset_x = x * TEX2D_CHANNELS;

							u32 color_mask = 0xFF000000;
							for (int k = 0, j = 3 * 8; j > -1; j -= 8, k++) {
								imageData[image_offset_y + image_offset_x + k] = (u8)((color & color_mask) >> j);
								color_mask >>= 8;
							}

							if (_prio && color_index != 0) { bgwinPrio[x] = true; }
						}
					}

					bit_mask >>= 1;
					x++;
				}
			}
		}

		void GameboyGPU::DrawObjectsCGB(const u8& _ly, const int* _objects, const int& _num_objects, const bool& _no_prio) {
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

				int palette_index = flags & OBJ_ATTR_PALETTE_CGB;
				u32* palette = graphicsCtx->cgb_obp_color_palettes[palette_index];
				int bank = flags & OBJ_ATTR_VRAM_BANK_CGB ? 1 : 0;

				bool x_flip = (flags & OBJ_ATTR_X_FLIP) ? true : false;

				FetchTileDataOBJ(tile_offset, y_clip * 2, bank);
				DrawTileOBJ(x_pos - 8, ly, palette, _no_prio, x_flip);
			}
		}

		void GameboyGPU::SearchOAMDMG(const u8& _ly) {
			if (graphicsCtx->obj_enable) {
				if (oamOffset < OAM_SIZE) {
					int y_pos;
					int ly = _ly;
					bool no_prio;
					bool add_entry = false;

					y_pos = (int)graphicsCtx->OAM[oamOffset + OBJ_ATTR_Y];

					add_entry =
						(graphicsCtx->obj_size_16 && ((y_pos - ly) > 0 && (y_pos - _ly) < (16 + 1))) ||
						((y_pos - ly) > 8 && (y_pos - _ly) < (16 + 1));

					if (add_entry) {
						no_prio = (graphicsCtx->OAM[oamOffset + OBJ_ATTR_FLAGS] & OBJ_ATTR_PRIO) ? true : false;

						if (numOAMPrio0 + numOAMPrio1 < 10) {
							if (no_prio) {
								OAMPrio0[numOAMPrio0] = oamOffset;
								numOAMPrio0++;
							} else {
								OAMPrio1[numOAMPrio1] = oamOffset;
								numOAMPrio1++;
							}
						}
					}

					oamOffset += PPU_OBJ_ATTR_BYTES;
				}
			}
		}

		void GameboyGPU::SearchOAMCGB(const u8& _ly) {
			if (graphicsCtx->obj_enable) {
				if (oamOffset < OAM_SIZE) {
					int y_pos;
					int ly = _ly;
					bool no_prio;
					bool add_entry = false;

					y_pos = (int)graphicsCtx->OAM[oamOffset + OBJ_ATTR_Y];

					add_entry =
						(graphicsCtx->obj_size_16 && ((y_pos - ly) > 0 && (y_pos - _ly) < (16 + 1))) ||
						((y_pos - ly) > 8 && (y_pos - _ly) < (16 + 1));

					if (add_entry) {
						no_prio = (graphicsCtx->OAM[oamOffset + OBJ_ATTR_FLAGS] & OBJ_ATTR_PRIO) ? true : false;

						if (numOAMPrio0 + numOAMPrio1 < 10) {
							if (no_prio && graphicsCtx->obj_prio) {
								OAMPrio0[numOAMPrio0] = oamOffset;
								numOAMPrio0++;
							} else {
								OAMPrio1[numOAMPrio1] = oamOffset;
								numOAMPrio1++;
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


		void GameboyGPU::VRAMDMANextBlock() {
			if (graphicsCtx->vram_dma) {
				if (!machineCtx->halted) {
					u8& hdma5 = memInstance->GetIO(CGB_HDMA5_ADDR);
					int length = (int)(hdma5 & 0x7F) + 1;

					int bank;

					u16& source_addr = graphicsCtx->vram_dma_src_addr;
					u16& dest_addr = graphicsCtx->vram_dma_dst_addr;

					switch (graphicsCtx->vram_dma_mem) {
					case MEM_TYPE::ROM0:
						memcpy(&graphicsCtx->VRAM_N[machineCtx->vram_bank_selected][dest_addr], &memInstance->GetBank(MEM_TYPE::ROM0, 0)[source_addr], 0x10);
						break;
					case MEM_TYPE::ROMn:
						bank = machineCtx->rom_bank_selected;
						memcpy(&graphicsCtx->VRAM_N[machineCtx->vram_bank_selected][dest_addr], &memInstance->GetBank(MEM_TYPE::ROMn, bank)[source_addr], 0x10);
						break;
					case MEM_TYPE::RAMn:
						bank = machineCtx->ram_bank_selected;
						memcpy(&graphicsCtx->VRAM_N[machineCtx->vram_bank_selected][dest_addr], &memInstance->GetBank(MEM_TYPE::RAMn, bank)[source_addr], 0x10);
						break;
					case MEM_TYPE::WRAM0:
						memcpy(&graphicsCtx->VRAM_N[machineCtx->vram_bank_selected][dest_addr], &memInstance->GetBank(MEM_TYPE::WRAM0, 0)[source_addr], 0x10);
						break;
					case MEM_TYPE::WRAMn:
						bank = machineCtx->wram_bank_selected;
						memcpy(&graphicsCtx->VRAM_N[machineCtx->vram_bank_selected][dest_addr], &memInstance->GetBank(MEM_TYPE::WRAMn, bank)[source_addr], 0x10);
						break;
					}

					--length;

					int machine_cycles = (VRAM_DMA_MC_PER_BLOCK * machineCtx->currentSpeed) + 1;
					for (int i = 0; i < machine_cycles; i++) {
						coreInstance->TickTimers();
					}

					if (length == 0) {
						graphicsCtx->vram_dma = false;
						hdma5 = 0xFF;
						//LOG_ERROR("finish");
					} else {
						source_addr += 0x10;
						dest_addr += 0x10;
						--hdma5;
					}

					//LOG_INFO("LY: ", std::format("{:d}", memInstance->GetIO(LY_ADDR)), "; source: ", std::format("0x{:04x}", source_addr), "; dest: ", std::format("0x{:04x}", dest_addr), "; remaining: ", length * 0x10);
				}
			}
		}

		void GameboyGPU::OAMDMANextBlock() {
			if (graphicsCtx->oam_dma) {
				if (!machineCtx->halted) {
					int bank;

					int& counter = graphicsCtx->oam_dma_counter;
					u16& source_addr = graphicsCtx->oam_dma_src_addr;

					switch (graphicsCtx->oam_dma_mem) {
					case MEM_TYPE::ROM0:
						graphicsCtx->OAM[counter] = memInstance->GetBank(MEM_TYPE::ROM0, 0)[source_addr];
						break;
					case MEM_TYPE::ROMn:
						bank = machineCtx->rom_bank_selected;
						graphicsCtx->OAM[counter] = memInstance->GetBank(MEM_TYPE::ROMn, bank)[source_addr];
						break;
					case MEM_TYPE::RAMn:
						bank = machineCtx->ram_bank_selected;
						graphicsCtx->OAM[counter] = memInstance->GetBank(MEM_TYPE::RAMn, bank)[source_addr];
						break;
					case MEM_TYPE::WRAM0:
						graphicsCtx->OAM[counter] = memInstance->GetBank(MEM_TYPE::WRAM0, 0)[source_addr];
						break;
					case MEM_TYPE::WRAMn:
						bank = machineCtx->wram_bank_selected;
						graphicsCtx->OAM[counter] = memInstance->GetBank(MEM_TYPE::WRAMn, bank)[source_addr];
						break;
					}

					counter++;
					if (counter == OAM_DMA_LENGTH) {
						graphicsCtx->oam_dma = false;
						//LOG_ERROR("done");
					} else {
						source_addr++;
					}

					//LOG_INFO("OAM Addr.", format("{:04x}", source_addr), "; Index ", format("{:d}", counter));
				}
			}
		}
	}
}