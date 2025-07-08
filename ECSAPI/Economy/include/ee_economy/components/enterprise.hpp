#pragma once
#include <entt/entt.hpp>
#include <vector>

namespace ee_economy
{
	struct enterprise
	{
		bool is_building = false;
		entt::entity state;

		double efficiency = 0.;
		double base_efficiency = 0.;
		double factor_efficiency = 1.;

		std::vector<entt::entity> links_from;
		std::vector<entt::entity> links_to;

		inline void update_efficiency()
		{
			this->efficiency = this->base_efficiency * this->factor_efficiency;
		}

		void add_base_efficiency(double delta)
		{
			this->base_efficiency += delta;
			update_efficiency();
		}

		void add_factor_efficiency(double delta)
		{
			this->factor_efficiency += delta;
			update_efficiency();
		}

		void multiply_base_efficiency(double factor)
		{
			this->base_efficiency *= factor;
			update_efficiency();
		}

		void multiply_factor_efficiency(double factor)
		{
			this->factor_efficiency *= factor;
			update_efficiency();
		}
	};
}