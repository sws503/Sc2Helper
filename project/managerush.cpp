#include "memibot.h"

void MEMIBot::ManageRush() { // 5.17 오라클 유닛 관리 +6.25 폭풍함 유닛 관리
	const ObservationInterface* observation = Observation();
	Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
	Units Oracles = observation->GetUnits(Unit::Alliance::Self, IsOracle());
	Units Tempests = observation->GetUnits(Unit::Alliance::Self, IsTempest()); //6.25 폭풍함 컨트롤 추가
	Units Carriers = observation->GetUnits(Unit::Alliance::Self, IsCarrier());
	Units EnemyWorker = observation->GetUnits(Unit::Alliance::Enemy, IsWorker());
	Units AirAttackers = observation->GetUnits(Unit::Alliance::Enemy, AirAttacker()); //적 방어 유닛 및 건물
	Units AirAttackers2 = observation->GetUnits(Unit::Alliance::Enemy, AirAttacker2());
	//Units ProxyEnemy = observation->GetUnits(Unit::Alliance::Enemy, ExceptBuilding());
	float rx = GetRandomScalar();
	float ry = GetRandomScalar();

	for (const auto& unit : Oracles) {
		if (unit == oracle_second)
			continue;
		bool OracleCanAttack = false;
		if (!unit->orders.empty()) { // 펄서광선  ON / OFF
			float distance = std::numeric_limits<float>::max();
			for (const auto& u : EnemyWorker) {
				float d = Distance2D(u->pos, unit->pos);
				if (d < distance) {
					distance = d;
				}
			}

			if (unit->energy == 1)
				OracleCanAttack = false;
			else if (distance < 6 && unit->energy >= 50) {
				Actions()->UnitCommand(unit, ABILITY_ID::BEHAVIOR_PULSARBEAMON);
				OracleCanAttack = true;
			}
			else if (distance > 20) {
				Actions()->UnitCommand(unit, ABILITY_ID::BEHAVIOR_PULSARBEAMOFF);
				OracleCanAttack = false;
			}
		}

		if (find_enemy_location == 1) {
			if (unit->energy > 50) {
				Actions()->UnitCommand(unit, ABILITY_ID::MOVE, game_info_.enemy_start_locations.front());

			}
			if (unit->energy <= 50 && !OracleCanAttack) {
				ScoutWithUnit(unit, observation);
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
				add = 5.0f;
			}

			// 공격해야되는지 피해야 되는지 판단.
			// 내가 공격할 수 있고 적 사거리보다 멀리 있을 때 공격한다
			// 7.5 OracleRange 는 안전거리 (적도 나를 향해 움직이기 때문)
			if (distance <= TargetAttackRange + add) {
				evade = true;
				break;
			}
		}
		// 적 유닛 피하기
		if (evade) {
			Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
		}
		// 스킬 피하기
		else if (EvadeEffect(unit)) {}
		// 적 일꾼 공격
		else
		{
			for (const auto& Proxy1 : EnemyWorker) {
				//Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
				Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, Proxy1);
				//Chat("Target Attack!"); 7.6 너무 시끄러움 ㅋㅋ
				break;
			}
		}

		/*
		if (distance < 11) {
		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation); // 5.21 깔짝깔짝 대는걸 구현하고 싶다 // 5.24 구현이 안됨
		} //6.25 구현됨
		if (!EnemyWorker.empty() && OracleCanAttack && distance > 10.5) { //6.26 적
		for (const auto& Proxy1 : EnemyWorker) {
		//Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
		Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, Proxy1->pos);
		Chat("Target Attack!");
		}
		}
		*/
		/*
		else if (!ProxyEnemy.empty() && OracleCanAttack && distance > 10.5 && distance < 20) { //6.28 예언자가 건물때리는건 불필요하다
		for (const auto& Proxy2 : ProxyEnemy) {
		Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, ProxyEnemy.front()->pos);
		}
		}
		*/

		//}
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


		/*
		for (const auto& u : AirAttackers) {
		float d = Distance2D(u->pos, unit->pos);
		if (d < distance) {
		distance = d;
		}
		Vector2D diff = unit->pos - u->pos;
		Normalize2D(diff);
		KitingLocation = unit->pos + diff * 7.0f;
		}
		if (distance < 10) {
		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
		}
		if (!AirAttackers.empty() && distance > 10) { //6.26 적
		for (const auto& Proxy2 : AirAttackers) {
		Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, AirAttackers.front()->pos);
		}
		}
		else if (!EnemyWorker.empty() && distance > 10 && distance < 20) { //6.26 적
		for (const auto& Proxy1 : EnemyWorker) {
		Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
		}
		}*/

	}

	int CurrentCarrier = CountUnitType(observation, UNIT_TYPEID::PROTOSS_CARRIER);

	if (CurrentCarrier <= 3) {
		OracleRange = 7.0f;
	}
	else {
		OracleRange = 5.0f;
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

			float RealCarrierRange = (unit->shield < 10) ? 6.0f : CarrierRange;

			// 하나라도 가까이 있는 경우.
			if (distance <= TargetAttackRange + RealCarrierRange) // 내가 공격할 수 있고 적 사거리보다 멀리 있을 때 공격한다
			{
				enemiesnear = true;
				break;
			}
		}


		if (CurrentCarrier <= 3) {

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
		else { //if (CurrentCarrier > 3)
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
		/*
		for (const auto& u : AirAttackers) {
		float d = Distance2D(u->pos, unit->pos);
		if (d < distance) {
		distance = d;
		}
		Vector2D diff = unit->pos - u->pos;
		Normalize2D(diff);
		KitingLocation = unit->pos + diff * 7.0f;
		}
		if (distance < 10) {
		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
		}
		if (!AirAttackers.empty() && distance > 10) { //6.29
		for (const auto& Proxy2 : AirAttackers) {
		Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, AirAttackers.front()->pos);
		}
		}
		else if (!EnemyWorker.empty() && distance >= 10 && distance < 20) { //6.29
		for (const auto& Proxy1 : EnemyWorker) {
		Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
		}
		}
		*/
	}
}