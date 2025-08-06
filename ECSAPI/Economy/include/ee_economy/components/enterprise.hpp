#pragma once
#include <ee_economy/economy.hpp>
#include <entt/entt.hpp>
#include <vector>

namespace ee_economy
{
	struct enterprise
	{
		bool is_building = false;
		entt::entity state;
		ACTION_TYPE action = ACTION_TYPE::STORE;

		/*
		* Mining - Amount of mined resource
		* Delivering - Amount of resource that can be delivered by one truck
		* Production - 
		*/
		double efficiency = 0.;
		double base_efficiency = 0.;
		double factor_efficiency = 1.;

		std::vector<entt::entity> links_from;
		std::vector<entt::entity> links_to;

		uint16_t staff_size = 1;
		
		// This can be splitted in percents between bank enterprises in future
		double total_money = 0;
		double salary = 0;
		bool is_bankrupt = false;

		void set_money(double value)
		{
			total_money = value;
		}

		inline void update_efficiency()
		{
			this->efficiency = this->base_efficiency * this->factor_efficiency;
		}

		void set_base_efficiency(double value)
		{
			this->base_efficiency = value;
			update_efficiency();
		}

		void set_factor_efficiency(double value)
		{
			this->factor_efficiency = value;
			update_efficiency();
		}
	};
}