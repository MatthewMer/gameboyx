#include "GraphicsUnitSM83.h"

#include "gameboy_defines.h"

void GraphicsUnitSM83::NextFrame() {
	if (graphics_ctx->LCDC & PPU_LCDC_ENABLE) {
		isrFlags = 0x00;

		// generate frame data
		SearchOAM();
		ReadVRAMandOAM();
		GenerateFrameData();
		// TODO: render result to texture or prepare data for vulkan renderpass

		// everything from here just pretends to have processed the scanlines (horizontal lines) of the LCD *****
		// request mode 0 to 2 interrupts (STAT)
		if (graphics_ctx->STAT & (PPU_STAT_MODE0_EN | PPU_STAT_MODE1_EN | PPU_STAT_MODE2_EN)) {
			isrFlags |= ISR_LCD_STAT;
		}

		// request ly compare interrupt (STAT) and set flag
		if ((graphics_ctx->STAT & PPU_STAT_LYC_SOURCE) && (graphics_ctx->LYC <= LCD_SCANLINES_VBLANK)) {
			graphics_ctx->STAT |= PPU_STAT_LYC_FLAG;
			isrFlags |= ISR_LCD_STAT;
		}
		else {
			graphics_ctx->STAT &= ~PPU_STAT_LYC_FLAG;
		}

		mem_instance->RequestInterrupts(isrFlags | ISR_VBLANK);
		graphics_ctx->LY = PPU_VBLANK;
	}
}

// search oam for objects
void GraphicsUnitSM83::SearchOAM() {

}

// read VRAM and OAM to generate picture
void GraphicsUnitSM83::ReadVRAMandOAM() {

}

// generate frame -> 
void GraphicsUnitSM83::GenerateFrameData() {

}