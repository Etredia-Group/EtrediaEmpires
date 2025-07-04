#pragma once
#include <unordered_map>
#include <ee_economy/economy.hpp>

namespace ee_ecs
{

    struct InitConfiguration
	{
		std::unordered_map<short, ee_economy::container> states;
	};

}