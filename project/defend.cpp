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
		SmartAttackUnit(unit, target);
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

	/*for (const auto & enemyunit : enemyUnitsInRegion)
	{
		if (enemyunit->unit_type == UNIT_TYPEID::PROTOSS_PYLON || enemyunit->unit_type == UNIT_TYPEID::PROTOSS_PHOTONCANNON)
		{
			Units fighters = FindUnitsNear(enemyunit->pos, 15.0f, Workers);
			for (const auto & fighter : fighters)
			{
				SmartAttackUnit(fighter, enemyunit);
			}
		}
		else
		{
			Units fighters = FindUnitsNear(enemyunit->pos, 15.0f, Workers);
			for (const auto & fighter : fighters)
			{
				SmartAttackUnit(fighter, enemyunit);
			}


			//const Unit * fighter = FindNearestUnit(enemyunit->pos, Workers);
			//SmartAttackUnit(fighter, enemyunit);
		}
		
		
	}*/
}
