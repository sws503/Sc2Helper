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

	//TODO 경장갑과 무장갑이 섞여있는 경우 생각하기 & 방어력 생각하기
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

		//TODO : 체력 다 합치기 (지금은 그냥 테스트라서 냅둠)

		//적 방어타입을 계산 (경장갑인지 확인하기위해)
		for (const auto & Attribute : Observation()->GetUnitTypeData()[enemy->unit_type].attributes)
		{
			if (Attribute == Attribute::Light)
			{
				isLight = true;
			}
		}


		if (isLight) // 적이 경장갑인 경우
		{
			OracleDPS = 22 * 1.1;
			OracleDeal = (Oracle->energy - gap) / 2 * OracleDPS;
		}

		//Get its weapon
		AirRange = getAttackRange(enemy);

		if (AirRange) // 공중공격 못하면 0이므로
		{
			EnemyHP = enemy->health + enemy->shield;
			TotalEnemyHP += EnemyHP;



			//DPS를 계산
			for (const auto & Weapon : Observation()->GetUnitTypeData()[enemy->unit_type].weapons)
			{
				if (Weapon.type == Weapon::TargetType::Ground)
					continue;

				dps = Weapon.attacks * Weapon.damage_ / Weapon.speed;

				if (enemy->is_flying) //TODO 펄서광선을 킬때 알고리즘 고려하자..
				{
					dps = 100;
				}
			}
		}
		else //적이 공중공격을 할수 없음  > 무조건 내가 이김
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

bool MustAttack = false;
bool StalkerMustAttack = false;

void MEMIBot::ManageRush() { 

	const ObservationInterface* observation = Observation();
	Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
	Units enemy_army = observation->GetUnits(Unit::Alliance::Enemy, IsArmy(observation));

	Units Adepts = observation->GetUnits(Unit::Alliance::Self, IsAdept());
	Units AdeptShades = observation->GetUnits(Unit::Alliance::Self, IsAdeptShade());
	Units EnemyWorker = observation->GetUnits(Unit::Alliance::Enemy, IsWorker());
	size_t CurrentStalker = CountUnitType(observation, UNIT_TYPEID::PROTOSS_STALKER);
	size_t CurrentAdept = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ADEPT);

	

	Units RangedUnitTargets;

	Units rangedunits = observation->GetUnits(Unit::Alliance::Self, IsRanged(observation));

	// TODO : 정확하게 shadeneararmies 받아오기
	Units ShadeNearEnemies;
	Units ShadeNearArmies;
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
		Units NearbyArmies = observation->GetUnits(Unit::Alliance::Enemy, IsNearbyArmies(observation, unit->pos, 15));
		Units NearbyEnemies = observation->GetUnits(Unit::Alliance::Enemy, IsNearbyEnemies(observation, unit->pos, 20)); //각 유닛의 근처에 있는 
		float TargetAttackRange = 0.0f;
		float UnitAttackRange = getAttackRangeGROUND(unit);

		// 타겟을 받아옵니다 *^^*
		const Unit * target = GetTarget(unit, NearbyEnemies);
		
		if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_STALKER)
		{
			if (MustAttack == true && target == nullptr)
			{
				ScoutWithUnit(unit, observation);
			}

			if (target != nullptr) // 카이팅은 항상하자
			{
				if (BlinkResearched)
				{
					ManageBlink(unit, target);
				}
				Kiting(unit, target);
			}
			
			if (CurrentStalker < 12 && !StalkerMustAttack && unit->orders.empty())
			{
				RetreatWithUnit(unit, advance_pylon_location);
			}
			else if (CurrentStalker == 1 && StalkerMustAttack)
			{
				StalkerMustAttack = false;
			}
			else if (CurrentStalker >= 12)
			{
				StalkerMustAttack = true;
				if (target == nullptr)
				{
					ScoutWithUnit(unit, observation);
				}
			}

		}


		if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_ADEPT)
		{
			bool ComeOn = false;
			if (NearbyWorkers.size() > 0) // 만약 주변에 진짜 가까운 일꾼이 있으면
			{
				const Unit * Workertarget = GetTarget(unit, NearbyWorkers);
				Chat("FrontKiting Activating");
				

				if(target != nullptr) AdeptPhaseShift(unit, ShadeNearArmies, NearbyArmies, ComeOn);
				if (ComeOn)
				{
					ComeOnKiting(unit, target); // 적을 유인하는 Kiting
				}
				else
				{
					FrontKiting(unit, Workertarget);
				}
			}
			else if (CurrentAdept < 6 && !MustAttack)
			{
				RetreatWithUnit(unit, advance_pylon_location);
			}
			else if (CurrentAdept == 1 && MustAttack)
			{
				MustAttack = false;
			}
			else //(CurrentAdept >= 8)
			{
				MustAttack = true;
				if (target == nullptr)
				{
					ScoutWithUnit(unit, observation);
				}
				else // 타겟이 존재할 때
				{
					AdeptPhaseShift(unit, ShadeNearArmies, NearbyArmies, ComeOn);
					if (ComeOn)
					{
						ComeOnKiting(unit, target); // 적을 유인하는 Kiting
					}
					else
					{
						Kiting(unit, target);
					}
					
				}				
			}
		}

		if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_COLOSSUS)
		{
			if (target == nullptr)
			{
				ScoutWithUnit(unit, observation);
			}
			else // 타겟이 존재할 때
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



void MEMIBot::AdeptPhaseShift(const Unit* unit, Units ShadeNearEnemies , Units NearbyEnemies, bool & ComeOn)
{
	bool Timer = false;

	// nullpointer
	const Unit * EnemyExpansionMineral = FindNearestMineralPatch(Enemy_front_expansion);
	const Unit * EnemyBaseMineral = FindNearestMineralPatch(game_info_.enemy_start_locations.front());

	// **********************TEST 용 입니다 **************************
	bool ControlTest = false; 
	if (ControlTest)
	{
		Chat("Warning!! TEST MODE");
		AdeptPhaseToLocation(unit, Point2D(100,50), Timer, ComeOn);
	}
	// **********************TEST 용 입니다 **************************
	else if (Distance2D(EnemyBaseMineral->pos, unit->pos) <= 15) //적 기지근처에 있으면 적 앞마당으로 분신을 날린다
	{
		Point2D HarassLocation = EnemyExpansionMineral->pos;
		Point2D KitingLocation = CalcKitingPosition(EnemyExpansionMineral->pos, Enemy_front_expansion);
		HarassLocation += KitingLocation * 5.0f;

		AdeptPhaseToLocation(unit, HarassLocation, Timer, ComeOn);
	}
	else // 그 경우가 아니면 일반적으로 적 기지로 분신을 날린다
	{
		Point2D HarassLocation = EnemyBaseMineral->pos;
		Point2D KitingLocation = CalcKitingPosition(EnemyBaseMineral->pos, EnemyBaseLocation);
		HarassLocation += KitingLocation * 5.0f;

		AdeptPhaseToLocation(unit, HarassLocation , Timer, ComeOn);
	}

	if (Timer)
	{
		std::cout << ShadeNearEnemies.size() << ">=" << NearbyEnemies.size() << "=" << (ShadeNearEnemies.size() >= NearbyEnemies.size()) << std::endl;

		if (ShadeNearEnemies.size() > NearbyEnemies.size()) // TODO : DPS로 계산하기
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
	std::cout << "Manage BLINK !!";

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

void MEMIBot::FrontKiting(const Unit* unit, const Unit* enemyarmy)
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
	else if (DIST > 3.0f) // 거리가 멀어서 추격해야 하는 상황
	{
		sc2::Point2D KitingLocation = unit->pos;
		//If its a building we want range -1 distance
		//The same is true if it outranges us. We dont want to block following units
		KitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos);

		if (!Observation()->IsPathable(KitingLocation)) // 이동할 위치가 지상유닛이 갈 수 없는 곳이라면
		{
			EmergencyKiting(unit, enemyarmy);
		}
		else if (float pathingdistance = Query()->PathingDistance(unit, KitingLocation) > 10) // 이동할 위치가 멀리 돌아가야하는 곳이라면
		{
			EmergencyKiting(unit, enemyarmy);
		}
		else
		{
			Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
		}
	}
	else // 거리가 가까워서 도망가야 하는 상황
	{
		sc2::Point2D KitingLocation = unit->pos;
		//If its a building we want range -1 distance
		//The same is true if it outranges us. We dont want to block following units
		KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos);

		if (!Observation()->IsPathable(KitingLocation)) // 이동할 위치가 지상유닛이 갈 수 없는 곳이라면
		{
			EmergencyKiting(unit, enemyarmy);
		}
		else if (float pathingdistance = Query()->PathingDistance(unit, KitingLocation) > 10) // 이동할 위치가 멀리 돌아가야하는 곳이라면
		{
			EmergencyKiting(unit, enemyarmy);
		}
		else
		{
			Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
		}
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

	std::cout << getAttackRangeGROUND(enemyarmy) << "<" << unitattackrange << "=" << (getAttackRangeGROUND(enemyarmy) < unitattackrange) << "           1이면 공격합니다 ^^" <<std::endl;

	if ( (unit->weapon_cooldown == 0.0f && (getAttackRangeGROUND(enemyarmy) < unitattackrange)) || DIST > 10) //적 공격사거리가 나보다 짧으면 공격한다
	{
		Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemyarmy->pos);
	}
	else
	{
		sc2::Point2D KitingLocation = unit->pos;
		//If its a building we want range -1 distance
		//The same is true if it outranges us. We dont want to block following units
		if (IsStructure(Observation())(ENEMYARMY)) // 건물이면 달라붙죠 ㅇㅋ?
		{
			KitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos);
		}
		else // 적 공격사거리가 나랑 같거나 길면
		{
			KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos); // 적을 데려나오기 위해 뒤로 빼게 했습니다.
		}
		
		if (!Observation()->IsPathable(KitingLocation)) // 이동할 위치가 지상유닛이 갈 수 없는 곳이라면
		{
			EmergencyKiting(unit, enemyarmy);
		}
		else if (float pathingdistance = Query()->PathingDistance(unit, KitingLocation) > 10) // 이동할 위치가 멀리 돌아가야하는 곳이라면
		{
			EmergencyKiting(unit, enemyarmy);
		}
		else
		{
			Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
		}

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
		if (IsStructure(Observation())(ENEMYARMY)) // 건물이면
		{
			KitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos);
		}
		else if (getAttackRangeGROUND(enemyarmy) > unitattackrange) // 날 때릴 수 있는 적의 사정거리가 내 사정거리보다 길면
		{
			KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos) * 7;
			//KitingLocation -= CalcKitingPosition(unit->pos, enemyarmy->pos); //TODO : 적을 데려나오기 위해 임시로 뒤로 빼게 했습니다.
		}
		else
		{
			KitingLocation += CalcKitingPosition(unit->pos, enemyarmy->pos) * 7;
		}

		if (!Observation()->IsPathable(KitingLocation) ) // 이동할 위치가 지상유닛이 갈 수 없는 곳이라면
		{
			EmergencyKiting(unit, enemyarmy);
		}
		else if (float pathingdistance = Query()->PathingDistance(unit, KitingLocation) > 10) // 이동할 위치가 멀리 돌아가야하는 곳이라면
		{
			EmergencyKiting(unit, enemyarmy);
		}
		else
		{
			Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
		}
	}
}

void MEMIBot::EmergencyKiting(const Unit* unit, const Unit* enemyarmy)
{
	const ObservationInterface* observation = Observation();
	// 적의 평균 위치를 받아옵니다
	Units NearbyEnemies = observation->GetUnits(Unit::Alliance::Enemy, IsNearbyEnemies(observation, unit->pos, 10));
	Point2D enemyposition;
	GetPosition(NearbyEnemies, Unit::Alliance::Enemy, enemyposition);

	//Distance to target
	float dist = Distance2D(unit->pos, enemyarmy->pos);
	float DIST = dist - unit->radius - enemyarmy->radius;

	Point2D KitingLocation1 = unit->pos;
	Point2D KitingLocation2 = unit->pos;

	Point2D Vector = CalcKitingPosition(unit->pos, enemyarmy->pos);

	KitingLocation1 += Point2D(-Vector.y, +Vector.x) * 5;
	KitingLocation2 += Point2D(+Vector.y, -Vector.x) * 5;

	Point2D KitingLocation;

	if (Distance2D(KitingLocation1, enemyposition) < Distance2D(KitingLocation2, enemyposition)) //1로 가는게 2로 가는 것보다 안전한 경우
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

