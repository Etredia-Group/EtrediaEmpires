// Rules for resources exchange in base before modifiers applying

#pragma once
#include <unordered_map>
#include "defines.hpp"

namespace ee_economy
{
	struct ExchangeRule
	{
		double outValue;
		
		std::unordered_map<RESOURCE_TYPE, double> consume;
	};

	const std::unordered_map<RESOURCE_TYPE, exchange_rule> resourcesExchange = {
		{
			RESOURCE_TYPE::FABRIC,
			exchange_rule {
				.outValue = 2.,
				.consume = {
					{RESOURCE_TYPE::WOOL, 3.}
				}
			}
		}
	};
}