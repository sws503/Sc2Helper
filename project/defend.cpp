#include "memibot.h"

void MEMIBot::Defend() {
	const ObservationInterface* observation = Observation();
	Units Oracles = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_ORACLE));
	Units nexus = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));
	size_t CurrentOracle = Oracles.size();

	Units Workers = observation->GetUnits(Unit::Alliance::Self, IsWorker());
	Units EnemyWorkers = observation->GetUnits(Unit::Alliance::Enemy, IsWorker());
	size_t cannon_count = observation->GetUnits(Unit::Alliance::Enemy, Rusher(startLocation_)).size();
	size_t EnemyWorkercount = observation->GetUnits(Unit::Alliance::Enemy, IsWorker()).size();
	Units EnemyCannon = observation->GetUnits(Unit::Alliance::Enemy, Rusher(startLocation_));


	Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
	Units enemyUnitsInRegion;

	enemyUnitsInRegion.clear();
	for (const auto & unit : enemy_units)
	{
		if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_OVERLORD || unit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_OBSERVER || unit->unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_REAPER)
		{
			continue;
		}

		if (Distance2D(startLocation_, unit->pos) < base_range + getAttackRangeGround(unit))
		{
			Chat("Enemy Captured 1");
			enemyUnitsInRegion.push_back(unit);
		}
	}

	if (!0)
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
	if (OracleTrained) {

	}

	if (enemyUnitsInRegion.size() > 3)
	{
		EnemyRush = true;
	}
	if (enemyUnitsInRegion.size() == 0)
	{
		EnemyRush = false;
	}
}
