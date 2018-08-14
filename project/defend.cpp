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
		if (!unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::ATTACK && unit->weapon_cooldown == 0.0f) // ���� ������ ������Ȳ��
		{
			//������ �ֵ��� �սô�
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

	enemyUnitsInRegion.clear();
	const float base_range = 20;

	for (const auto & unit : enemy_units)
	{
		if (unit->unit_type == UNIT_TYPEID::ZERG_OVERLORD)
		{
			continue;
		}

		for (const auto & base : bases) //��������
		{
			if (Distance2D(base->pos, startLocation_) < 15)
			{
				if (Distance2D(base->pos, unit->pos) < 30)
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

	ControlWorkers();
	cancelnexus();
}

void MEMIBot::cancelnexus() {
	const ObservationInterface* observation = Observation();

	for (const auto& u : observation->GetUnits(Unit::Alliance::Self, IsTownHall())) {
		if (u->build_progress == 1.0f) continue;
		float hp_max = u->shield_max + u->health_max;
		float hp = u->shield + u->health;
		if ((hp / hp_max < 0.1f) && (u->build_progress > hp / hp_max)) {
			Actions()->UnitCommand(u, ABILITY_ID::CANCEL_BUILDINPROGRESS);
		}
	}
}