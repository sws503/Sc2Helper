#include "memibot.h"



Point2D MEMIBot::CalcKitingPosition(Point2D Mypos, Point2D EnemyPos) {
	Vector2D diff = Mypos - EnemyPos; // 7.3 적 유닛과의 반대 방향으로 도망
	Normalize2D(diff);
	return diff;
	//Point2D KitingLocation = Mypos + diff * 7.0f;
	//return KitingLocation;
}


void MEMIBot::FleeKiting(const Unit* unit, const Unit* enemyarmy)
{
	const ObservationInterface* observation = Observation();
	Units NearbyArmies = FindUnitsNear(unit, 10, Unit::Alliance::Enemy, IsArmy(Observation()));
	Point2D enemyposition;
	GetPosition(NearbyArmies, Unit::Alliance::Enemy, enemyposition);

	//Distance to target
	float dist = Distance2D(unit->pos, enemyposition);
	float DIST = dist - unit->radius - enemyarmy->radius;

	const Unit& ENEMYARMY = *enemyarmy;

	if (DIST < getAttackRangeGROUND(enemyarmy) + 1.0f || DIST < 5.0f) // 거리가 멀어서 추격해야 하는 상황
	{
		sc2::Point2D KitingLocation = unit->pos;
		KitingLocation += CalcKitingPosition(unit->pos, enemyposition) * 3.0f;
		SmartMove(unit, KitingLocation);
	}
}

void MEMIBot::DistanceKiting(const Unit* unit, const Unit* enemyarmy, const Unit* army) // 일꾼 수비병력이 있는 경우
{
	//Distance to target
	float dist = Distance2D(unit->pos, army->pos);
	float DIST = dist - unit->radius - army->radius;

	//Our range
	float unitattackrange = getAttackRangeGROUND(unit);
	float enemyattackrange = getAttackRangeGROUND(army);

	/*if (!unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::ATTACK && unit->weapon_cooldown == 0.0f) // 현재 공격이 선딜상황임
	{
	//가만히 있도록 합시다
	}
	else if (unit->weapon_cooldown == 0.0f)
	{
	SmartAttackUnit(unit, enemyarmy);
	}
	else // 거리가 가까워서 도망가야 하는 상황
	{
	sc2::Point2D KitingLocation = unit->pos;
	KitingLocation += CalcKitingPosition(unit->pos, army->pos) * 6.0f;

	SmartMoveEfficient(unit, KitingLocation, army);
	}*/

	if (!unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::ATTACK && unit->weapon_cooldown == 0.0f) // 현재 공격이 선딜상황임
	{
		//가만히 있도록 합시다
	}
	else if (unit->weapon_cooldown == 0.0f)
	{
		SmartAttackUnit(unit, enemyarmy);
	}
	else // 거리가 가까워서 도망가야 하는 상황
	{
		sc2::Point2D KitingLocation = unit->pos;
		KitingLocation += CalcKitingPosition(unit->pos, army->pos) * 6.0f;

		SmartMoveEfficient(unit, KitingLocation, army);
	}
}

void MEMIBot::FrontKiting(const Unit* unit, const Unit* enemyarmy) // 일꾼 공격하기 위해서
{
	//Distance to target
	float dist = Distance2D(unit->pos, enemyarmy->pos);
	float DIST = dist - unit->radius - enemyarmy->radius;

	//Our range
	float unitattackrange = getAttackRangeGROUND(unit);

	const Unit& ENEMYARMY = *enemyarmy;

	if (!unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::ATTACK && unit->weapon_cooldown == 0.0f) // 현재 공격이 선딜상황임
	{
		//가만히 있도록 합시다
	}
	else if (unit->weapon_cooldown == 0.0f)
	{
		SmartAttackUnit(unit, enemyarmy);
	}
	else if (DIST > 3.0f) // 거리가 멀어서 추격해야 하는 상황
	{
		sc2::Point2D KitingLocation = unit->pos;
		//If its a building we want range -1 distance
		//The same is true if it outranges us. We dont want to block following units
		KitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos);

		/*if (!Observation()->IsPathable(KitingLocation)) // 이동할 위치가 지상유닛이 갈 수 없는 곳이라면
		{
		EmergencyKiting(unit, enemyarmy);
		}
		else if (float pathingdistance = Query()->PathingDistance(unit, KitingLocation) > 10) // 이동할 위치가 멀리 돌아가야하는 곳이라면
		{
		EmergencyKiting(unit, enemyarmy);
		}
		else if (pathingdistance == 0)
		{
		EmergencyKiting(unit, enemyarmy);
		}
		else
		{
		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
		}*/
		SmartMove(unit, KitingLocation);
	}
	else // 거리가 가까워서 도망가야 하는 상황
	{
		sc2::Point2D KitingLocation = unit->pos;
		//If its a building we want range -1 distance
		//The same is true if it outranges us. We dont want to block following units
		KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos);

		/*if (!Observation()->IsPathable(KitingLocation)) // 이동할 위치가 지상유닛이 갈 수 없는 곳이라면
		{
		EmergencyKiting(unit, enemyarmy);
		}
		else if (float pathingdistance = Query()->PathingDistance(unit, KitingLocation) > 10) // 이동할 위치가 멀리 돌아가야하는 곳이라면
		{
		EmergencyKiting(unit, enemyarmy);
		}
		else if (pathingdistance == 0)
		{
		EmergencyKiting(unit, enemyarmy);
		}
		else
		{
		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
		}*/
		SmartMove(unit, KitingLocation);
	}
}

bool MEMIBot::CanHitMe(const Unit* unit)
{
	Units NearbyArmies = FindUnitsNear(unit, 10, Unit::Alliance::Enemy, IsArmy(Observation()));

	bool hitme = false;
	for (const auto & target : NearbyArmies)
	{
		float dist = Distance2D(unit->pos, target->pos);
		float DIST = dist - unit->radius - target->radius;

		float targetattackrange = getAttackRangeGROUND(target);

		if (DIST < targetattackrange + 1.0f)
		{
			hitme = true;
		}
	}
	return hitme;
}

void MEMIBot::ComeOnKiting(const Unit* unit, const Unit* enemyarmy)
{
	//std::cout << ShadeNearEnemies.size() << ">=" << NearbyEnemies.size() << "=" << (ShadeNearEnemies.size() >= NearbyEnemies.size()) << std::endl;

	//Distance to target
	float dist = Distance2D(unit->pos, enemyarmy->pos);
	float DIST = dist - unit->radius - enemyarmy->radius;

	//Our range
	float unitattackrange = getAttackRangeGROUND(unit);

	const Unit& ENEMYARMY = *enemyarmy;

	if (CanHitMe(unit) == false) //적 공격사거리가 나보다 짧으면 공격한다
	{
		SmartAttackUnit(unit, enemyarmy);
	}
	else
	{
		sc2::Point2D KitingLocation = unit->pos;
		KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos); // 적을 데려나오기 위해 뒤로 빼게 했습니다.

		SmartMoveEfficient(unit, KitingLocation, enemyarmy);

		/*if (!Observation()->IsPathable(KitingLocation)) // 이동할 위치가 지상유닛이 갈 수 없는 곳이라면
		{
		EmergencyKiting(unit, enemyarmy);
		}
		else if (float pathingdistance = Query()->PathingDistance(unit, KitingLocation) > 10) // 이동할 위치가 멀리 돌아가야하는 곳이라면
		{
		EmergencyKiting(unit, enemyarmy);
		}
		else if (pathingdistance == 0)
		{
		EmergencyKiting(unit, enemyarmy);
		}
		else
		{
		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
		}*/
	}
}

void MEMIBot::ColossusKiting(const Unit* unit, const Unit* enemyarmy)
{
	//Distance to target
	float dist = Distance2D(unit->pos, enemyarmy->pos);
	float DIST = dist - unit->radius - enemyarmy->radius;

	//Our range
	float unitattackrange = getAttackRangeGROUND(unit);

	const Unit& ENEMYARMY = *enemyarmy;

	//Units my_army = Observation()->GetUnits(Unit::Alliance::Self, IsNearbyArmies(Observation(), unit->pos, 10));
	//Units enemy_army = Observation()->GetUnits(Unit::Alliance::Self, IsNearbyArmies(Observation(), enemyarmy->pos, 10));

	//float myDps = getunitsDpsGROUND(my_army);
	//float enemyDps = getunitsDpsGROUND(enemy_army);


	//현재 공격중인데 WC가 0이면 냅둔다 or 현재 공격가능하면 공격하고 or 적이 멀리 떨어지면

	float Wait = 21.0f;

	if (DIST < unitattackrange && !unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::ATTACK && unit->weapon_cooldown == 0.0f) // 현재 공격이 선딜상황임
	{
		//가만히 있도록 합시다
	}
	else if (DIST < unitattackrange && !unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::ATTACK && unit->weapon_cooldown >= Wait) // 후딜을 기다려
	{
	}
	else if (unitattackrange - 0.3f < DIST && DIST <= unitattackrange + 2.0f) // 최대사거리를 0.3 낮춤
	{
		SmartMove(unit, enemyarmy->pos);
	}
	else if (unit->weapon_cooldown == 0.0f || DIST > unitattackrange + 2.0f)
	{
		std::cout << unitattackrange;
		SmartAttackUnit(unit, enemyarmy);
	}
	else
	{
		sc2::Point2D KitingLocation = unit->pos;
		KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos) * 10.0f;
		sc2::Point2D FrontKitingLocation = unit->pos;
		FrontKitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos) * 4.0f;

		if (IsStructure(Observation())(ENEMYARMY)) // 건물이면
		{
			SmartMove(unit, FrontKitingLocation);
		}
		else if (getAttackRangeGROUND(enemyarmy) > unitattackrange) // 날 때릴 수 있는 적의 사정거리가 내 사정거리보다 길면
		{
			SmartMove(unit, FrontKitingLocation);
		}
		else // 적 사정거리가 나랑 같거나 짧으면
		{
			SmartMoveEfficient(unit, KitingLocation, enemyarmy);
		}
	}
}

void MEMIBot::Kiting(const Unit* unit, const Unit* enemyarmy)
{
	float dist = Distance2D(unit->pos, enemyarmy->pos);
	float DIST = dist - unit->radius - enemyarmy->radius;

	//Our range
	float unitattackrange = getAttackRangeGROUND(unit);

	const Unit& ENEMYARMY = *enemyarmy;

	if (DIST < unitattackrange && !unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::ATTACK && unit->weapon_cooldown == 0.0f) // 현재 공격이 선딜상황임
	{
		//가만히 있도록 합시다
	}
	else if (unit->weapon_cooldown == 0.0f || DIST > unitattackrange + 2.0f)
	{
		SmartAttackUnit(unit, enemyarmy);
	}
	else if (!unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::MOVE) // 움직이고 있는 상황일 때도
	{

	}
	else
	{
		sc2::Point2D KitingLocation = unit->pos;
		KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos) * 10.0f;
		sc2::Point2D FrontKitingLocation = unit->pos;
		FrontKitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos) * 4.0f;

		if (IsStructure(Observation())(ENEMYARMY)) // 건물이면
		{
			SmartMove(unit, FrontKitingLocation);
		}
		else if (getAttackRangeGROUND(enemyarmy) > unitattackrange) // 날 때릴 수 있는 적의 사정거리가 내 사정거리보다 길면
		{
			SmartMove(unit, FrontKitingLocation);
		}
		//else if (getAttackRangeGROUND(enemyarmy) == unitattackrange && (myDps > (enemyDps + 50)) ) // 압도적이면 앞으로 가서 싸워라 TODO : enemy_army.size() * 5 로 가중치를 둘까요??
		//{
		//	KitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos) * 4.0f;
		//}
		else // 적 사정거리가 나랑 같거나 짧으면
		{
			SmartMoveEfficient(unit, KitingLocation, enemyarmy);
		}
	}
}

void MEMIBot::SmartMoveEfficient(const Unit* unit, Point2D KitingLocation, const Unit * enemyarmy)
{
	float pathingdistance;

	if ((pathingdistance = Query()->PathingDistance(unit, KitingLocation)) > 30) // 이동할 위치가 멀리 돌아가야하는 곳이라면
	{
		EmergencyKiting(unit, enemyarmy);
	}
	else if (pathingdistance == 0)
	{
		EmergencyKiting(unit, enemyarmy);
	}
	else
	{
		SmartMove(unit, KitingLocation);
	}
}

void MEMIBot::EmergencyKiting(const Unit* unit, const Unit * enemyarmy)
{
	const ObservationInterface* observation = Observation();
	// 적의 평균 위치를 받아옵니다
	Units NearbyArmies = FindUnitsNear(unit, 10, Unit::Alliance::Enemy, IsArmy(Observation()));
	Point2D enemyposition;
	GetPosition(NearbyArmies, Unit::Alliance::Enemy, enemyposition);

	Point2D KitingLocation1 = unit->pos;
	Point2D KitingLocation2 = unit->pos;

	Point2D Vector = CalcKitingPosition(unit->pos, enemyarmy->pos);

	KitingLocation1 += Point2D(-Vector.y, +Vector.x) * 5;
	KitingLocation2 += Point2D(+Vector.y, -Vector.x) * 5;

	Point2D RealKitingLocation;

	if (Distance2D(KitingLocation1, enemyposition) < Distance2D(KitingLocation2, enemyposition)) //1로 가는게 2로 가는 것보다 안전한 경우
	{
		RealKitingLocation = KitingLocation1;
	}
	else
	{
		RealKitingLocation = KitingLocation2;
	}
	SmartMove(unit, RealKitingLocation);
}
