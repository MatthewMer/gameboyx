#pragma once

#include "Mmu.h"

class GraphicsUnit
{
public:
	// get/reset instance
	static GraphicsUnit* getInstance(const Cartridge& _cart_obj);
	static void resetInstance();

	// clone/assign protection
	GraphicsUnit(GraphicsUnit const&) = delete;
	GraphicsUnit(GraphicsUnit&&) = delete;
	GraphicsUnit& operator=(GraphicsUnit const&) = delete;
	GraphicsUnit& operator=(GraphicsUnit&&) = delete;

protected:
	// constructor
	GraphicsUnit() = default;

	Mmu* mmu_instance;

private:
	static GraphicsUnit* instance;
	~GraphicsUnit() = default;
};


class GraphicsUnitSM83 : protected GraphicsUnit
{
public:
	friend class GraphicsUnit;

private:
	// constructor
	explicit GraphicsUnitSM83(const Cartridge& _cart_obj);
	// destructor
	~GraphicsUnitSM83() = default;
};