#include "memibot.h"



Point2D MEMIBot::CalcKitingPosition(Point2D Mypos, Point2D EnemyPos) {
	Vector2D diff = Mypos - EnemyPos; // 7.3 �� ���ְ��� �ݴ� �������� ����
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

	if (DIST < getAttackRangeGROUND(enemyarmy) + 1.0f || DIST < 5.0f) // �Ÿ��� �־ �߰��ؾ� �ϴ� ��Ȳ
	{
		sc2::Point2D KitingLocation = unit->pos;
		KitingLocation += CalcKitingPosition(unit->pos, enemyposition) * 3.0f;
		SmartMove(unit, KitingLocation);
	}
}

void MEMIBot::DistanceKiting(const Unit* unit, const Unit* enemyarmy, const Unit* army) // �ϲ� ���񺴷��� �ִ� ���
{
	//Distance to target
	float dist = Distance2D(unit->pos, army->pos);
	float DIST = dist - unit->radius - army->radius;

	//Our range
	float unitattackrange = getAttackRangeGROUND(unit);
	float enemyattackrange = getAttackRangeGROUND(army);

	/*if (!unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::ATTACK && unit->weapon_cooldown == 0.0f) // ���� ������ ������Ȳ��
	{
	//������ �ֵ��� �սô�
	}
	else if (unit->weapon_cooldown == 0.0f)
	{
	SmartAttackUnit(unit, enemyarmy);
	}
	else // �Ÿ��� ������� �������� �ϴ� ��Ȳ
	{
	sc2::Point2D KitingLocation = unit->pos;
	KitingLocation += CalcKitingPosition(unit->pos, army->pos) * 6.0f;

	SmartMoveEfficient(unit, KitingLocation, army);
	}*/

	if (!unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::ATTACK && unit->weapon_cooldown == 0.0f) // ���� ������ ������Ȳ��
	{
		//������ �ֵ��� �սô�
	}
	else if (unit->weapon_cooldown == 0.0f)
	{
		SmartAttackUnit(unit, enemyarmy);
	}
	else // �Ÿ��� ������� �������� �ϴ� ��Ȳ
	{
		sc2::Point2D KitingLocation = unit->pos;
		KitingLocation += CalcKitingPosition(unit->pos, army->pos) * 6.0f;

		SmartMoveEfficient(unit, KitingLocation, army);
	}
}

void MEMIBot::FrontKiting(const Unit* unit, const Unit* enemyarmy) // �ϲ� �����ϱ� ���ؼ�
{
	//Distance to target
	float dist = Distance2D(unit->pos, enemyarmy->pos);
	float DIST = dist - unit->radius - enemyarmy->radius;

	//Our range
	float unitattackrange = getAttackRangeGROUND(unit);

	const Unit& ENEMYARMY = *enemyarmy;

	if (!unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::ATTACK && unit->weapon_cooldown == 0.0f) // ���� ������ ������Ȳ��
	{
		//������ �ֵ��� �սô�
	}
	else if (unit->weapon_cooldown == 0.0f)
	{
		SmartAttackUnit(unit, enemyarmy);
	}
	else if (DIST > 3.0f) // �Ÿ��� �־ �߰��ؾ� �ϴ� ��Ȳ
	{
		sc2::Point2D KitingLocation = unit->pos;
		//If its a building we want range -1 distance
		//The same is true if it outranges us. We dont want to block following units
		KitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos);

		/*if (!Observation()->IsPathable(KitingLocation)) // �̵��� ��ġ�� ���������� �� �� ���� ���̶��
		{
		EmergencyKiting(unit, enemyarmy);
		}
		else if (float pathingdistance = Query()->PathingDistance(unit, KitingLocation) > 10) // �̵��� ��ġ�� �ָ� ���ư����ϴ� ���̶��
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
	else // �Ÿ��� ������� �������� �ϴ� ��Ȳ
	{
		sc2::Point2D KitingLocation = unit->pos;
		//If its a building we want range -1 distance
		//The same is true if it outranges us. We dont want to block following units
		KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos);

		/*if (!Observation()->IsPathable(KitingLocation)) // �̵��� ��ġ�� ���������� �� �� ���� ���̶��
		{
		EmergencyKiting(unit, enemyarmy);
		}
		else if (float pathingdistance = Query()->PathingDistance(unit, KitingLocation) > 10) // �̵��� ��ġ�� �ָ� ���ư����ϴ� ���̶��
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

	if (CanHitMe(unit) == false) //�� ���ݻ�Ÿ��� ������ ª���� �����Ѵ�
	{
		SmartAttackUnit(unit, enemyarmy);
	}
	else
	{
		sc2::Point2D KitingLocation = unit->pos;
		KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos); // ���� ���������� ���� �ڷ� ���� �߽��ϴ�.

		SmartMoveEfficient(unit, KitingLocation, enemyarmy);

		/*if (!Observation()->IsPathable(KitingLocation)) // �̵��� ��ġ�� ���������� �� �� ���� ���̶��
		{
		EmergencyKiting(unit, enemyarmy);
		}
		else if (float pathingdistance = Query()->PathingDistance(unit, KitingLocation) > 10) // �̵��� ��ġ�� �ָ� ���ư����ϴ� ���̶��
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


	//���� �������ε� WC�� 0�̸� ���д� or ���� ���ݰ����ϸ� �����ϰ� or ���� �ָ� ��������

	float Wait = 21.0f;

	if (DIST < unitattackrange && !unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::ATTACK && unit->weapon_cooldown == 0.0f) // ���� ������ ������Ȳ��
	{
		//������ �ֵ��� �սô�
	}
	else if (DIST < unitattackrange && !unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::ATTACK && unit->weapon_cooldown >= Wait) // �ĵ��� ��ٷ�
	{
	}
	else if (unitattackrange - 0.3f < DIST && DIST <= unitattackrange + 2.0f) // �ִ��Ÿ��� 0.3 ����
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

		if (IsStructure(Observation())(ENEMYARMY)) // �ǹ��̸�
		{
			SmartMove(unit, FrontKitingLocation);
		}
		else if (getAttackRangeGROUND(enemyarmy) > unitattackrange) // �� ���� �� �ִ� ���� �����Ÿ��� �� �����Ÿ����� ���
		{
			SmartMove(unit, FrontKitingLocation);
		}
		else // �� �����Ÿ��� ���� ���ų� ª����
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

	if (DIST < unitattackrange && !unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::ATTACK && unit->weapon_cooldown == 0.0f) // ���� ������ ������Ȳ��
	{
		//������ �ֵ��� �սô�
	}
	else if (unit->weapon_cooldown == 0.0f || DIST > unitattackrange + 2.0f)
	{
		SmartAttackUnit(unit, enemyarmy);
	}
	else if (!unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::MOVE) // �����̰� �ִ� ��Ȳ�� ����
	{

	}
	else
	{
		sc2::Point2D KitingLocation = unit->pos;
		KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos) * 10.0f;
		sc2::Point2D FrontKitingLocation = unit->pos;
		FrontKitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos) * 4.0f;

		if (IsStructure(Observation())(ENEMYARMY)) // �ǹ��̸�
		{
			SmartMove(unit, FrontKitingLocation);
		}
		else if (getAttackRangeGROUND(enemyarmy) > unitattackrange) // �� ���� �� �ִ� ���� �����Ÿ��� �� �����Ÿ����� ���
		{
			SmartMove(unit, FrontKitingLocation);
		}
		//else if (getAttackRangeGROUND(enemyarmy) == unitattackrange && (myDps > (enemyDps + 50)) ) // �е����̸� ������ ���� �ο��� TODO : enemy_army.size() * 5 �� ����ġ�� �ѱ��??
		//{
		//	KitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos) * 4.0f;
		//}
		else // �� �����Ÿ��� ���� ���ų� ª����
		{
			SmartMoveEfficient(unit, KitingLocation, enemyarmy);
		}
	}
}

void MEMIBot::SmartMoveEfficient(const Unit* unit, Point2D KitingLocation, const Unit * enemyarmy)
{
	float pathingdistance;

	if ((pathingdistance = Query()->PathingDistance(unit, KitingLocation)) > 30) // �̵��� ��ġ�� �ָ� ���ư����ϴ� ���̶��
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
	// ���� ��� ��ġ�� �޾ƿɴϴ�
	Units NearbyArmies = FindUnitsNear(unit, 10, Unit::Alliance::Enemy, IsArmy(Observation()));
	Point2D enemyposition;
	GetPosition(NearbyArmies, Unit::Alliance::Enemy, enemyposition);

	Point2D KitingLocation1 = unit->pos;
	Point2D KitingLocation2 = unit->pos;

	Point2D Vector = CalcKitingPosition(unit->pos, enemyarmy->pos);

	KitingLocation1 += Point2D(-Vector.y, +Vector.x) * 5;
	KitingLocation2 += Point2D(+Vector.y, -Vector.x) * 5;

	Point2D RealKitingLocation;

	if (Distance2D(KitingLocation1, enemyposition) < Distance2D(KitingLocation2, enemyposition)) //1�� ���°� 2�� ���� �ͺ��� ������ ���
	{
		RealKitingLocation = KitingLocation1;
	}
	else
	{
		RealKitingLocation = KitingLocation2;
	}
	SmartMove(unit, RealKitingLocation);
}
