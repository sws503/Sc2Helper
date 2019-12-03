#include "memibot.h"



Point2D MEMIBot::CalcKitingPosition(Point2D Mypos, Point2D EnemyPos) {
	Vector2D diff = Mypos - EnemyPos; // 7.3 �� ���ְ��� �ݴ� �������� ����
	Normalize2D(diff);
	return diff;
	//Point2D KitingLocation = Mypos + diff * 7.0f;
	//return KitingLocation;
}

void MEMIBot::PredictKiting(const Unit* unit, const Unit* enemyarmy)
{
	if (unit == nullptr) return;
	Units NearbyArmies = FindUnitsNear(unit, 30, Unit::Alliance::Enemy, IsArmy(Observation()));
	Units NearMyArmies = FindUnitsNear(unit, 15, Unit::Alliance::Self, IsArmy(Observation()));
	Units NearEnemyWorkers = FindUnitsNear(unit, 15, Unit::Alliance::Enemy, IsWorker());
	size_t nearenemyworkers_size = NearEnemyWorkers.size();
	size_t nearenemyarmies_size = NearbyArmies.size();

	const Unit* nearestenemy = FindNearestUnit(unit->pos, Unit::Alliance::Enemy, IsArmy(Observation()), 6.0f);

	int stalkers = 0;
	int immortals = 0;
	int marines = 0;
	int marauders = 0;
	int siegetanks = 0;
	int medivacs = 0;
	int vikings = 0;
	int cyclones = 0;
	int battlecruisers = 0;
	int enemysum = 0;

	for (const Unit* u : NearMyArmies) {
		stalkers += IsUnit(UNIT_TYPEID::PROTOSS_STALKER)(*u);
		immortals += IsUnit(UNIT_TYPEID::PROTOSS_IMMORTAL)(*u);
	}
	for (const Unit* u : NearbyArmies) {
		marines += IsUnits({ UNIT_TYPEID::TERRAN_MARINE })(*u);
		marauders += IsUnit(UNIT_TYPEID::TERRAN_MARAUDER)(*u);
		siegetanks += IsUnits({ UNIT_TYPEID::TERRAN_SIEGETANK, UNIT_TYPEID::TERRAN_SIEGETANKSIEGED })(*u);
		medivacs += IsUnit(UNIT_TYPEID::TERRAN_MEDIVAC)(*u);
		vikings += IsUnits({ UNIT_TYPEID::TERRAN_VIKINGASSAULT, UNIT_TYPEID::TERRAN_VIKINGFIGHTER })(*u);
		cyclones += IsUnit(UNIT_TYPEID::TERRAN_CYCLONE)(*u);
		battlecruisers += IsUnit(UNIT_TYPEID::TERRAN_BATTLECRUISER)(*u);
	}
	marines += (int)nearenemyworkers_size;
	enemysum = marines + marauders + siegetanks + medivacs + vikings + cyclones + battlecruisers;

	// 0�� �������� �̱�Ͱ���
	float winrate = PredictWinrate(stalkers, immortals, marines, marauders, siegetanks, medivacs, vikings, cyclones, battlecruisers);

	RealChat("current winrate is" + std::to_string(100 - 100 * winrate));
	//RealChat(std::to_string(100 - 100 * winrate));

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
		FrontKitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos) * 8.0f;

		if (IsStructure(Observation())(ENEMYARMY)) // �ǹ��̸�
		{
			SmartMove(unit, FrontKitingLocation);
		}
		else if (getAttackRangeGROUND(enemyarmy) > unitattackrange) // �� ���� �� �ִ� ���� �����Ÿ��� �� �����Ÿ����� ���
		{
			SmartMove(unit, FrontKitingLocation);
		}
		// �е����̸� ������ ���� �ο���
		else if (branch == 5 && nearenemyarmies_size + nearenemyworkers_size - enemysum == 0 && winrate < 0.3f) 
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
		FrontKitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos) * 8.0f;

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

void MEMIBot::EvadeKiting(const Unit* unit, const Unit* enemyarmy)
{
	const ObservationInterface* observation = Observation();
	Units NearbyArmies = FindUnitsNear(unit, 10, Unit::Alliance::Enemy, IsArmy(Observation()));
	Point2D enemyposition;
	GetPosition(NearbyArmies, Unit::Alliance::Enemy, enemyposition);

	//Distance to target
	float dist = Distance2D(unit->pos, enemyposition);
	float DIST = dist - unit->radius - enemyarmy->radius;

	const Unit& ENEMYARMY = *enemyarmy;

	sc2::Point2D KitingLocation = unit->pos;
	KitingLocation += CalcKitingPosition(unit->pos, enemyposition) * 3.0f;
	SmartMove(unit, KitingLocation);
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

void MEMIBot::OracleBackKiting(const Unit* unit, const Unit* Workertarget, const Unit* Armytarget) // �ϲ� ���񺴷��� �ִ� ���
{
	float unitHP = unit->health + unit->shield; //ü�� ����ؼ� ����?

	float dist = Distance2D(unit->pos, Armytarget->pos);
	float DIST = dist - unit->radius - Armytarget->radius; //������ �Ÿ�

	float Wdist = Distance2D(unit->pos, Workertarget->pos);
	float WDIST = dist - unit->radius - Workertarget->radius; // �ϲ۰��� �Ÿ�

	float enemyattackrange = getAttackRangeAIR(Armytarget); //���� ��Ÿ�
	float myattackrange = getAttackRangeGROUND(unit); // ���� ��Ÿ�

	if (Armytarget->unit_type == UNIT_TYPEID::PROTOSS_PHOTONCANNON || Armytarget->unit_type == UNIT_TYPEID::TERRAN_MISSILETURRET || Armytarget->unit_type == UNIT_TYPEID::ZERG_SPORECRAWLER || Armytarget->unit_type == UNIT_TYPEID::TERRAN_BUNKER)
	{
		if (WDIST < myattackrange && !unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::ATTACK && unit->weapon_cooldown == 0.0f) // ���� ������ ������Ȳ��
		{
			//������ �ֵ��� �սô�
		}
		if (DIST <= enemyattackrange + 0.5f)
		{
			sc2::Point2D KitingLocation = unit->pos;
			KitingLocation += CalcKitingPosition(unit->pos, Armytarget->pos) * 6.0f;
			SmartMove(unit, KitingLocation);
		}
		else if (unit->weapon_cooldown == 0.0f && WDIST <= 4)
		{
			SmartAttackUnit(unit, Workertarget);
		}
		else
		{
			Point2D KitingLocation1 = unit->pos;
			Point2D Vector = CalcKitingPosition(unit->pos, Armytarget->pos);
			KitingLocation1 += Point2D(-Vector.y, +Vector.x) * 5;

			SmartMove(unit, KitingLocation1);
		}
	}
	else
	{
		if (WDIST < myattackrange && !unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::ATTACK && unit->weapon_cooldown == 0.0f) // ���� ������ ������Ȳ��
		{
			//������ �ֵ��� �սô�
		}
		else if (DIST <= enemyattackrange * 1.2 + 1.0f) // �ʹ� ����� 1.25? 1.35?
		{
			sc2::Point2D KitingLocation = unit->pos;
			KitingLocation += CalcKitingPosition(unit->pos, Armytarget->pos) * 6.0f;
			SmartMove(unit, KitingLocation);
		}
		else if (unit->weapon_cooldown == 0.0f && WDIST <= 5)
		{
			SmartAttackUnit(unit, Workertarget);
		}
		else
		{
			Point2D KitingLocation1 = unit->pos;
			Point2D Vector = CalcKitingPosition(unit->pos, Armytarget->pos);
			KitingLocation1 += Point2D(-Vector.y, +Vector.x) * 5;

			SmartMove(unit, KitingLocation1);
		}
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
		KitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos) * 3.0f;

		SmartMove(unit, KitingLocation);
	}
	else // �Ÿ��� ������� �������� �ϴ� ��Ȳ
	{
		sc2::Point2D KitingLocation = unit->pos;
		//If its a building we want range -1 distance
		//The same is true if it outranges us. We dont want to block following units
		KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos) * 3.0f;

		SmartMove(unit, KitingLocation);
	}
}

bool MEMIBot::CanHitMeGROUND(const Unit* unit)
{
	Units NearbyArmies = FindUnitsNear(unit, 15, Unit::Alliance::Enemy, IsArmy(Observation()));

	bool hitme = false;
	for (const auto & target : NearbyArmies)
	{
		float dist = Distance2D(unit->pos, target->pos);
		float DIST = dist - unit->radius - target->radius;

		float targetattackrange = getAttackRangeGROUND(target);

		if (DIST <= targetattackrange * 1.35 + 0.5f)
		{
			hitme = true;
		}
	}
	return hitme;
}


bool MEMIBot::CanHitMe(const Unit* unit, float distance)
{
	Units NearbyArmies = FindUnitsNear(unit, 20, Unit::Alliance::Enemy, AirAttacker());

	bool hitme = false;
	for (const auto & target : NearbyArmies)
	{
		float dist = Distance2D(unit->pos, target->pos);
		float DIST = dist - unit->radius - target->radius;

		float targetattackrange = getAttackRangeAIR(target);

		if (target->unit_type == UNIT_TYPEID::PROTOSS_PHOTONCANNON || target->unit_type == UNIT_TYPEID::TERRAN_MISSILETURRET || target->unit_type == UNIT_TYPEID::ZERG_SPORECRAWLER || target->unit_type == UNIT_TYPEID::TERRAN_BUNKER)
		{
			if (DIST <= targetattackrange + 1.2f)
			{
				hitme = true;
			}
		}
		else
		{
			if (DIST <= targetattackrange * 1.5 + distance)
			{
				hitme = true;
			}
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

	if (CanHitMeGROUND(unit) == false) //�� ���ݻ�Ÿ��� ������ ª���� �����Ѵ�
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

void MEMIBot::CarrierKiting(const Unit* unit, const Unit* enemyarmy)
{

	float dist = Distance2D(unit->pos, enemyarmy->pos);
	float DIST = dist - unit->radius - enemyarmy->radius;

	float unitattackrange = getAttackRangeAIR(unit);
	float enemyattackrange = getAttackRangeAIR(enemyarmy);
	const Unit& ENEMYARMY = *enemyarmy;

	bool enemiesnear = false;

	if (unit->shield < 1)
	{ 
		if (DIST <= 9.5) 
		{
			enemiesnear = true; 
		} 
	}
	else 
	{ 
		if (DIST <= 7.5)
		{
			enemiesnear = true; 
		} 
	}

	if (enemyattackrange < 1)
	{
		if (unit->weapon_cooldown == 0.0f)
		{
			SmartAttackUnit(unit, enemyarmy);
		}
		else
		{
			sc2::Point2D FrontKitingLocation = unit->pos;
			FrontKitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos) * 7.0f;

			SmartMove(unit, FrontKitingLocation);
		}
	}
	else
	{
		if (!enemiesnear || unit->weapon_cooldown == 0.0f)
		{
			SmartAttackUnit(unit, enemyarmy);
		}
		else if (IsStructure(Observation())(ENEMYARMY))
		{
			sc2::Point2D FrontKitingLocation = unit->pos;
			FrontKitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos) * 7.0f;

			SmartMove(unit, FrontKitingLocation);
		}
		else
		{
			sc2::Point2D KitingLocation = unit->pos;
			KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos) * 7.0f;

			SmartMove(unit, KitingLocation);
		}
	}
}

bool MEMIBot::ChargeShield(const Unit* unit)
{
	const Unit * NearestBattery = FindNearestUnit(unit->pos, [](const Unit& unit) {return (IsUnit(UNIT_TYPEID::PROTOSS_SHIELDBATTERY)(unit) && unit.energy > 10 && unit.is_powered == 1 && unit.alliance == 1); });

	bool need = false;
	bool value;

	
	if (unit->shield < 5 && NearestBattery != nullptr)
	{
		Actions()->UnitCommand(unit, 3707);
		SmartMove(unit, NearestBattery->pos);
		value = true;
		need = true;
	}
	else if (need && unit->shield >= unit->shield_max - 15)
	{
		value = false;
		SmartMove(unit, NearestBattery->pos);
		//value = (unit->shield < unit->shield_max);
	}
	else
	{
		value = false;
	}
	
	return value;
}


void MEMIBot::VoidRayKiting(const Unit* unit, const Unit* enemyarmy)
{
	//Distance to target
	float dist = Distance2D(unit->pos, enemyarmy->pos);
	float DIST = dist - unit->radius - enemyarmy->radius;

	//Our range
	float unitattackrange = getAttackRangeGROUND(unit);
	const Unit& ENEMYARMY = *enemyarmy;

	if (DIST < unitattackrange)
	{
		if (enemyarmy->unit_type == UNIT_TYPEID::PROTOSS_PYLON || enemyarmy->unit_type == UNIT_TYPEID::ZERG_OVERLORD || enemyarmy->unit_type == UNIT_TYPEID::PROTOSS_PHOTONCANNON || enemyarmy->unit_type == UNIT_TYPEID::PROTOSS_SHIELDBATTERY || enemyarmy->unit_type == UNIT_TYPEID::ZERG_SPORECRAWLER || enemyarmy->unit_type == UNIT_TYPEID::TERRAN_MISSILETURRET || enemyarmy->unit_type == UNIT_TYPEID::TERRAN_BUNKER)
		{
			Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_VOIDRAYPRISMATICALIGNMENT);
		}

		/*for (const auto & Attribute : Observation()->GetUnitTypeData()[enemyarmy->unit_type].attributes)
		{
			if (Attribute == Attribute::Armored)
			{
				Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_VOIDRAYPRISMATICALIGNMENT);
			}
		}*/
	}
	
	SmartAttackUnit(unit, enemyarmy);

	/*if (DIST < unitattackrange && !unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::ATTACK && unit->weapon_cooldown == 0.0f)
	{
		//������ �ֵ��� �սô�
	}
	else if (unit->weapon_cooldown == 0.0f || DIST > unitattackrange + 2.0f)
	{
		SmartAttackUnit(unit, enemyarmy);
	}
	else if (IsStructure(Observation())(ENEMYARMY) && dist > 1.5f)
	{
		sc2::Point2D FrontKitingLocation = unit->pos;
		FrontKitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos) * 1.0f;

		SmartMove(unit, FrontKitingLocation);
	}*/
}

void MEMIBot::OracleKiting(const Unit* unit, const Unit* enemyarmy)
{
	Units NearbyAirAttackers = FindUnitsNear(unit, 20, Unit::Alliance::Enemy, AirAttacker());
	Units NearbyWorkers = FindUnitsNear(unit, 20, Unit::Alliance::Enemy, IsWorker());

	const Unit * Workertarget = GetOracleTarget(unit, NearbyWorkers);
	const Unit * Armytarget = GetTarget(unit, NearbyAirAttackers);

	float dist = Distance2D(unit->pos, Workertarget->pos);
	float DIST = dist - unit->radius - Workertarget->radius;

	float myattackrange = getAttackRangeGROUND(unit);

	if (DIST <= myattackrange && !unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::ATTACK && unit->weapon_cooldown == 0.0f) // ���� ������ ������Ȳ��
	{
		//������ �ֵ��� �սô�
	}
	else if (unit->weapon_cooldown == 0.0f)
	{
		SmartAttackMove(unit, Workertarget->pos);
	}
	else
	{
		sc2::Point2D FrontKitingLocation = unit->pos;
		FrontKitingLocation -= CalcKitingPosition(unit->pos, Workertarget->pos) * 4.0f;
		
		//TEST �����̸鼭 �����°� ȿ�������� �ƴѰ� ȿ�������� �׽�Ʈ
		SmartMove(unit, FrontKitingLocation);
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

		if (IsTurretType()(ENEMYARMY))
		{
			SmartMoveEfficient(unit, KitingLocation, enemyarmy);
		}
		else if (IsStructure(Observation())(ENEMYARMY)) // �ǹ��̸�
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

void MEMIBot::SentryKiting(const Unit* unit, const Unit* enemyarmy)
{
	//Distance to target
	float dist = Distance2D(unit->pos, enemyarmy->pos);
	float DIST = dist - unit->radius - enemyarmy->radius;

	//Our range
	float unitattackrange = getAttackRangeGROUND(unit);

	const Unit& ENEMYARMY = *enemyarmy;

	//Units my_army = Observation()->GetUnits(Unit::Alliance::Self, IsNearbyArmies(Observation(), unit->pos, 10));
	//Units enemy_army = Observation()->GetUnits(Unit::Alliance::Self, IsNearbyArmies(Observation(), enemyarmy->pos, 10));
	Units NearbyArmies = FindUnitsNear(unit, 10, Unit::Alliance::Enemy, IsArmy(Observation()));

	//float myDps = getunitsDpsGROUND(my_army);
	//float enemyDps = getunitsDpsGROUND(enemy_army);


	//���� �������ε� WC�� 0�̸� ���д� or ���� ���ݰ����ϸ� �����ϰ� or ���� �ָ� ��������

	if (NearbyArmies.size() > 5)
	{
		Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_GUARDIANSHIELD);
	}
	
	if (unitattackrange + 1.0f < DIST) // �ִ��Ÿ��� 0.3 ����
	{
		SmartAttackUnit(unit, enemyarmy);
	}
	else
	{
		sc2::Point2D KitingLocation = unit->pos;
		KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos) * 10.0f;

		SmartMoveEfficient(unit, KitingLocation, enemyarmy);

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
