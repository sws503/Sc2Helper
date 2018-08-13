#include "memibot.h"

bool MEMIBot::DefendDuty(const Unit * unit)
{
	const Unit * target = GetTarget(unit, enemyUnitsInRegion);
	Units bases = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));

	if(target != nullptr && IsUnitInUnits(target, enemyUnitsInRegion) && Distance2D(unit->pos, target->pos) < 20)
	{
		Kiting(unit, target);
		return true;
	}

	return false;
}

bool MEMIBot::DefendDutyAttack(const Unit * unit)
{
	const Unit * target = GetTarget(unit, enemyUnitsInRegion);
	Units bases = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));

	if (target != nullptr && IsUnitInUnits(target, enemyUnitsInRegion) && Distance2D(unit->pos, target->pos) < 20)
	{
		if (!unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::ATTACK && unit->weapon_cooldown == 0.0f) // 현재 공격이 선딜상황임
		{
			//가만히 있도록 합시다
		}
		else
		{
			SmartAttackUnit(unit, target);
		}
		return true;
	}

	return false;
}

void MEMIBot::Defend() {
	const ObservationInterface* observation = Observation();
	Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
	Units workers = observation->GetUnits(Unit::Alliance::Self, IsWorker());
	Units my_armies = observation->GetUnits(Unit::Alliance::Self, IsArmy(observation));
	Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
	Units EnemyCannon = observation->GetUnits(Unit::Alliance::Enemy, Rusher(startLocation_));
	size_t cannon_count = EnemyCannon.size();

	enemyUnitsInRegion.clear();
	const float base_range = 10;

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
	EnemyRush = !enemyUnitsInRegion.empty();
	ManyEnemyRush = (enemyUnitsInRegion.size() >= 3);

	DefendWorkers();
	cancelbuilding();
}

void MEMIBot::cancelbuilding() {
	const ObservationInterface* observation = Observation();

	for (const auto& u : observation->GetUnits(Unit::Alliance::Self, IsStructure(observation))) {
		if (u->build_progress != 1.0f) continue;
		float hp_max = u->shield_max + u->health_max;
		float hp = u->shield + u->health;
		if ((hp / hp_max < 0.1f) && (u->build_progress > hp / hp_max)) {
			Actions()->UnitCommand(u, ABILITY_ID::CANCEL);
		}
	}
}