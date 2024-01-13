#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	The GraphicsUnitBase class is just used to derive the different GPU types (NES, GB, ..) of the emulated hardware from it.
*	The member methods are as generic as possible to fit every GPU type without exposing the GPUs
*	specific internals.
*/

#include "Cartridge.h"
#include "VulkanMgr.h"

class GraphicsUnitBase
{
public:
	// get/reset instance
	static GraphicsUnitBase* getInstance(graphics_information& _graphics_info, VulkanMgr* _graphics_mgr);
	static GraphicsUnitBase* getInstance();
	static void resetInstance();

	// clone/assign protection
	GraphicsUnitBase(GraphicsUnitBase const&) = delete;
	GraphicsUnitBase(GraphicsUnitBase&&) = delete;
	GraphicsUnitBase& operator=(GraphicsUnitBase const&) = delete;
	GraphicsUnitBase& operator=(GraphicsUnitBase&&) = delete;

	// members
	virtual void ProcessGPU(const int& _ticks) = 0;
	virtual int GetDelayTime() const = 0;
	virtual int GetTicksPerFrame(const float& _clock) const = 0;
	virtual int GetFrames() = 0;

protected:
	// constructor
	GraphicsUnitBase() = default;
	~GraphicsUnitBase() = default;

	virtual void SetGraphicsParameters() = 0;

	int frameCounter = 0;
	int tickCounter = 0;

	VulkanMgr* graphicsMgr;

private:
	static GraphicsUnitBase* instance;
};