#include "GraphicsUnitSM83.h"

#include "gameboy_defines.h"

void GraphicsUnitSM83::NextFrame() {
	isrFlags = 0x00;




	mem_instance->RequestInterrupts(isrFlags | ISR_VBLANK);
	graphics_ctx->LY = PPU_VBLANK;
}