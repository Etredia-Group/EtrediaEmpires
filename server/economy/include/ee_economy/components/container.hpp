#pragma once
#include "../defines.hpp"
#include <unordered_map>

namespace ee_economy
{
	struct Container
	{
		double capacity = 0.;
		double total = 0.;
		std::unordered_map<RESOURCE_TYPE, double> content;

		void add(RESOURCE_TYPE type, double delta)
		{
			this->content[type] += delta;
			this->total += delta;
		}
	};
}