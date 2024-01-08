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
	static GraphicsUnitBase* getInstance(graphics_information& _graphics_info);
	static void resetInstance();

	// clone/assign protection
	GraphicsUnitBase(GraphicsUnitBase const&) = delete;
	GraphicsUnitBase(GraphicsUnitBase&&) = delete;
	GraphicsUnitBase& operator=(GraphicsUnitBase const&) = delete;
	GraphicsUnitBase& operator=(GraphicsUnitBase&&) = delete;

	// members
	virtual bool ProcessGPU(const int& _substep) = 0;

protected:
	// constructor
	GraphicsUnitBase() = default;
	~GraphicsUnitBase() = default;

	virtual void SetGraphicsParameters() = 0;

private:
	static GraphicsUnitBase* instance;
};