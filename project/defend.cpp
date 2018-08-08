#include "memibot.h"

struct IsNearbyArmies {
	IsNearbyArmies(const ObservationInterface* obs, Point2D MyPosition, int Radius) :
		observation_(obs), mp(MyPosition), radius(Radius) {}

	bool operator()(const Unit& unit) {
		auto attributes = observation_->GetUnitTypeData().at(unit.unit_type).attributes;
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::ZERG_OVERLORD: return false;
		case UNIT_TYPEID::ZERG_LARVA: return false;
		case UNIT_TYPEID::ZERG_EGG: return false;
		case UNIT_TYPEID::TERRAN_MULE: return false;
		case UNIT_TYPEID::TERRAN_NUKE: return false;
		case UNIT_TYPEID::ZERG_DRONE:
		case UNIT_TYPEID::TERRAN_SCV:
		case UNIT_TYPEID::PROTOSS_PROBE:return false;

		case UNIT_TYPEID::PROTOSS_PHOTONCANNON:
		case UNIT_TYPEID::TERRAN_BUNKER:
		case UNIT_TYPEID::ZERG_SPINECRAWLER:
		case UNIT_TYPEID::TERRAN_PLANETARYFORTRESS: return Distance2D(mp, unit.pos) < radius;


		default:
			for (const auto& attribute : attributes) {
				if (attribute == Attribute::Structure) {
					return false;
				}
			}
			return Distance2D(mp, unit.pos) < radius;
		}
	}
private:
	const ObservationInterface* observation_;
	Point2D mp;
	int radius;
};

bool MEMIBot::DefendDuty(const Unit * unit)
{
	const Unit * target = GetTarget(unit, enemyUnitsInRegion);
	Units bases = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));

	if (Distance2D(unit->pos, startLocation_) > 50)
	{
		if (target == nullptr)
		{
			Actions()->UnitCommand(unit, ABILITY_ID::STOP);
		}
		else
		{
			FleeKiting(unit, target);
		}
		return true;
	}
	else if (target != nullptr && Distance2D(unit->pos, target->pos) < 20)
	{
		Kiting(unit, target);
		return true;
	}

	return false;
}

void MEMIBot::Defend() {
	const ObservationInterface* observation = Observation();
	Units Stalkers = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_STALKER));
	Units bases = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));
	size_t CurrentStalker = Stalkers.size();

	Units Workers = observation->GetUnits(Unit::Alliance::Self, IsWorker());
	Units EnemyWorkers = observation->GetUnits(Unit::Alliance::Enemy, IsWorker());
	size_t cannon_count = observation->GetUnits(Unit::Alliance::Enemy, Rusher(startLocation_)).size();
	size_t EnemyWorkercount = observation->GetUnits(Unit::Alliance::Enemy, IsWorker()).size();
	Units EnemyCannon = observation->GetUnits(Unit::Alliance::Enemy, Rusher(startLocation_));


	Units enemy_units = FindUnitsNear(startLocation_, 50, Unit::Alliance::Enemy);
	Units enemy_armies = FindUnitsNear(startLocation_, 50, Unit::Alliance::Enemy, IsArmy(Observation()));
	Units my_armies = observation->GetUnits(Unit::Alliance::Self, IsArmy(observation));

	enemyUnitsInRegion.clear();
	const float base_range = 1;

	for (const auto & unit : enemy_units)
	{
		for (const auto & base : bases) //기지별로
		{
			if (Distance2D(base->pos, startLocation_) < 15)
			{
				if (Distance2D(base->pos, unit->pos) < base_range + 10)
				{
					enemyUnitsInRegion.push_back(unit);
					break;
				}
			}
			else if (Distance2D(base->pos, unit->pos) < base_range)
			{
				enemyUnitsInRegion.push_back(unit);
				break;
			}
		}
	}
	
	if (!enemyUnitsInRegion.empty() && my_armies.empty())
	{
		for (const auto & worker : Workers)
		{
			const Unit* target = GetTarget(worker, enemyUnitsInRegion);

			if (target != nullptr && Distance2D(worker->pos, target->pos) < 10 && Killers.size() <= enemyUnitsInRegion.size() + 1)
			{
				Killers.push_back(worker);
			}
		}
		std::cout << enemyUnitsInRegion.size();
		for (auto& it = Killers.begin(); it != Killers.end();)
		{
			const Unit* killer = *it;
			if (Distance2D(killer->pos, startLocation_) > 10 || !killer->is_alive)
			{
				std::cout << " 방금 뺀건 ";
				Killers.erase(it++);
				Actions()->UnitCommand(killer, ABILITY_ID::STOP);
				continue;
			}
			it++;
			const Unit* target = GetTarget(killer, enemyUnitsInRegion);
			SmartAttackUnit(killer, target);
		}
	}
	else
	{
		Killers.clear();
	}

	EnemyRush = !enemyUnitsInRegion.empty();
	ManyEnemyRush = (enemyUnitsInRegion.size() > 3);

	/*Units defenders = FindUnitsNear(startLocation_, 50, Unit::Alliance::Self, IsArmy(Observation()));
	for (const auto & defender : defenders)
	{
		const Unit* target = GetTarget(defender, enemyUnitsInRegion);
		if (target != nullptr)
		{
			SmartAttackUnit(defender, target);
		}
	}*/

	/*for (const auto & base : bases) //기지별로
	{
		if (enemyUnitsInRegion.size() > 0)
		{
			Units defenders = FindUnitsNear(base, 30, Unit::Alliance::Enemy, IsArmy(Observation()));

			Units defenders = observation->GetUnits(Unit::Alliance::Self, IsNearbyArmies(observation, base->pos, 35));

			
			for (const auto & defender : defenders)
			{
				const Unit* target = GetTarget(defender, enemyUnitsInRegion);
				SmartAttackUnit(defender, target);
			}
		}
	}*/
	/*if (false)
	{
		if (Killers.size() < enemyUnitsInRegion.size()) {
			Killers.resize(enemyUnitsInRegion.size(), nullptr);
		}

		if (enemyUnitsInRegion.size() > 0)
		{
			for (int i = 0; i<Killers.size();)
			{
				auto& Killer = Killers.at(i);

				if (Killer == nullptr || !Killer->is_alive)
				{
					Chat("Killer Captured 2");
					GetRandomUnit(Killer, observation, UNIT_TYPEID::PROTOSS_PROBE);
					if (Killer == probe_scout || Killer == probe_forward)
						continue;
				}
				i++;
			}
			Chat("Killer Captured 2");
			Actions()->UnitCommand(Killers, ABILITY_ID::SMART, enemyUnitsInRegion.front());
		}
		else //enemyUnitsInRegion.size() == 0
		{
			//
		}

		if (!EnemyCannon.empty())
		{
			PhotonRush = true;
			for (const auto& worker : Workers) {
				Actions()->UnitCommand(worker, ABILITY_ID::ATTACK, EnemyCannon.front()->pos);
			}
		}
		else {
			if (PhotonRush == true) {
				for (const auto& worker : Workers) {
					if (!worker->orders.empty() && worker->orders.front().ability_id == ABILITY_ID::ATTACK)
						Actions()->UnitCommand(worker, ABILITY_ID::STOP);
				}
			}
			PhotonRush = false;
		}
	}

	if (enemyUnitsInRegion.size() > 3)
	{
		EnemyRush = true;
	}
	if (enemyUnitsInRegion.size() == 0)
	{
		EnemyRush = false;
	}*/
}
