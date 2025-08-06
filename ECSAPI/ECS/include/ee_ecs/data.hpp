#pragma once
#include <unordered_map>
#include <ee_economy/economy.hpp>
#include <variant>

namespace ee_ecs
{

    struct init_configuration
	{
		std::unordered_map<int16_t, ee_economy::container> states;
	};

	struct enterprise_info
	{
		const ee_economy::enterprise* info = nullptr;
		const ee_economy::container* storage = nullptr;
		std::variant<
			const ee_economy::miner*,
			const ee_economy::deliverer*
		> special_info;
	};

}