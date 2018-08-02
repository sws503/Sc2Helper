#include "memibot.h"

Units two_stalkers;
bool selected = false;

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


	Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
	Units enemy_armies = observation->GetUnits(Unit::Alliance::Enemy, IsArmy(observation));
	Units my_armies = observation->GetUnits(Unit::Alliance::Enemy, IsArmy(observation));
	Units enemyUnitsInRegion;

	if (Stalkers.empty())
		return;
	else if (!Stalkers.empty() && CurrentStalker == 2 && !selected) {
		two_stalkers = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_STALKER));
		selected = true;
	}

	for (const auto & unit : two_stalkers)
	{

	}

	enemyUnitsInRegion.clear();
	const float base_range = 15;

	for (const auto & base : bases) //기지별로
	{
		for (const auto & unit : enemy_armies)
		{
			/*if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_OVERLORD || unit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_OBSERVER || unit->unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_REAPER)
			{
			continue;
			}*/
			if (Distance2D(base->pos, startLocation_) < 10)
			{
				if (Distance2D(base->pos, unit->pos) < 30)
				{
					enemyUnitsInRegion.push_back(unit);
				}
			}
			else if (Distance2D(base->pos, unit->pos) < base_range)
			{
				enemyUnitsInRegion.push_back(unit);
			}
		}
	}

	for (const auto & base : bases) //기지별로
	{
		if (enemyUnitsInRegion.size() > 0)
		{
			Units defenders = observation->GetUnits(Unit::Alliance::Self, IsNearbyArmies(observation, base->pos, 35));
			for (const auto & defender : defenders)
			{
				const Unit * target = GetTarget(defender, enemyUnitsInRegion);
				Kiting(defender, target);
			}
		}
	}

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
