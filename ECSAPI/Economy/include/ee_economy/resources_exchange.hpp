// Rules for resources exchange in base before modifiers applying

#pragma once
#include <unordered_map>
#include "defines.hpp"

namespace ee_ecs
{
	struct exchange_rule
	{
		double out_value;
		
		std::unordered_map<RESOURCE_TYPE, double> consume;
	};

	const std::unordered_map<RESOURCE_TYPE, exchange_rule> resources_exchange = {
		{
			RESOURCE_TYPE::FABRIC,
			exchange_rule {
				.out_value = 2.,
				.consume = {
					{RESOURCE_TYPE::WOOL, 3.}
				}
			}
		}
	};
}