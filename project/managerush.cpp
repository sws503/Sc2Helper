#include "memibot.h"

struct IsAdept {
	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_ADEPT: return true;
		default: return false;
		}
	}
};

struct IsAdeptShade {
	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_ADEPTPHASESHIFT: return true;
		default: return false;
		}
	}
};

struct IsNearbyArmy {
	IsNearbyArmy(const ObservationInterface* obs, Point2D MyPosition, int Radius) : 
		observation_(obs), mp(MyPosition), radius(Radius) {}

	bool operator()(const Unit& unit) {
		auto attributes = observation_->GetUnitTypeData().at(unit.unit_type).attributes;
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::ZERG_OVERLORD: return false;
		case UNIT_TYPEID::ZERG_LARVA: return false;
		case UNIT_TYPEID::ZERG_EGG: return false;
		case UNIT_TYPEID::TERRAN_MULE: return false;
		case UNIT_TYPEID::TERRAN_NUKE: return false;
		
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

std::map<uint64_t, uint32_t> adept_map;

void MEMIBot::ManageRush() { // 5.17 오라클 유닛 관리 +6.25 폭풍함 유닛 관리
	const ObservationInterface* observation = Observation();
	Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
	Units enemy_army = observation->GetUnits(Unit::Alliance::Enemy, IsArmy(observation));

	Units Adepts = observation->GetUnits(Unit::Alliance::Self, IsAdept());
	Units AdeptShades = observation->GetUnits(Unit::Alliance::Self, IsAdeptShade());
	Units EnemyWorker = observation->GetUnits(Unit::Alliance::Enemy, IsWorker());
	size_t CurrentAdept = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ADEPT);

	

	Units RangedUnitTargets;

	Units rangedunits = observation->GetUnits(Unit::Alliance::Self, IsRanged(observation));
	Units ShadeNearEnemies;

	for (const auto& unit : AdeptShades) {
		ShadeNearEnemies = observation->GetUnits(Unit::Alliance::Enemy, IsNearbyArmy(observation, unit->pos, 15));

		
	}

	for (const auto& unit : rangedunits) {
		
		Units NearbyEnemies = observation->GetUnits(Unit::Alliance::Enemy, IsNearbyArmy(observation, unit->pos, 20)); //각 유닛의 근처에 있는 
		float TargetAttackRange = 0.0f;
		float UnitAttackRange = getAttackRangeGROUND(unit);

		// 타겟을 받아옵니다 *^^*
		const Unit * target = GetTarget(unit, NearbyEnemies);
		
		if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_STALKER)
		{
			float UnitHealth = unit->health + unit->shield;

			if (UnitHealth < 30)
			{
				StalkerBlinkEscape(unit, target);
			}
			if (getDpsGROUND(target) == 0.0f)
			{
				StalkerBlinkForward(unit, target);
			}

		}


		if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_ADEPT)
		{
			if (CurrentAdept < 8)
			{
				RetreatWithUnit(unit, advance_pylon_location);
			}
			else if (CurrentAdept >= 8)
			{
				if (target == nullptr)
				{
					ScoutWithUnit(unit, observation);
				}
				if (target != nullptr)
				{
					AdeptPhaseShift(unit, ShadeNearEnemies, NearbyEnemies);
					Kiting(unit, target);
				}
				
			}
		}


		if (target == nullptr)
		{
			/*if (enemy_units.empty())
			{
				ScoutWithUnit(unit, observation);
			}
			else
			{
				AttackWithUnit(unit, observation);
			}*/
		}
		else
		{
			Kiting(unit, target);
		}



		/*bool enemiesnear = false;
		Point2D KitingLocation;
		Point2D EnemyAveragePosition;
		if (!GetPosition(NearbyEnemies, Unit::Alliance::Enemy, EnemyAveragePosition))
		{
		return;
		}*/


		/*for (const auto& enemyarmy : NearbyEnemies)
		{
			if (!enemyarmy->is_alive)
			{
				continue;
			}

			float TargetAttackRange = getAttackRangeGROUND(enemyarmy);
			float distance = MinimumDistance2D(unit, enemyarmy);


			if (distance <= TargetAttackRange) //근처에 있을때만 KitingLocation을 업데이트한다
			{
				enemiesnear = true;
				KitingLocation = CalcKitingPosition(unit->pos, enemyarmy->pos);
				break;
			}
		}*/

		/*else if(!IsStructure(Observation())(* target) && Distance2D(unit->pos, target->pos) <= UnitAttackRange)
		{
			Kiting(unit, target);
		}*/

	}
}

void MEMIBot::AdeptPhaseShift(const Unit* unit, Units ShadeNearEnemies , Units NearbyEnemies)
{
	bool Timer = false;
	if (Distance2D(game_info_.enemy_start_locations.front(), unit->pos) <= 10) //적 기지근처에 있으면 우리 집으로 분신을 날린다
	{
		AdeptPhaseToLocation(unit, startLocation_, Timer);
	}
	else // 그 경우가 아니면 일반적으로 적 기지로 분신을 날린다
	{
		AdeptPhaseToLocation(unit, game_info_.enemy_start_locations.front(), Timer);
	}

	if (Timer)
	{
		std::cout << ShadeNearEnemies.size() << ">=" << NearbyEnemies.size() << "=" << (ShadeNearEnemies.size() >= NearbyEnemies.size()) << std::endl;

		if (ShadeNearEnemies.size() > NearbyEnemies.size())
		{
			Actions()->UnitCommand(unit, ABILITY_ID::CANCEL_ADEPTPHASESHIFT);
		}
	}
}

void MEMIBot::AdeptPhaseToLocation(const Unit* unit, Point2D Location , bool & Timer)
{
	Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_ADEPTPHASESHIFT, Location);

	const ObservationInterface* observation = Observation();
	uint32_t game_loop = Observation()->GetGameLoop();

	if (adept_map.count(unit->tag) == 0) {
		adept_map.insert(std::make_pair((unit->tag), observation->GetGameLoop()));
		Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_ADEPTPHASESHIFT, Location);
	}
	if (adept_map.find(unit->tag)->second + 140 <= game_loop)
	{
		Timer = true;
		adept_map.erase(unit->tag);
	}
}

void MEMIBot::StalkerBlinkEscape(const Unit* unit , const Unit* enemyarmy)
{
	Point2D BlinkLocation = unit->pos;
	Point2D KitingLocation = CalcKitingPosition(unit->pos, enemyarmy->pos);
	BlinkLocation += KitingLocation * 7.0f;

	Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_BLINK, BlinkLocation);
}

void MEMIBot::StalkerBlinkForward(const Unit* unit, const Unit* enemyarmy)
{
	Point2D BlinkLocation = unit->pos;
	Point2D KitingLocation = CalcKitingPosition(unit->pos, enemyarmy->pos);
	BlinkLocation -= KitingLocation * 7.0f;

	Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_BLINK, BlinkLocation);
}

void MEMIBot::Kiting(const Unit* unit, const Unit* enemyarmy)
{
	//Distance to target
	float dist = Distance2D(unit->pos, enemyarmy->pos);
	float DIST = dist - unit->radius - enemyarmy->radius;

	//Our range
	float unitattackrange = getAttackRangeGROUND(unit);

	const Unit& ENEMYARMY = *enemyarmy;

	if (unit->weapon_cooldown == 0.0f || DIST > unitattackrange + 0.5f)
	{
		Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemyarmy->pos);
	}
	else
	{
		sc2::Point2D KitingLocation = unit->pos;
		//If its a building we want range -1 distance
		//The same is true if it outranges us. We dont want to block following units
		if (IsStructure(Observation())(ENEMYARMY)) // 건물이면
		{
			KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos);
		}
		else if (getAttackRangeGROUND(enemyarmy) > unitattackrange) // 날 때릴 수 있는 적의 사정거리가 내 사정거리보다 길면
		{
			KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos);
			//KitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos); //TODO : 적을 데려나오기 위해 임시로 뒤로 빼게 했습니다.
		}
		else
		{
			KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos);
		}
		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
	}
}

void MEMIBot::KiteEnemy(const Unit* unit, Units enemy_army, Units enemy_units, Point2D KitingLocation, bool enemiesnear, const ObservationInterface* observation)
{
	if (enemy_units.empty()) // 적이 아예 없는 경우
	{
		ScoutWithUnit(unit, observation);
	}
	else if (enemy_army.empty()) //적 군대가 없는 경우
	{
		AttackWithUnit(unit, observation);
	}

	else if (!enemy_army.empty()) //적 군대가 있을 때
	{
		if (unit->weapon_cooldown == 0.0f || !enemiesnear) {
			Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_army.front()->pos);
		}
		else {
			Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
		}
	}
}

float MEMIBot::MinimumDistance2D(const Unit* unit ,const Unit* enemyarmy)
{
	float distance = std::numeric_limits<float>::max();

	float d = Distance2D(enemyarmy->pos, unit->pos);
	if (d < distance) {
		distance = d;
	}
	return distance;
}

Point2D MEMIBot::CalcKitingPosition(Point2D Mypos, Point2D EnemyPos) {
	Vector2D diff = Mypos - EnemyPos; // 7.3 적 유닛과의 반대 방향으로 도망
	Normalize2D(diff);
	return diff;
	//Point2D KitingLocation = Mypos + diff * 7.0f;
	//return KitingLocation;
}

bool MEMIBot::GetPosition(Units Enemyunits, Unit::Alliance alliace, Point2D& position) {

	const ObservationInterface* observation = Observation();
	if (Enemyunits.empty()) {
		return false;
	}
	position = Point2D(0.0f, 0.0f);
	unsigned int count = 0;

	for (const auto& u : Enemyunits) {
		position += u->pos;
		++count;
	}
	position /= (float)count;
	return true;
}

bool MEMIBot::GetPosition(UNIT_TYPEID unit_type, Unit::Alliance alliace, Point2D& position) {

	const ObservationInterface* observation = Observation();
	Units units = observation->GetUnits(alliace);
	if (units.empty()) {
		return false;
	}
	position = Point2D(0.0f, 0.0f);
	unsigned int count = 0;

	for (const auto& u : units) {
		if (u->unit_type == unit_type) {
			position += u->pos;
			++count;
		}
	}
	position /= (float)count;
	return true;
}

int MEMIBot::getAttackPriority(const Unit * u)
{
	if (u == nullptr) return 0;
	const Unit& unit = *u;
	if (IsArmy(Observation())(unit))
	{
		if (unit.unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_BANELING)
		{
			return 12;
		}
		if (unit.unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_LURKERMPBURROWED)
		{
			return 11;
		}
		return 10;
	}
	if (unit.unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_PHOTONCANNON || unit.unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_SPINECRAWLER)
	{
		return 10;
	}
	if (IsWorker()(unit))
	{
		return 31;
	}
	if (unit.unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_PYLON || unit.unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_SPORECRAWLER || unit.unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_MISSILETURRET)
	{
		return 5;
	}
	if (IsTownHall()(unit))
	{
		return 4;
	}
	return 1;
}

 const Unit * MEMIBot::GetTarget(const Unit * rangedUnit, Units & targets)
{
	int highPriorityFar = 0;
	int highPriorityNear = 0;
	double closestDist = std::numeric_limits<double>::max();
	double lowestHealth = std::numeric_limits<double>::max();
	const Unit * closestTargetOutsideRange = nullptr;
	const Unit * weakestTargetInsideRange = nullptr;

	for (const auto & targetUnit : targets)
	{
		if (!targetUnit->is_alive)
		{
			continue;
		}
		const float range = getAttackRangeGROUND(rangedUnit); //rangedUnit->getAttackRange(targetUnit); 사도 사거리 4
		int priority = getAttackPriority(targetUnit);
		const float distance = Distance2D(rangedUnit->pos, targetUnit->pos);

		if (distance > range) //적과 나 사이의 거리 > 나의 사정거리 // 거리가 멀어서 때릴 수 없는 경우
		{
			// If in sight we just add 20 to prio. This should make sure that a unit in sight has higher priority than any unit outside of range
			//
			float SightRange = Observation()->GetUnitTypeData()[rangedUnit->unit_type].sight_range;

			if (distance <= SightRange)
			{
				priority += 20;
			}
			// 우선순위가 높거나 또는 거리가 가까우면 설정한다
			if (!closestTargetOutsideRange || (priority > highPriorityFar) || (priority == highPriorityFar && distance < closestDist))
			{
				if (getDpsGROUND(targetUnit)) // 위협되는 적인가?
				{
					closestDist = distance;
					highPriorityFar = priority;
					closestTargetOutsideRange = targetUnit;
				}
			}
		}
		else
		{
			if (!weakestTargetInsideRange || (priority > highPriorityNear) || (priority == highPriorityNear && targetUnit->health < lowestHealth))
			{
				if (getDpsGROUND(targetUnit)) // 위협되는 적인가?
				{
					lowestHealth = targetUnit->health;
					highPriorityNear = priority;
					weakestTargetInsideRange = targetUnit;
				}
			}
		}
	}
	return weakestTargetInsideRange && highPriorityNear>1 ? weakestTargetInsideRange : closestTargetOutsideRange;
}

const float MEMIBot::getDpsGROUND(const Unit* target) const
{

	const ObservationInterface* observation = Observation();

	float dps = 0.0f;

	for (const auto & Weapon : Observation()->GetUnitTypeData()[target->unit_type].weapons)
	{
		if (Weapon.type == Weapon::TargetType::Air)
			continue;

		dps = Weapon.attacks * Weapon.damage_ / Weapon.speed;
	}
	return dps;
}

const float MEMIBot::getAttackRangeGROUND(const Unit* target) const
{

	const ObservationInterface* observation = Observation();
	sc2::Weapon groundWeapons;
	sc2::Weapon AirWeapons;

	for (const auto & Weapon : Observation()->GetUnitTypeData()[target->unit_type].weapons)
	{
		if (Weapon.type == sc2::Weapon::TargetType::Air || Weapon.type == sc2::Weapon::TargetType::Any)
		{
			AirWeapons = Weapon;
		}
		if (Weapon.type == sc2::Weapon::TargetType::Ground || Weapon.type == sc2::Weapon::TargetType::Any)//Siege tanks
		{
			groundWeapons = Weapon;
			if (groundWeapons.range < 0.11f)//melee. Not exactly 0.1
			{
				groundWeapons.range += target->radius;
			}
		}
	}
	//return AirWeapons.range; // 7.5 이건 오로지 공중유닛을 위한 함수!!
	return groundWeapons.range; // 7.5 지상유닛을 위한 함수
}

