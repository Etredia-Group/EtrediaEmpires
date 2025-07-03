#pragma once 
#include "defines.hpp"
#include "components/container.hpp"

namespace ee_ecs
{
	struct enterprise_defaults
	{
		container container;

		RESOURCE_TYPE resource_target;
		double base_efficiency_flat;
	};
}