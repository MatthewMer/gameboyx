#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	The BaseGPU class is just used to derive the different GPU types (NES, GB, ..) of the emulated hardware from it.
*	The member methods are as generic as possible to fit every GPU type without exposing the GPUs
*	specific internals.
*/

#include "GameboyCartridge.h"
#include "GraphicsMgr.h"

class BaseGPU
{
public:
	// get/reset instance
	static BaseGPU* getInstance(graphics_information& _graphics_info, GraphicsMgr* _graphics_mgr);
	static BaseGPU* getInstance();
	static void resetInstance();

	// clone/assign protection
	BaseGPU(BaseGPU const&) = delete;
	BaseGPU(BaseGPU&&) = delete;
	BaseGPU& operator=(BaseGPU const&) = delete;
	BaseGPU& operator=(BaseGPU&&) = delete;

	// members
	virtual void ProcessGPU(const int& _ticks) = 0;
	virtual int GetDelayTime() const = 0;
	virtual int GetTicksPerFrame(const float& _clock) const = 0;
	virtual int GetFrames() = 0;

protected:
	// constructor
	BaseGPU() = default;
	virtual ~BaseGPU() = default;

	int frameCounter = 0;
	int tickCounter = 0;

	GraphicsMgr* graphicsMgr = nullptr;

private:
	static BaseGPU* instance;
};