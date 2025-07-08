#pragma once 
#include "defines.hpp"
#include "components/container.hpp"

namespace ee_economy
{
	struct enterprise_defaults
	{
		container container;

		RESOURCE_TYPE resource_target = RESOURCE_TYPE::NONE;
		double base_efficiency = 0.0;
		double factor_efficiency = 1.0;
	};
}