#pragma once

namespace HardwareMgr {
	bool InitHardware();
	void ShutdownHardware();
	void NextFrame();
	void RenderFrame();
	void ProcessInput(bool& _running);
	void ToggleFullscreen();
};

