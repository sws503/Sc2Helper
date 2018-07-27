#include "memibot.h"
bool MEMIBot::OracleCanWin(const Unit* Oracle , Units enemyunits, bool OracleCanAttack)
{
	const ObservationInterface* observation = Observation();
	//Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy); 
	
	float OracleHP = Oracle->health + Oracle->shield;
	float TotalEnemyHP = 0.1f;
	float TotalDPS = 0.1f;
	float AirRange = 0.0f;
	float dps = 0.0f;

	//TODO 경장갑과 무장갑이 섞여있는 경우 생각하기 & 방어력 생각하기
	float gap = !OracleCanAttack * 25;
	float OracleDPS = 15 * 1.1;
	float OracleDeal = (Oracle->energy - gap) / 2 * OracleDPS;

	float EnemyHP = 0.0f;

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
			}
		}
		else //적이 공중공격을 할수 없음  > 무조건 내가 이김
		{
			dps = 0.0;
		}
		TotalDPS += dps;
	}
#ifdef DEBUG
	std::cout << TotalEnemyHP << "<" << OracleDeal << (TotalEnemyHP < OracleDeal) << std::endl;

	std::cout << TotalEnemyHP << "/" << OracleDPS << "<" <<  OracleHP << "/" << TotalDPS << "=" << ((TotalEnemyHP / OracleDPS) < (OracleHP / TotalDPS)) << std::endl;
#endif


	if (TotalEnemyHP < OracleDeal) // 일단 적을 죽일만큼 딜이 나올 경우
		if (TotalEnemyHP / OracleDPS < OracleHP / TotalDPS) // 내가 적보다 빨리 죽일 수 있으면
		{
			//Chat("I can fight");
			return true;
		}
		else
		{
			//Chat("I can't win");
			return false; // false
		}
	else
	{
		//Chat("I can kill no one");
		return false; // false
	}
}

bool mustattack = false;

void MEMIBot::ManageRush() { // 5.17 오라클 유닛 관리 +6.25 폭풍함 유닛 관리

	const float TempestRange = 3.0f;
	const float CarrierRange = 3.0f;

	const ObservationInterface* observation = Observation();
	Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
	Units enemy_army = observation->GetUnits(Unit::Alliance::Enemy, IsArmy(observation));
	Units Oracles = observation->GetUnits(Unit::Alliance::Self, IsOracle());
	Units Tempests = observation->GetUnits(Unit::Alliance::Self, IsTempest());
	Units Voidrays = observation->GetUnits(Unit::Alliance::Self, IsVoidray());
	Units Carriers = observation->GetUnits(Unit::Alliance::Self, IsCarrier());
	Units EnemyWorker = observation->GetUnits(Unit::Alliance::Enemy, IsWorker());
	Units AirAttackers = observation->GetUnits(Unit::Alliance::Enemy, AirAttacker()); //적 방어 유닛 및 건물
	Units AirAttackers2 = observation->GetUnits(Unit::Alliance::Enemy, AirAttacker2());
	//Units ProxyEnemy = observation->GetUnits(Unit::Alliance::Enemy, ExceptBuilding());
	float rx = GetRandomScalar();
	float ry = GetRandomScalar();

	

	for (const auto& unit : Oracles) {
		bool OracleFight = false; //false

		if (unit == oracle_second)
			continue;
		bool OracleCanAttack = false;


		if (!unit->orders.empty()) { // 펄서광선  ON / OFF
			float distance = std::numeric_limits<float>::max();



			for (const auto& u : enemy_army) {
				float d = Distance2D(u->pos, unit->pos);
				if (d < distance) {
					distance = d;
				}
			}
			if (unit->energy == 1)
				OracleCanAttack = false;
			else if (distance < 6 && unit->energy >= 45) {
				Actions()->UnitCommand(unit, ABILITY_ID::BEHAVIOR_PULSARBEAMON);
				OracleCanAttack = true;
			}
			else if (distance < 10 && unit->energy >= 40 || OracleFight) {
				Actions()->UnitCommand(unit, ABILITY_ID::BEHAVIOR_PULSARBEAMON);
				OracleCanAttack = true;
			}
			else if (distance > 20) {
				Actions()->UnitCommand(unit, ABILITY_ID::BEHAVIOR_PULSARBEAMOFF);
				OracleCanAttack = false;
			}
		}

		if (find_enemy_location == 1) {
			if (enemy_units.empty()) {
				Actions()->UnitCommand(unit, ABILITY_ID::MOVE, game_info_.enemy_start_locations.front());
			}
			
		}

		bool evade = false;

		float distance = std::numeric_limits<float>::max(); // 5.21 방어 건물,유닛이 근처로 가지 않는다
		float UnitAttackRange = getAttackRange(unit); // 7.3 이 유닛의 공격사정거리
		float TargetAttackRange = 0.0f; // 7.3 나를 공격할 수 있는 유닛의 공격 사정거리

		for (const auto& u : AirAttackers2) {
			float d = Distance2D(u->pos, unit->pos);
			if (d < distance) {
				distance = d; // 가장 가까운 거리의 적을 고른다
			}

			float TargetAttackRange = getAttackRange(u);

			Vector2D diff = unit->pos - u->pos; // 7.3 적 유닛과의 반대 방향으로 도망
			Normalize2D(diff);
			KitingLocation = unit->pos + diff * 7.0f;

			float add = OracleRange;

			if (u->unit_type == sc2::UNIT_TYPEID::PROTOSS_PHOTONCANNON || u->unit_type == sc2::UNIT_TYPEID::ZERG_SPORECRAWLER || u->unit_type == sc2::UNIT_TYPEID::TERRAN_MISSILETURRET)
			{
				add = 1.0f;
			}

			// 공격해야되는지 피해야 되는지 판단.
			// 내가 공격할 수 있고 적 사거리보다 멀리 있을 때 공격한다
			// 7.5 OracleRange 는 안전거리 (적도 나를 향해 움직이기 때문)
			if (distance <= TargetAttackRange + add) {
				evade = true;
				break;
			}
		}

		//적들을 내가 이길수 있는지 판단
		if (!OracleCanWin(unit, AirAttackers2, OracleCanAttack)) // 이길수없으면 아예 덤비지말고
		{
			OracleFight = false;
			//Chat("OMG! It's too scary..@@@@@@@@@@@@@@@");
		}
		/*if (!OracleCanAttack && unit->energy < 60) // 위협적인 적이 아닌데 펄서광선 켤 에너지가 모자라면 덤비지말고
		{
			OracleFight = false;
			Chat("Not enough energy!!!!!!!!!!!!!!!!!!!");
		}*/
		else // 위협적인 적이 아닐때 에너지도 충분하면 덤비자
		{
			OracleFight = true;
			//Chat("Come on~ Let's fight#################");
		}

		// 적 유닛 피하기
		// 펄서빔이 꺼져있을 땐 
		if (!OracleCanAttack) {				
			//이길수 없는 적이 있으면 뒤로 도망간다
			if (!OracleFight && evade)
			{
				Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
				Chat("It's dangerous EVADE!!");
			}
			//이길 수 있는 적이 있으면 가까이 다가간다
			else
				ScoutWithUnit(unit, observation);
		}

		// 스킬 피하기
		else if (EvadeEffect(unit)) {}

		//펄서빔이 켜져있을 때 
		else if (OracleCanAttack)
		{
			//이길 수 없는 적 유닛이 가까이 오면
			if (!OracleFight && evade)
			{
				//도망간다
				Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
				Chat("It's dangerous EVADE!!");
			}
			//반대로 이길수 있는 상황에선
			else
			{
				//나를 공격하러 오면
				if (evade)
				{
					//그 유닛을 죽인다
					Chat("Come on~ fight");
					for (const auto& Proxy1 : AirAttackers2) {
						if (!Proxy1->is_alive) continue;
						Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, Proxy1);
						break;
					}
				}
				//위협이 없는 상황에선
				else
				{
					//일꾼을 우선 죽인다
					for (const auto& Proxy1 : EnemyWorker) {
						if (!Proxy1->is_alive) continue;
						Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, Proxy1);
						Chat("Worker Attack!");
						break;
					}
				}
			}
		}
#ifdef DEBUG
		if (OracleFight) std::cout << "Fight";
		else std::cout << "No Fight";
#endif
	}
	for (const auto& unit : Voidrays) {
		float distance = std::numeric_limits<float>::max(); // 6.25 공허포격기 거리유지
		float UnitAttackRange = getAttackRange(unit); // 7.3 이 유닛의 공격사정거리
		float TargetAttackRange = 0.0f; // 7.3 나를 공격할 수 있는 유닛의 공격 사정거리

		if (EvadeEffect(unit)) continue;

		bool enemiesnear = false;

		bool IsArmored = false;
		for (const auto& u : AirAttackers) {


			if (!u->is_alive)
			{
				continue;
			}

			float d = Distance2D(u->pos, unit->pos);
			if (d < distance) {
				distance = d;
			}

			float TargetAttackRange = getAttackRange(u);

			Vector2D diff = unit->pos - u->pos; // 7.3 적 유닛과의 반대 방향으로 도망
			Normalize2D(diff);
			KitingLocation = unit->pos + diff * 7.0f;

			if (distance < 6) // 공허포격기는 되도록 최대 사거리(=6)에서 공격한다
			{
				for (const auto & Attribute : Observation()->GetUnitTypeData()[u->unit_type].attributes)
				{
					if (Attribute == Attribute::Armored)
					{
						IsArmored = true;
					}
				}


				enemiesnear = true;
				break;
			}
		}

		// 내가 공격할 수 있고 적 사거리보다 멀리 있을 때 공격한다
		if (unit->weapon_cooldown == 0.0f || !enemiesnear) {
			if(IsArmored)
				Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_VOIDRAYPRISMATICALIGNMENT); // 적 유닛 중 중장갑 유닛이 있으면 분광정렬 사용
			if(EnemyRush)
				RetreatWithVoidray(unit);

			if (unit->orders.empty())
				RetreatWithCarrier(unit);
		}
		// 가까우면 도망간다.
		else {
			Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
		}


	}


	for (const auto& unit : Tempests) {
		float distance = std::numeric_limits<float>::max(); // 6.25 폭풍함은 사거리를 활용해 방어 건물,유닛 근처로 가지 않는다
		float UnitAttackRange = getAttackRange(unit); // 7.3 이 유닛의 공격사정거리
		float TargetAttackRange = 0.0f; // 7.3 나를 공격할 수 있는 유닛의 공격 사정거리

		if (EvadeEffect(unit)) continue;

		for (const auto& u : AirAttackers) {
			float d = Distance2D(u->pos, unit->pos);
			if (d < distance) {
				distance = d;
			}

			float TargetAttackRange = getAttackRange(u);

			Vector2D diff = unit->pos - u->pos; // 7.3 적 유닛과의 반대 방향으로 도망
			Normalize2D(diff);
			KitingLocation = unit->pos + diff * 7.0f;

			if (unit->weapon_cooldown == 0.0f || TargetAttackRange + TempestRange < distance) // 내가 공격할 수 있고 적 사거리보다 멀리 있을 때 공격한다
			{

				for (const auto& Proxy2 : AirAttackers) {
					//Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
					Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, AirAttackers.front()->pos);

					break; // 타겟할 유닛을 찾고 찾으면 공격하는 걸로
				}

			}
			else if (distance <= TargetAttackRange + TempestRange) {
				Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
				break;
			}
			else
			{
				for (const auto& Proxy1 : EnemyWorker) {
					//Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
					Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, Proxy1->pos);

					break;
				}
			}
		}
	}

	size_t CurrentCarrier = CountUnitType(UNIT_TYPEID::PROTOSS_CARRIER);

	if (CurrentCarrier <= 3) {
		OracleRange = 3.0f;
	}
	else {
		OracleRange = 4.0f;
		TimetoAttack = true;
	}

	for (const auto& unit : Carriers) {
		float distance = std::numeric_limits<float>::max(); // 6.25 캐리어 거리유지
		float UnitAttackRange = getAttackRange(unit); // 7.3 이 유닛의 공격사정거리
		float TargetAttackRange = 0.0f; // 7.3 나를 공격할 수 있는 유닛의 공격 사정거리

		if (EvadeEffect(unit)) continue;

		bool enemiesnear = false;
		for (const auto& u : AirAttackers) {
			if (!u->is_alive)
			{
				continue;
			}

			float d = Distance2D(u->pos, unit->pos);
			if (d < distance) {
				distance = d;
			}

			float TargetAttackRange = getAttackRange(u);

			Vector2D diff = unit->pos - u->pos; // 7.3 적 유닛과의 반대 방향으로 도망
			Normalize2D(diff);
			KitingLocation = unit->pos + diff * 7.0f;

			float RealCarrierRange = (unit->shield < 10) ? 7.5f : CarrierRange;

			// 하나라도 가까이 있는 경우.
			if (distance <= TargetAttackRange + RealCarrierRange) // 내가 공격할 수 있고 적 사거리보다 멀리 있을 때 공격한다
			{
				enemiesnear = true;
				break;
			}
		}

		
		if (CurrentCarrier < 10 && !mustattack) {
			// 내가 공격할 수 있고 적 사거리보다 멀리 있을 때 공격한다
			if (unit->weapon_cooldown == 0.0f || !enemiesnear) {
				if (unit->orders.empty())
					RetreatWithCarrier(unit);
			}
			// 가까우면 도망간다.
			else {
				Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
			}
		}
		else if (CurrentCarrier < 4 && mustattack) {
			mustattack = false;
			if (unit->weapon_cooldown == 0.0f || !enemiesnear) {
				if (unit->orders.empty())
					RetreatWithCarrier(unit);
			}
			// 가까우면 도망간다.
			else {
				Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
			}
		}

		else { //if (CurrentCarrier >= 10)
			mustattack = true;

			if (AirAttackers.empty())
			{
				AttackWithUnit(unit, observation);
			}
			else if (!enemy_units.empty()) {
				// 내가 공격할 수 있고 적 사거리보다 멀리 있을 때 공격한다
				if (unit->weapon_cooldown == 0.0f || !enemiesnear) {
					Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, AirAttackers.front()->pos);
				}
				// 가까우면 도망간다.
				else {
					Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
				}
			}
			else // 지도상에 적 유닛이 아예 없는 상황에선 캐리어가 주도적으로 탐색해야함
			{
				ScoutWithUnit(unit, observation);
				scoutprobe();
			}
		}
	}
}