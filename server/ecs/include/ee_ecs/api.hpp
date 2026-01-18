#pragma once
#include <vector>
#include "data.hpp"

namespace ee_ecs
{

	void init(InitConfiguration config);
	void start();

	template<typename  T>
		const std::vector<EnterpriseInfo> getAllEnterprises();
	//const EnterpriseInfo getEnterprise(uint32_t id);
}