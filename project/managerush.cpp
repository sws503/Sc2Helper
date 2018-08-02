#include "memibot.h"

/*bool MEMIBot::ExpectBattle(Units myunits, Units enemyunits, bool OracleCanAttack)
{
	const ObservationInterface* observation = Observation();
	//Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);

	float UnitHP = Oracle->health + Oracle->shield;
	float TotalUnitHP
	float TotalEnemyHP = 0.1f;
	float TotalDPS = 0.1f;
	float AirRange = 0.0f;
	float dps = 0.0f;

	//TODO ���尩�� ���尩�� �����ִ� ��� �����ϱ� & ���� �����ϱ�
	float gap = !OracleCanAttack * 25;
	float OracleDPS = 15 * 1.1;
	float OracleDeal = (Oracle->energy - gap) / 2 * OracleDPS;

	float EnemyHP = 0.0f;

	for (const auto & myunit : myunits)
	{
		TotalUnitHP
	}

	for (const auto & enemy : enemyunits)
	{
		bool isLight = false;
		// if it's a combat unit

		//TODO : ü�� �� ��ġ�� (������ �׳� �׽�Ʈ�� ����)

		//�� ���Ÿ���� ��� (���尩���� Ȯ���ϱ�����)
		for (const auto & Attribute : Observation()->GetUnitTypeData()[enemy->unit_type].attributes)
		{
			if (Attribute == Attribute::Light)
			{
				isLight = true;
			}
		}


		if (isLight) // ���� ���尩�� ���
		{
			OracleDPS = 22 * 1.1;
			OracleDeal = (Oracle->energy - gap) / 2 * OracleDPS;
		}

		//Get its weapon
		AirRange = getAttackRange(enemy);

		if (AirRange) // ���߰��� ���ϸ� 0�̹Ƿ�
		{
			EnemyHP = enemy->health + enemy->shield;
			TotalEnemyHP += EnemyHP;



			//DPS�� ���
			for (const auto & Weapon : Observation()->GetUnitTypeData()[enemy->unit_type].weapons)
			{
				if (Weapon.type == Weapon::TargetType::Ground)
					continue;

				dps = Weapon.attacks * Weapon.damage_ / Weapon.speed;

				if (enemy->is_flying) //TODO �޼������� ų�� �˰��� �������..
				{
					dps = 100;
				}
			}
		}
		else //���� ���߰����� �Ҽ� ����  > ������ ���� �̱�
		{
			dps = 0.0;
		}
		TotalDPS += dps;
	}*/

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

struct IsWarpPrism {
	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_WARPPRISM: return true;
		case UNIT_TYPEID::PROTOSS_WARPPRISMPHASING: return true;
		default: return false;
		}
	}
};

struct IsColossus {
	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_COLOSSUS: return true;
		default: return false;
		}
	}
};

struct IsNearbyWorker {
	IsNearbyWorker(const ObservationInterface* obs, Point2D MyPosition, int Radius) :
		observation_(obs), mp(MyPosition), radius(Radius) {}

	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::ZERG_DRONE:
		case UNIT_TYPEID::TERRAN_SCV:
		case UNIT_TYPEID::TERRAN_MULE:
		case UNIT_TYPEID::PROTOSS_PROBE:return Distance2D(mp, unit.pos) < radius;


		default: return false;
		}
	}
private:
	const ObservationInterface* observation_;
	Point2D mp;
	int radius;
};

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

struct IsNearbyEnemies {
	IsNearbyEnemies(const ObservationInterface* obs, Point2D MyPosition, int Radius) :
		observation_(obs), mp(MyPosition), radius(Radius) {}

	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::ZERG_OVERLORD: return false;
		case UNIT_TYPEID::ZERG_LARVA: return false;
		case UNIT_TYPEID::ZERG_EGG: return false;
		case UNIT_TYPEID::TERRAN_MULE: return false;
		case UNIT_TYPEID::TERRAN_NUKE: return false;

		default:
			return Distance2D(mp, unit.pos) < radius;
		}
	}
private:
	const ObservationInterface* observation_;
	Point2D mp;
	int radius;
};

bool MustAttack = false;
bool StalkerMustAttack = false;

////////////////////////////////
/*void shuttle::loadPassangers()
{
	if (m_shuttle->getCargoSpaceTaken() < m_passengers.size())
	{
		Micro::SmartRightClick(m_passengers, m_shuttle, *m_bot);
		Micro::SmartRightClick(m_shuttle, m_passengers, *m_bot);
		Micro::SmartCDAbility(m_shuttle, sc2::ABILITY_ID::EFFECT_MEDIVACIGNITEAFTERBURNERS, *m_bot);
	}
	else
	{
		m_status = ShuttleStatus::OnMyWay;
	}
}*/

bool MEMIBot::LoadUnit(const Unit * unit, const Unit* passenger)
{
	uint32_t game_loop = Observation()->GetGameLoop();
	Actions()->UnitCommand(unit, ABILITY_ID::LOAD, passenger);
	return true;
}


bool MEMIBot::LoadUnitWeaponCooldown(const Unit * unit, const Unit* passenger)
{
	uint32_t game_loop = Observation()->GetGameLoop();


	float unitWC = unit->weapon_cooldown; // ��� �� ������
	float StepWC = unitWC * 21.7;
	Actions()->UnitCommand(unit, ABILITY_ID::LOAD, passenger); // �¿��

	if (unit->last_seen_game_loop + StepWC <= game_loop) //�ð��� �Ǹ�

	{
		Actions()->UnitCommand(unit, ABILITY_ID::UNLOADALLAT_WARPPRISM, unit->pos); // �����ش�
		return true;
	}
}

const Unit * MEMIBot::GetPassenger(const Unit * shuttle, Units & passengers)
{
	double closestDist = std::numeric_limits<double>::max();
	double lowestHealth = std::numeric_limits<double>::max();
	const Unit * closestTargetOutsideRange = nullptr;
	const Unit * weakestTargetInsideRange = nullptr;
	int highWCNear = 0;
	const float range = 6.0f;

	for (const auto & targetUnit : passengers)
	{
		if (!targetUnit->is_alive)
		{
			continue;
		}
		const float distance = Distance2D(shuttle->pos, targetUnit->pos);
		float UnitWC = targetUnit->weapon_cooldown;

		if (distance > range) // �Ÿ��� �־ �ٷ� �¿� �� ���� ���
		{
			// ���� ����� ������ ����
			if (!closestTargetOutsideRange || distance < closestDist)
			{
				closestDist = distance;
				closestTargetOutsideRange = targetUnit;
			}
		}
		else // ������� �ٷ� �¿� �� �ִ� ���
		{
			// �ֱٿ� �����ؼ� ���� ��Ÿ���� �� ������ ����
			if (!weakestTargetInsideRange || (UnitWC > highWCNear) || (UnitWC == highWCNear && targetUnit->health < lowestHealth))
			{
				lowestHealth = targetUnit->health;
				highWCNear = UnitWC;
				weakestTargetInsideRange = targetUnit;
			}
		}
	}
	//��Ÿ� ���� ü�� ���� �Ʊ����� �¿�� �� �¿�� ������ �ִ� �ͺ��� �¿��
	return weakestTargetInsideRange && highWCNear>1 ? weakestTargetInsideRange : closestTargetOutsideRange;
}

const Unit * MEMIBot::GetNearShuttle(const Unit * unit)
{
	const ObservationInterface* observation = Observation();
	Units WarpPrisms = observation->GetUnits(Unit::Alliance::Self, IsWarpPrism());
	double closestDist = std::numeric_limits<double>::max();
	const Unit * closest_shuttle = nullptr;

	for (const auto & shuttle : WarpPrisms)
	{
		const float distance = Distance2D(shuttle->pos, unit->pos);

		if (!closest_shuttle || distance < closestDist)
		{
			closestDist = distance;
			closest_shuttle = shuttle;
		}
	}
	//��Ÿ� ���� ü�� ���� �Ʊ����� �¿�� �� �¿�� ������ �ִ� �ͺ��� �¿��
	return closest_shuttle;
}



void MEMIBot::ManageWarpBlink(const Unit * unit)
{
	float UnitHealth = unit->health + unit->shield;

	const Unit * closest_shuttle = GetNearShuttle(unit);

	if (unit->shield == 0)
	{
		LoadUnit(closest_shuttle, unit);
	}
}

void MEMIBot::ManageRush() {


	const ObservationInterface* observation = Observation();
	Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
	Units enemy_army = observation->GetUnits(Unit::Alliance::Enemy, IsArmy(observation));
	Units my_army = observation->GetUnits(Unit::Alliance::Self, IsArmy(observation));

	Units Adepts = observation->GetUnits(Unit::Alliance::Self, IsAdept());
	Units AdeptShades = observation->GetUnits(Unit::Alliance::Self, IsAdeptShade());
	Units WarpPrisms = observation->GetUnits(Unit::Alliance::Self, IsWarpPrism());
	Units Colossuses = observation->GetUnits(Unit::Alliance::Self, IsColossus());
	Units EnemyWorker = observation->GetUnits(Unit::Alliance::Enemy, IsWorker());
	size_t CurrentStalker = CountUnitType(observation, UNIT_TYPEID::PROTOSS_STALKER);
	size_t CurrentAdept = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ADEPT);

	Units RangedUnitTargets;

	Units rangedunits = observation->GetUnits(Unit::Alliance::Self, IsRanged(observation));

	// TODO : ��Ȯ�ϰ� shadeneararmies �޾ƿ���
	Units ShadeNearEnemies;
	Units ShadeNearArmies;

	for (const auto& unit : WarpPrisms)
	{
		Units NearbyArmies = observation->GetUnits(Unit::Alliance::Enemy, IsNearbyArmies(observation, unit->pos, 15));

		const Unit * passenger = GetPassenger(unit, Colossuses);
		const Unit * target = GetTarget(unit, NearbyArmies);
		
		Point2D retreat_position;
		GetPosition(my_army, Unit::Alliance::Self, retreat_position);

		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, retreat_position);

		if (target == nullptr)
		{
			Point2D retreat_position;
			GetPosition(my_army, Unit::Alliance::Self, retreat_position);

			RetreatSmart(unit, retreat_position);
		}
		else
		{
			sc2::Point2D KitingLocation = unit->pos;
			KitingLocation += CalcKitingPosition(retreat_position, target->pos) * 4.0f;
			RetreatSmart(unit, KitingLocation);
		}

		if (unit->cargo_space_taken > 0)
		{
			Actions()->UnitCommand(unit, ABILITY_ID::UNLOADALLAT_WARPPRISM, unit->pos);
		}
		/*if (unit->cargo_space_taken == unit->cargo_space_max)
		{
			if (target == nullptr)
			{
				Point2D retreat_position;
				GetPosition(my_army, Unit::Alliance::Self, retreat_position);

				RetreatWithUnit(unit, retreat_position);
			}
			else
			{
				FleeKiting(unit, target);
			}
		}
		else if(passenger != nullptr)
		{
			LoadUnitWeaponCooldown(unit, passenger);
		}*/

		/*if (unit->cargo_space_taken == unit->cargo_space_max) {
		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, game_info_.enemy_start_locations.front());
		}
		Actions()->UnitCommand(unit, ABILITY_ID::LOAD, stalkers.front());

		if (Query()->PathingDistance(unit->pos, Point2D(game_info_.enemy_start_locations.front().x + 3, game_info_.enemy_start_locations.front().y)) < 20) {
		Actions()->UnitCommand(unit, ABILITY_ID::UNLOADALLAT_WARPPRISM, unit->pos);

		if (unit->cargo_space_taken == 0) {
		Actions()->UnitCommand(unit, ABILITY_ID::MORPH_WARPPRISMPHASINGMODE);
		}
		}*/
	}
	for (const auto& unit : AdeptShades) {
		ShadeNearArmies = observation->GetUnits(Unit::Alliance::Enemy, IsNearbyArmies(observation, unit->pos, 15));
		ShadeNearEnemies = observation->GetUnits(Unit::Alliance::Enemy, IsNearbyEnemies(observation, unit->pos, 15));
		Units ShadeNearWorkers = observation->GetUnits(Unit::Alliance::Enemy, IsNearbyWorker(observation, unit->pos, 15));

		for (const auto & ShadeNearWorker : ShadeNearWorkers)
		{
			/*if (unit->orders.empty()) {
				Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, ShadeNearWorker->pos);
			}*/

		}
	}

	for (const auto& unit : rangedunits) {
		Units NearbyWorkers = observation->GetUnits(Unit::Alliance::Enemy, IsNearbyWorker(observation, unit->pos, 7));
		Units NearbyArmies = observation->GetUnits(Unit::Alliance::Enemy, IsNearbyArmies(observation, unit->pos, 10));
		Units NearbyEnemies = observation->GetUnits(Unit::Alliance::Enemy, IsNearbyEnemies(observation, unit->pos, 20)); //�� ������ ��ó�� �ִ�
		float TargetAttackRange = 0.0f;
		float UnitAttackRange = getAttackRangeGROUND(unit);

		// Ÿ���� �޾ƿɴϴ� *^^*
		const Unit * target = GetTarget(unit, NearbyEnemies);

		if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_IMMORTAL)
		{
			if (MustAttack == true && target == nullptr)
			{
				ScoutWithUnit(unit, observation);
			}

			if (target != nullptr) // ī������ �׻�����
			{
				Kiting(unit, target);
			}
		}

		if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_VOIDRAY)
		{
			if (MustAttack == true && target == nullptr)
			{
				ScoutWithUnit(unit, observation);
			}

			if (target != nullptr) // ī������ �׻�����
			{
				Kiting(unit, target);
			}
		}

		if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_STALKER)
		{
			ManageWarpBlink(unit);

			if (target != nullptr) // ī������ �׻�����
			{
				if (BlinkResearched)
				{
					ManageBlink(unit, target);
				}
				Kiting(unit, target);
			}

			if (StalkerMustAttack) // ����Ÿ�̹��̸�
			{
				if (target == nullptr)
				{
					ScoutWithUnit(unit, observation);
				}
			}
			else if (!StalkerMustAttack && unit->orders.empty()) // ����Ÿ�̹��� �ƴҶ� �Ѱ��ϸ�
			{
				Roam_randombase(unit);
			}


			if (CurrentStalker >= 12)
			{
				StalkerMustAttack = true;
				
			}

		}


		if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_ADEPT)
		{
			bool ComeOn = false;
			if (target == nullptr)
			{
				if (MustAttack)
				{
					ScoutWithUnit(unit, observation);
				}
			}
			else // Ÿ���� ������ ��
			{
				if (getunitsDpsGROUND(NearbyArmies) > 20.0f)
				{
					AdeptPhaseShift(unit, ShadeNearArmies, NearbyArmies, ComeOn);
				}

				const Unit * Armytarget = GetTarget(unit, NearbyArmies);
				if (ComeOn && Armytarget != nullptr) //�н��� ������ ��ó�� �� ������ �ִ� ���
				{
					ComeOnKiting(unit, Armytarget);
				}
				else if (NearbyWorkers.size() > 0) // ��ó�� �� �ϲ��� �ִµ�
				{
					const Unit * Workertarget = GetTarget(unit, NearbyWorkers);

					if (!NearbyArmies.empty()) // �������ֵ� ���� ������
					{
						const Unit * Armytarget = GetTarget(unit, NearbyArmies);
						DistanceKiting(unit, Workertarget, Armytarget);
					}
					else //�ϲ۸� ������
					{
						FrontKiting(unit, Workertarget);
					}
				}
				else //���� DPS�� ���� ���� ��
				{
					Kiting(unit, target);
				}
			}

			if (CurrentAdept < 9 && !MustAttack && unit->orders.empty())
			{
				RetreatWithUnit(unit, advance_pylon_location);
			}
			else if (CurrentAdept == 1 && MustAttack)
			{
				MustAttack = false;
			}
			else //(CurrentAdept >= 9)
			{
				MustAttack = true;
			}
		}

		if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_COLOSSUS)
		{
			if (target == nullptr)
			{
				ScoutWithUnit(unit,  observation);
			}
			else // Ÿ���� ������ ��
			{
				Kiting(unit, target);
			}
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


			if (distance <= TargetAttackRange) //��ó�� �������� KitingLocation�� ������Ʈ�Ѵ�
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

void  MEMIBot::Roam_randombase(const Unit* unit)
{
	const ObservationInterface* observation = Observation();
	Units bases = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));

	bool good_position = false;
	Point2D mp = bases.front()->pos;

	float rx = GetRandomScalar();
	float ry = GetRandomScalar();
	Point2D RoamPosition = Point2D(mp.x + rx * 20, mp.y + ry * 20);

	Actions()->UnitCommand(unit, ABILITY_ID::MOVE, RoamPosition);
}

void MEMIBot::AdeptPhaseShift(const Unit* unit, Units ShadeNearEnemies , Units NearbyEnemies, bool & ComeOn)
{
	bool Timer = false;

	// nullpointer
	const Unit * EnemyExpansionMineral = FindNearestMineralPatch(Enemy_front_expansion);
	const Unit * EnemyBaseMineral = FindNearestMineralPatch(game_info_.enemy_start_locations.front());

	// **********************TEST �� �Դϴ� **************************
	bool ControlTest = false;
	if (ControlTest)
	{
		Chat("Warning!! TEST MODE");
		AdeptPhaseToLocation(unit, Point2D(100,50), Timer, ComeOn);
	}
	// **********************TEST �� �Դϴ� **************************
	else if (Distance2D(EnemyBaseMineral->pos, unit->pos) <= 15) //�� ������ó�� ������ �� �ո������� �н��� ������
	{
		Point2D HarassLocation = EnemyExpansionMineral->pos;
		Point2D KitingLocation = CalcKitingPosition(EnemyExpansionMineral->pos, Enemy_front_expansion);
		HarassLocation += KitingLocation * 1.0f;

		AdeptPhaseToLocation(unit, HarassLocation, Timer, ComeOn);
	}
	else // �� ��찡 �ƴϸ� �Ϲ������� �� ������ �н��� ������
	{
		Point2D HarassLocation = EnemyBaseMineral->pos;
		Point2D KitingLocation = CalcKitingPosition(EnemyBaseMineral->pos, EnemyBaseLocation);
		HarassLocation += KitingLocation * 5.0f;

		AdeptPhaseToLocation(unit, HarassLocation , Timer, ComeOn);
	}

	if (Timer)
	{
		std::cout << ShadeNearEnemies.size() << ">=" << NearbyEnemies.size() << "=" << (ShadeNearEnemies.size() >= NearbyEnemies.size()) << std::endl;

		if (ShadeNearEnemies.size() > NearbyEnemies.size()) // TODO : DPS�� ����ϱ�
		{
			Actions()->UnitCommand(unit, ABILITY_ID::CANCEL_ADEPTPHASESHIFT);
		}
	}
}

void MEMIBot::AdeptPhaseToLocation(const Unit* unit, Point2D Location , bool & Timer, bool & ComeOn)
{
	Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_ADEPTPHASESHIFT, Location);

	const ObservationInterface* observation = Observation();
	uint32_t game_loop = Observation()->GetGameLoop();

	if (adept_map.count(unit->tag) == 0) {
		adept_map.insert(std::make_pair((unit->tag), observation->GetGameLoop()));
		Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_ADEPTPHASESHIFT, Location);
	}
	if (adept_map.find(unit->tag)->second + 145 <= game_loop)
	{
		Timer = true;
		adept_map.erase(unit->tag);
	}
	else
	{
		ComeOn = true;
	}
}

void MEMIBot::ManageBlink(const Unit* unit, const Unit* target)
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

void MEMIBot::FleeKiting(const Unit* unit, const Unit* enemyarmy)
{
	const ObservationInterface* observation = Observation();
	Units NearbyArmies = observation->GetUnits(Unit::Alliance::Enemy, IsNearbyArmies(observation, unit->pos, 10));
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
		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
	}
}

void MEMIBot::DistanceKiting(const Unit* unit, const Unit* enemyarmy, const Unit* army) // �ϲ� ���񺴷��� �ִ� ���
{
	//Distance to target
	float dist = Distance2D(unit->pos, enemyarmy->pos);
	float DIST = dist - unit->radius - enemyarmy->radius;

	//Our range
	float unitattackrange = getAttackRangeGROUND(unit);

	const Unit& ENEMYARMY = *enemyarmy;

	if (unit->weapon_cooldown == 0.0f)
	{
		Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemyarmy->pos);
	}
	else // �Ÿ��� ������� �������� �ϴ� ��Ȳ
	{
		sc2::Point2D KitingLocation = unit->pos;
		KitingLocation += CalcKitingPosition(unit->pos, army->pos) * 4;

		if (!Observation()->IsPathable(KitingLocation)) // �̵��� ��ġ�� ���������� �� �� ���� ���̶��
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
		}
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

	if (unit->weapon_cooldown == 0.0f)
	{
		Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemyarmy->pos);
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
		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
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
		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
	}
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

	if ( (unit->weapon_cooldown == 0.0f && (getAttackRangeGROUND(enemyarmy) < unitattackrange)) || DIST > getAttackRangeGROUND(enemyarmy) + 4) //�� ���ݻ�Ÿ��� ������ ª���� �����Ѵ�
	{
		Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemyarmy->pos);
	}
	else
	{
		sc2::Point2D KitingLocation = unit->pos;
		KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos); // ���� ���������� ���� �ڷ� ���� �߽��ϴ�.
				

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
		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
	}
}

void MEMIBot::Kiting(const Unit* unit, const Unit* enemyarmy)
{
	//Distance to target
	float dist = Distance2D(unit->pos, enemyarmy->pos);
	float DIST = dist - unit->radius - enemyarmy->radius;

	//Our range
	float unitattackrange = getAttackRangeGROUND(unit);

	const Unit& ENEMYARMY = *enemyarmy;

	if (unit->weapon_cooldown == 0.0f || DIST > unitattackrange + 2.0f)
	{
		Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemyarmy->pos);
	}
	else
	{
		sc2::Point2D KitingLocation = unit->pos;
		//If its a building we want range -1 distance
		//The same is true if it outranges us. We dont want to block following units
		if (IsStructure(Observation())(ENEMYARMY)) // �ǹ��̸�
		{
			KitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos);
		}
		else if (getAttackRangeGROUND(enemyarmy) > unitattackrange) // �� ���� �� �ִ� ���� �����Ÿ��� �� �����Ÿ����� ���
		{
			KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos) * 7;
			//KitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos); //TODO : ���� ���������� ���� �ӽ÷� �ڷ� ���� �߽��ϴ�.
		}
		else
		{
			KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos) * 7;
		}

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
		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
	}
}

void MEMIBot::EmergencyKiting(const Unit* unit, const Unit* enemyarmy)
{
	const ObservationInterface* observation = Observation();
	// ���� ��� ��ġ�� �޾ƿɴϴ�
	Units NearbyArmies = observation->GetUnits(Unit::Alliance::Enemy, IsNearbyArmies(observation, unit->pos, 10));
	Point2D enemyposition;
	GetPosition(NearbyArmies, Unit::Alliance::Enemy, enemyposition);

	//Distance to target
	float dist = Distance2D(unit->pos, enemyarmy->pos);
	float DIST = dist - unit->radius - enemyarmy->radius;

	Point2D KitingLocation1 = unit->pos;
	Point2D KitingLocation2 = unit->pos;

	Point2D Vector = CalcKitingPosition(unit->pos, enemyarmy->pos);

	KitingLocation1 += Point2D(-Vector.y, +Vector.x) * 5;
	KitingLocation2 += Point2D(+Vector.y, -Vector.x) * 5;

	Point2D KitingLocation;

	if (Distance2D(KitingLocation1, enemyposition) < Distance2D(KitingLocation2, enemyposition)) //1�� ���°� 2�� ���� �ͺ��� ������ ���
	{
		KitingLocation = KitingLocation1;
	}
	else
	{
		KitingLocation = KitingLocation2;
	}
	Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
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

void MEMIBot::KiteEnemy(const Unit* unit, Units enemy_army, Units enemy_units, Point2D KitingLocation, bool enemiesnear, const ObservationInterface* observation)
{
	if (enemy_units.empty()) // ���� �ƿ� ���� ���
	{
		ScoutWithUnit(unit, observation);
	}
	else if (enemy_army.empty()) //�� ���밡 ���� ���
	{
		AttackWithUnit(unit, observation);
	}

	else if (!enemy_army.empty()) //�� ���밡 ���� ��
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
	Vector2D diff = Mypos - EnemyPos; // 7.3 �� ���ְ��� �ݴ� �������� ����
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
	if (unit.unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_MEDIVAC)
	{
		return 10;
	}
	if (unit.unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_PHOTONCANNON || unit.unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_SPINECRAWLER)
	{
		return 10;
	}
	if (IsWorker()(unit))
	{
		return 10;
	}
	if (unit.unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_PYLON || unit.unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_SPORECRAWLER || unit.unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_MISSILETURRET)
	{
		return 5;
	}
	if (IsTownHall()(unit))
	{
		return 4;
	}
	return 2;
}

 const Unit * MEMIBot::GetTarget(const Unit * rangedUnit, Units & targets)
{
	
	int highPriorityFar = 0;
	int highPriorityNear = 0;
	int highDpsNear = 0;
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
		const float range = getAttackRangeGROUND(rangedUnit); //rangedUnit->getAttackRange(targetUnit); �絵 ��Ÿ� 4
		int priority = getAttackPriority(targetUnit);
		float target_dps = getDpsGROUND(targetUnit);

		const float distance = Distance2D(rangedUnit->pos, targetUnit->pos);

		if (distance > range) //���� �� ������ �Ÿ� > ���� �����Ÿ� // �Ÿ��� �־ ���� �� ���� ���
		{
			// If in sight we just add 20 to prio. This should make sure that a unit in sight has higher priority than any unit outside of range
			//
			float SightRange = Observation()->GetUnitTypeData()[rangedUnit->unit_type].sight_range;

			if (distance <= SightRange)
			{
				priority += 20;
			}
			// �켱������ ���ų� �Ǵ� �Ÿ��� ������ �����Ѵ�
			if (!closestTargetOutsideRange || (priority > highPriorityFar) || (priority == highPriorityFar && distance < closestDist))
			{
				closestDist = distance;
				highPriorityFar = priority;
				closestTargetOutsideRange = targetUnit;
			}
		}
		else
		{
			if (!weakestTargetInsideRange || (priority > highPriorityNear) || (priority == highPriorityNear && targetUnit->health < lowestHealth)) // �ǹ��� ������ ���� ����
			{
				lowestHealth = targetUnit->health + targetUnit->shield;
				highPriorityNear = priority;
				weakestTargetInsideRange = targetUnit;

			}
		}
	}
	return weakestTargetInsideRange && highPriorityNear>1 ? weakestTargetInsideRange : closestTargetOutsideRange;
}

 const float MEMIBot::getunitsDpsGROUND(Units targets) const
 {

	 const ObservationInterface* observation = Observation();

	 float dps = 0.0f;
	 float total = 0.0f;

	 for (const auto & target : targets)
	 {
		 if (target->unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_BUNKER)
		 {
			 return 50.0f;
		 }

		 for (const auto & Weapon : Observation()->GetUnitTypeData()[target->unit_type].weapons)
		 {
			 if (Weapon.type == Weapon::TargetType::Air)
				 continue;

			 dps = Weapon.attacks * Weapon.damage_ / Weapon.speed;
		 }
		 total += dps;
	 }
	 return total;
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
		if (target->unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_BUNKER)
		{
			return 6.0f;
		}
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
	//return AirWeapons.range; // 7.5 �̰� ������ ���������� ���� �Լ�!!
	return groundWeapons.range; // 7.5 ���������� ���� �Լ�
}

