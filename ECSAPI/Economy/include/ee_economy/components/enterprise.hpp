#pragma once
#include <ee_economy/economy.hpp>
#include <entt/entt.hpp>
#include <vector>

namespace ee_economy
{
	struct Enterprise
	{
		bool isBuilding = false;
		entt::entity state;
		ACTION_TYPE action = ACTION_TYPE::STORE;

		/*
		* Mining - Amount of mined resource
		* Delivering - Amount of resource that can be delivered by one truck
		* Production - 
		*/
		double efficiency = 0.;
		double baseEfficiency = 0.;
		double factorEfficiency = 1.;

		std::vector<entt::entity> linksFrom;
		std::vector<entt::entity> linksTo;

		uint16_t staffSize = 1;
		
		// This can be splitted in percents between bank enterprises in future
		double totalMoney = 0;
		double salary = 0;
		bool isBankrupt = false;

		void setMoney(double value)
		{
			totalMoney = value;
		}

		inline void updateEfficiency()
		{
			this->efficiency = this->baseEfficiency * this->factorEfficiency;
		}

		void setBaseEfficiency(double value)
		{
			this->baseEfficiency = value;
			updateEfficiency();
		}

		void setFactorEfficiency(double value)
		{
			this->factorEfficiency = value;
			updateEfficiency();
		}
	};
}