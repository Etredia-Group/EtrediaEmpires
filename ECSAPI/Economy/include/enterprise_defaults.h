#pragma once 
#include "components/container.h"

namespace ee_ecs
{
	struct enterprise_defaults
	{
		container container;

		RESOURCE_TYPE resource_target;
		double base_efficiency_flat;
	};
}