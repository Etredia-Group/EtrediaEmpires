#pragma once
#include <entt/entt.hpp>

namespace ee_economy
{
	void enterprise_routine(entt::registry& registry);

	void mining(entt::registry& registry);
	// void producing(entt::registry& registry);
	void delivering(entt::registry& registry);
	// void storaging(entt::registry& registry); // consume money for storage time
	// void destructing(entt::registry& registry);
	// void building(entt::registry& registry);
	// void trading(entt::registry& registry);
}