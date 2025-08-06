#pragma once
#include <vector>
#include "data.hpp"

namespace ee_ecs
{

	void init(init_configuration config);
	void start();

	template<typename  T>
		const std::vector<enterprise_info> get_all_enterprises();
	//const enterprise_info get_enterprise(uint32_t id);
}