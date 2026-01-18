#pragma once 
#include "defines.hpp"
#include "components/container.hpp"

namespace ee_economy
{
	struct EnterpriseDefaults
	{
		Container container;

		RESOURCE_TYPE resourceTarget = RESOURCE_TYPE::NONE;
		double baseEfficiency = 0.0;
		double factorEfficiency = 1.0;
	};
}