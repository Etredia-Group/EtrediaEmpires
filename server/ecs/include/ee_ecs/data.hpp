#pragma once
#include <unordered_map>
#include <ee_economy/economy.hpp>
#include <variant>

namespace ee_ecs
{

    struct InitConfiguration
	{
		std::unordered_map<int16_t, ee_economy::Container> states;
	};

	struct EnterpriseInfo
	{
		const ee_economy::Enterprise* info = nullptr;
		const ee_economy::Container* storage = nullptr;
		std::variant<
			const ee_economy::Miner*,
			const ee_economy::Deliverer*
		> specialInfo;
	};

}