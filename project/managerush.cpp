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

struct IsObserver {
	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_OBSERVER: return true;
		default: return false;
		}
	}
};
struct IsAdept {
	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_ADEPT: return true;
		default: return false;
		}
	}
};
struct IsStalker {
	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_STALKER: return true;
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
bool AdeptMustAttack = false;

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


	float unitWC = unit->weapon_cooldown; // 계산 한 다음에
	float StepWC = unitWC * 21.7;
	Actions()->UnitCommand(unit, ABILITY_ID::LOAD, passenger); // 태우고

	if (unit->last_seen_game_loop + StepWC <= game_loop) //시간이 되면

	{
		Actions()->UnitCommand(unit, ABILITY_ID::UNLOADALLAT_WARPPRISM, unit->pos); // 내려준다
		return true;
	}
	return false;
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

		if (distance > range) // 거리가 멀어서 바로 태울 수 없는 경우
		{
			// 가장 가까운 유닛을 고른다
			if (!closestTargetOutsideRange || distance < closestDist)
			{
				closestDist = distance;
				closestTargetOutsideRange = targetUnit;
			}
		}
		else // 가까워서 바로 태울 수 있는 경우
		{
			// 최근에 공격해서 공격 쿨타임이 긴 유닛을 고른다
			if (!weakestTargetInsideRange || (UnitWC > highWCNear) || (UnitWC == highWCNear && targetUnit->health < lowestHealth))
			{
				lowestHealth = targetUnit->health;
				highWCNear = UnitWC;
				weakestTargetInsideRange = targetUnit;
			}
		}
	}
	//사거리 내의 체력 낮은 아군부터 태우고 다 태우면 가까이 있는 것부터 태운다
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
	//사거리 내의 체력 낮은 아군부터 태우고 다 태우면 가까이 있는 것부터 태운다
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

bool MEMIBot::IsUnitInUnits(const Unit* unit, Units& units) {
	if (unit == nullptr) return false;
	for (const auto& u : units) {
		if (u->tag == unit->tag) return true;
	}
	return false;
}

/*void MEMIBot::DoGuerrillaWarp(const Unit * unit)
{


	float x_min = static_cast<float>(Observation()->GetGameInfo().playable_min.x);
	float x_max = static_cast<float>(Observation()->GetGameInfo().playable_max.x);
	float y_min = static_cast<float>(Observation()->GetGameInfo().playable_min.y);
	float y_max = static_cast<float>(Observation()->GetGameInfo().playable_max.y);

	//1 2
	//3 4
	Point2D Edge1 = Point2D(x_min, y_max);
	Point2D Edge2 = Point2D(x_max, y_max);
	Point2D Edge3 = Point2D(x_min, y_min);
	Point2D Edge4 = Point2D(x_max, y_min);

	float Distance1 = Distance2D(startLocation_, Edge1);
	float Distance2 = Distance2D(startLocation_, Edge2);
	float Distance3 = Distance2D(startLocation_, Edge3);
	float Distance4 = Distance2D(startLocation_, Edge4);

	MinimumDistance2D

	if(Distance2D(startLocation_, ))

	Units Stalkers = Observation()->GetUnits(Unit::Alliance::Self, IsStalker());
	const Unit* passenger = GetPassenger(unit, Stalkers);

	if (unit->cargo_space_taken == unit->cargo_space_max) {
		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, game_info_.enemy_start_locations.front());
	}
	Actions()->UnitCommand(unit, ABILITY_ID::LOAD, passenger);


	if (Query()->PathingDistance(unit->pos, Point2D(game_info_.enemy_start_locations.front().x + 3, game_info_.enemy_start_locations.front().y)) < 20) {
		Actions()->UnitCommand(unit, ABILITY_ID::UNLOADALLAT_WARPPRISM, unit->pos);

		if (unit->cargo_space_taken == 0) {
			Actions()->UnitCommand(unit, ABILITY_ID::MORPH_WARPPRISMPHASINGMODE);
		}
	}

	//TODO : enemy_townhalls_scouter_seen 를 사용해서 하기

	const Unit * EnemyExpansionMineral = FindNearestMineralPatch(enemy_expansion);
	const Unit * EnemyBaseMineral = FindNearestMineralPatch(game_info_.enemy_start_locations.front());

	if (Distance2D(EnemyBaseMineral->pos, unit->pos) <= 15) //적 기지근처에 있으면 적 앞마당으로 분신을 날린다
	{
		Point2D HarassLocation = EnemyExpansionMineral->pos;
		Point2D KitingLocation = CalcKitingPosition(EnemyExpansionMineral->pos, Enemy_front_expansion);
		HarassLocation += KitingLocation * 1.0f;

		AdeptPhaseToLocation(unit, HarassLocation, Timer, ComeOn);
	}
	else // 그 경우가 아니면 일반적으로 적 기지로 분신을 날린다
	{
		Point2D HarassLocation = EnemyBaseMineral->pos;
		Point2D KitingLocation = CalcKitingPosition(EnemyBaseMineral->pos, EnemyBaseLocation);
		HarassLocation += KitingLocation * 5.0f;

		AdeptPhaseToLocation(unit, HarassLocation, Timer, ComeOn);
	}
}*/
void MEMIBot::Merge(const Unit* unit, Point2D mergelocation)
{

}



void MEMIBot::ManageRush() {


	const ObservationInterface* observation = Observation();
	Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
	Units enemy_army = observation->GetUnits(Unit::Alliance::Enemy, IsArmy(observation));
	Units my_army = observation->GetUnits(Unit::Alliance::Self, IsArmy(observation));

	Units Adepts = observation->GetUnits(Unit::Alliance::Self, IsAdept());
	
	Units Observers = observation->GetUnits(Unit::Alliance::Self, IsObserver());
	Units AdeptShades = observation->GetUnits(Unit::Alliance::Self, IsAdeptShade());
	Units WarpPrisms = observation->GetUnits(Unit::Alliance::Self, IsWarpPrism());
	Units Colossuses = observation->GetUnits(Unit::Alliance::Self, IsColossus());
	Units EnemyWorker = observation->GetUnits(Unit::Alliance::Enemy, IsWorker());
	size_t CurrentStalker = CountUnitType(observation, UNIT_TYPEID::PROTOSS_STALKER);
	size_t CurrentAdept = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ADEPT);

	Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());

	Units RangedUnitTargets;

	Units rangedunits = observation->GetUnits(Unit::Alliance::Self, IsRanged(observation));

	// TODO : 정확하게 shadeneararmies 받아오기
	Units ShadeNearEnemies;
	Units ShadeNearArmies;

	///////////////////////////////////////

	
	////////////////////////////////////////////

	for (const auto& unit : Observers)
	{
		
		/*Units NearbyArmies = observation->GetUnits(Unit::Alliance::Enemy, IsNearbyArmies(observation, unit->pos, 25));

		Point2D enemy_position;
		Point2D retreat_position;

		GetPosition(NearbyArmies, Unit::Alliance::Enemy, enemy_position);
		GetPosition(my_army, Unit::Alliance::Self, retreat_position); // TODO : 러쉬하는 유닛들로만 지정


		if (NearbyArmies.empty())
		{
			RetreatSmart(unit, retreat_position);
		}
		else
		{
			sc2::Point2D KitingLocation = retreat_position;
			KitingLocation += CalcKitingPosition(enemy_position, retreat_position * 5.0f);

			RetreatSmart(unit, KitingLocation);
		}*/
	}

	for (const auto& unit : WarpPrisms)
	{
		Units NearbyArmies = FindUnitsNear(unit, 25, Unit::Alliance::Enemy, IsArmy(observation));
		//Units NearbyArmies = observation->GetUnits(Unit::Alliance::Enemy, IsNearbyArmies(observation, unit->pos, 25));

		Point2D enemy_position;
		Point2D retreat_position;

		GetPosition(NearbyArmies, Unit::Alliance::Enemy, enemy_position);
		GetPosition(my_army, Unit::Alliance::Self, retreat_position); // TODO : 러쉬하는 유닛들로만 지정
		

		/*if (NearbyArmies.empty())
		{
			if (RetreatSmart(unit, retreat_position)) {}
			else if(unit->cargo_space_taken > 0)
			{
				Actions()->UnitCommand(unit, ABILITY_ID::UNLOADALLAT_WARPPRISM, unit->pos);
			}
		}
		else
		{
			sc2::Point2D KitingLocation = retreat_position;
			KitingLocation += CalcKitingPosition(enemy_position, retreat_position * 5.0f);

			if (RetreatSmart(unit, KitingLocation)) {}
			else if(unit->cargo_space_taken > 0)
			{
				Actions()->UnitCommand(unit, ABILITY_ID::UNLOADALLAT_WARPPRISM, unit->pos);
			}
		}*/

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

	Point2D meeting_spot = advance_pylon_location;
	if (timing_attack && !EnemyRush)
	{
		// attacker 세팅
		if (Attackers.empty()) {
			for (const auto& unit : rangedunits) {
				if (Distance2D(startLocation_, unit->pos) < 1000)
				{
					Attackers.push_back(unit);
					std::cout << " 소집되었습니다 *^^*";
				}
			}
		}
		// 다 모였는지 체크
		else {
			Point2D attackers_avg_pos = Point2D(0,0);
			GetPosition(Attackers, attackers_avg_pos);
			if (Distance2D(attackers_avg_pos, meeting_spot) < 20) {
				timing_attack = false;
			}
			
		}
	}

	//유닛이라면 기본적으로 해야할 행동 강령
	for (const auto& unit : rangedunits) {
		Units NearbyEnemies = FindUnitsNear(unit, 20, Unit::Alliance::Enemy);
		//Units NearbyWorkers = observation->GetUnits(Unit::Alliance::Enemy, IsNearbyWorker(observation, unit->pos, 7));
		//Units NearbyArmies = observation->GetUnits(Unit::Alliance::Enemy, IsNearbyArmies(observation, unit->pos, 10));
		//Units NearbyEnemies = observation->GetUnits(Unit::Alliance::Enemy, IsNearbyEnemies(observation, unit->pos, 20)); //각 유닛의 근처에 있는
		float TargetAttackRange = 0.0f;
		float UnitAttackRange = getAttackRangeGROUND(unit);

		// 타겟을 받아옵니다 *^^*
		const Unit * target = GetTarget(unit, NearbyEnemies);
		// 스킬은 알아서 피하시구요 *^^*

		if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_IMMORTAL)
		{
			if (EvadeEffect(unit)) {}
			else if (DefendDuty(unit)) {}
			else if (target != nullptr) // 카이팅은 항상하자
			{
				Kiting(unit, target);
			}
			else if (IsUnitInUnits(unit, Attackers)) // target이 없음
			{
				if (timing_attack == 1) { // 모이자~
					RetreatSmart(unit, meeting_spot);
				}
				else if(timing_attack == 0) { // 처들어가자~
					ScoutWithUnit(unit, observation);
				}
			}
			else if (unit->orders.empty())
			{
				Roam_randombase(unit);
			}
		}

		if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_VOIDRAY)
		{
			if (EvadeEffect(unit)) {}
			else if (DefendDuty(unit)) {}
			else if (target != nullptr) // 카이팅은 항상하자
			{
				Kiting(unit, target);
			}
			else if (IsUnitInUnits(unit, Attackers)) // target이 없음
			{
				if (timing_attack == 1) { // 모이자~
					RetreatSmart(unit, meeting_spot);
				}
				else if (timing_attack == 0) { // 처들어가자~
					ScoutWithUnit(unit, observation);
				}
			}
			else if (unit->orders.empty())
			{
				Roam_randombase(unit);
			}
		}

		if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_STALKER)
		{
			//ManageWarpBlink(unit);

			if (EvadeEffect(unit)) {}
			else if (DefendDuty(unit)) {}
			else if (target != nullptr) // 카이팅은 항상하자
			{
				if (BlinkResearched)
				{
					ManageBlink(unit, target);
				}
				Kiting(unit, target);
			}
			else if (IsUnitInUnits(unit, Attackers)) // target이 없음
			{
				if (timing_attack == 1) { // 모이자~
					RetreatSmart(unit, meeting_spot);
				}
				else if (timing_attack == 0) { // 처들어가자~
					ScoutWithUnit(unit, observation);
				}
			}
			else if (unit->orders.empty())
			{
				Roam_randombase(unit);
			}
		}


		if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_ADEPT)
		{
			Units NearbyArmies = FindUnitsNear(unit, 7, Unit::Alliance::Enemy, IsArmy(observation));
			Units NearbyWorkers = FindUnitsNear(unit, 6, Unit::Alliance::Enemy, IsWorker());


			if (num_adept >= 8)
			{
				AdeptMustAttack = true;
			}



			bool ComeOn = false;

			if (EvadeEffect(unit)) {}
			else if (DefendDuty(unit)) {}
			else if(target != nullptr)
			{
				if (getunitsDpsGROUND(NearbyArmies) > 20.0f)
				{
					AdeptPhaseShift(unit, ShadeNearArmies, NearbyArmies, ComeOn);
				}

				const Unit * Armytarget = GetTarget(unit, NearbyArmies);

				if (NearbyWorkers.size() > 0) // 근처에 적 일꾼이 있는데
				{
					const Unit * Workertarget = GetTarget(unit, NearbyWorkers);

					if (!NearbyArmies.empty()) // 수비유닛도 같이 있으면
					{
						const Unit * Armytarget = GetTarget(unit, NearbyArmies);
						DistanceKiting(unit, Workertarget, Armytarget);
					}
					else //일꾼만 있으면
					{
						FrontKiting(unit, Workertarget);
					}
				}
				else if (ComeOn && Armytarget != nullptr) //분신이 있으며 근처에 적 유닛이 있는 경우
				{
					ComeOnKiting(unit, Armytarget);
				}
				else //적의 DPS가 높지 않을 때
				{
					Kiting(unit, target);
				}
			}
			else if (32 <= stage_number && stage_number < 40) //AdeptMustAttack) // target이 없음
			{
				ScoutWithUnit(unit, observation);
			}
			else if (unit->orders.empty())
			{
				RetreatSmart(unit, advance_pylon_location);
				//Roam_randombase(unit);
			}


		}

		if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_COLOSSUS)
		{
			const Unit * target = GetNearTarget(unit, NearbyEnemies);

			if (EvadeEffect(unit)) {}
			else if (DefendDuty(unit)) {}
			else if (target != nullptr) // 카이팅은 항상하자
			{
				ColossusKiting(unit, target);
			}
			else if (IsUnitInUnits(unit, Attackers)) // target이 없음
			{
				if (timing_attack == 1) { // 모이자~
					RetreatSmart(unit, meeting_spot);
				}
				else if (timing_attack == 0) { // 처들어가자~
					ScoutWithUnit(unit, observation);
				}
			}
			else if (unit->orders.empty())
			{
				Roam_randombase(unit);
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

void  MEMIBot::Roam_randombase(const Unit* unit)
{
	const ObservationInterface* observation = Observation();
	Units bases = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));

	bool good_position = false;
	if (!bases.empty())
	{
		const Unit * randombase;
		GetRandomUnit(randombase, observation, bases);
		Point2D mp = randombase->pos;

		float rx = GetRandomScalar();
		float ry = GetRandomScalar();
		int roam_radius = 10;
		if (Distance2D(mp, startLocation_) < 5)
		{
			roam_radius = 15;
		}

		Point2D RoamPosition = Point2D(mp.x + rx * roam_radius, mp.y + ry * roam_radius);
		
		if (!Observation()->IsPathable(RoamPosition)) // 이동할 위치가 지상유닛이 갈 수 없는 곳이라면
		{
			return;
		}
		SmartMove(unit, RoamPosition);
	}
}

void MEMIBot::AdeptPhaseShift(const Unit* unit, Units ShadeNearEnemies , Units NearbyEnemies, bool & ComeOn)
{
	bool Timer = false;

	// nullpointer
	const Unit * EnemyExpansionMineral = FindNearestMineralPatch(enemy_expansion);
	const Unit * EnemyBaseMineral = FindNearestMineralPatch(game_info_.enemy_start_locations.front());
	if (EnemyExpansionMineral == nullptr || EnemyBaseMineral == nullptr) return;

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
		HarassLocation += KitingLocation * 1.0f;

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
	Units NearbyArmies = FindUnitsNear(unit, 30, Unit::Alliance::Enemy, IsArmy(Observation()));
	Units NearMyArmies = FindUnitsNear(unit, 15, Unit::Alliance::Self, IsArmy(Observation()));


	//Units NearbyArmies = Observation()->GetUnits(Unit::Alliance::Enemy, IsNearbyArmies(Observation(), unit->pos, 30));
	//Units NearMyArmies = Observation()->GetUnits(Unit::Alliance::Self, IsNearbyArmies(Observation(), unit->pos, 15));

	float UnitHealth = unit->health + unit->shield;

	if (UnitHealth < 60)
	{
		StalkerBlinkEscape(unit, target);
	}
	if (getunitsDpsGROUND(NearbyArmies) < 7.0f + NearMyArmies.size() * 1.0f)
	{
		StalkerBlinkForward(unit, target);
	}
}

void MEMIBot::StalkerBlinkEscape(const Unit* unit , const Unit* enemyarmy)
{
	Point2D BlinkLocation = unit->pos;
	Point2D KitingLocation = CalcKitingPosition(unit->pos, enemyarmy->pos);
	BlinkLocation += KitingLocation * 7.0f;

	AvailableAbilities abilities = Query()->GetAbilitiesForUnit(unit);
	for (const auto& ability : abilities.abilities) {
		if (ability.ability_id == ABILITY_ID::EFFECT_BLINK) {
			Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_BLINK, BlinkLocation);
		}
	}
}

void MEMIBot::StalkerBlinkForward(const Unit* unit, const Unit* enemyarmy)
{
	Point2D BlinkLocation = unit->pos;
	Point2D KitingLocation = CalcKitingPosition(unit->pos, enemyarmy->pos);
	BlinkLocation -= KitingLocation * 7.0f;

	AvailableAbilities abilities = Query()->GetAbilitiesForUnit(unit);
	for (const auto& ability : abilities.abilities) {
		if (ability.ability_id == ABILITY_ID::EFFECT_BLINK) {
			Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_BLINK, BlinkLocation);
		}
	}
}


bool MEMIBot::GetPosition(Units& units, Point2D& position) {
	if (units.empty()) {
		return false;
	}

	position = Point2D(0.0f, 0.0f);
	size_t count = 0;

	for (const auto& u : units) {
		if (u->is_alive) {
			count++;
			position += u->pos;
		}
	}

	if (!count) return false;

	position /= static_cast<float>(count);

	return true;
}


bool MEMIBot::GetPosition(UNIT_TYPEID unit_type, Unit::Alliance alliance, Point2D& position) {
	const ObservationInterface* observation = Observation();
	Units units = observation->GetUnits(alliance, IsUnit(unit_type));

	if (units.empty()) {
		return false;
	}

	position = Point2D(0.0f, 0.0f);
	size_t count = units.size();

	for (const auto& u : units) {
		position += u->pos;
	}

	position /= static_cast<float>(count);

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
			SmartAttackUnit(unit, enemy_army.front());
		}
		else {
			SmartMove(unit, KitingLocation);
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
		if (unit.unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_INTERCEPTOR)
		{
			return 0;
		}
		return 10;
	}
	if (unit.unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_MEDIVAC)
	{
		return 11;
	}
	if (unit.unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_PHOTONCANNON || unit.unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_SPINECRAWLER)
	{
		return 10;
	}
	if (IsWorker()(unit))
	{
		return 10;
	}
	if (unit.build_progress != 1.0)
	{
		return 5;
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
		if (targetUnit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_INTERCEPTOR)
		{
			continue;
		}
		const float range = getAttackRangeGROUND(rangedUnit); //rangedUnit->getAttackRange(targetUnit); 사도 사거리 4
		int priority = getAttackPriority(targetUnit);
		float target_dps = getDpsGROUND(targetUnit);

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
				closestDist = distance;
				highPriorityFar = priority;
				closestTargetOutsideRange = targetUnit;
			}
		}
		else
		{
			if (!weakestTargetInsideRange || (priority > highPriorityNear) || (priority == highPriorityNear && targetUnit->health < lowestHealth)) // 건물을 때리고 싶을 때는
			{
				lowestHealth = targetUnit->health + targetUnit->shield;
				highPriorityNear = priority;
				weakestTargetInsideRange = targetUnit;

			}
		}
	}
	return weakestTargetInsideRange && highPriorityNear>1 ? weakestTargetInsideRange : closestTargetOutsideRange;
}

 const Unit * MEMIBot::GetNearTarget(const Unit * rangedUnit, Units & targets)
 {

	 int highPriorityFar = 0;
	 int highPriorityNear = 0;
	 int highDpsNear = 0;
	 double closestDist = std::numeric_limits<double>::max();
	 double lowestHealth = std::numeric_limits<double>::max();
	 const Unit * closestTargetOutsideRange = nullptr;
	 const Unit * closestTargetInsideRange = nullptr;

	 for (const auto & targetUnit : targets)
	 {
		 if (!targetUnit->is_alive)
		 {
			 continue;
		 }
		 const float range = getAttackRangeGROUND(rangedUnit); //rangedUnit->getAttackRange(targetUnit); 사도 사거리 4
		 int priority = getAttackPriority(targetUnit);
		 float target_dps = getDpsGROUND(targetUnit);

		 const float distance = Distance2D(rangedUnit->pos, targetUnit->pos);
		 float DIST = distance - rangedUnit->radius - targetUnit->radius;

		 if (DIST > range) //적과 나 사이의 거리 > 나의 사정거리 // 거리가 멀어서 때릴 수 없는 경우
		 {
			 // If in sight we just add 20 to prio. This should make sure that a unit in sight has higher priority than any unit outside of range
			 //
			 float SightRange = Observation()->GetUnitTypeData()[rangedUnit->unit_type].sight_range;

			 if (DIST <= SightRange)
			 {
				 priority += 20;
			 }
			 // 우선순위가 높거나 또는 거리가 가까우면 설정한다
			 if (!closestTargetOutsideRange || (priority > highPriorityFar) || (priority == highPriorityFar && DIST < closestDist))
			 {
				 closestDist = DIST;
				 highPriorityFar = priority;
				 closestTargetOutsideRange = targetUnit;
			 }
		 }
		 else
		 {
			 if (!closestTargetInsideRange || (priority > highPriorityNear) || (priority == highPriorityNear && DIST < closestDist)) // 건물을 때리고 싶을 때는
			 {
				 closestDist = DIST;
				 highPriorityNear = priority;
				 closestTargetInsideRange = targetUnit;

			 }
		 }
	 }
	 return closestTargetInsideRange && highPriorityNear>1 ? closestTargetInsideRange : closestTargetOutsideRange;
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
		if (target->unit_type.ToType() == UNIT_TYPEID::PROTOSS_COLOSSUS && ColossusRangeUp == true)
		{
			return 9.0f;
		}
		if (target->unit_type.ToType() == UNIT_TYPEID::PROTOSS_CARRIER)
		{
			return 8.0f;
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
	//return AirWeapons.range; // 7.5 이건 오로지 공중유닛을 위한 함수!!
	return groundWeapons.range; // 7.5 지상유닛을 위한 함수
}

