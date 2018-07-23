#include "memibot.h"

void MEMIBot::ManageRush() { // 5.17 ����Ŭ ���� ���� +6.25 ��ǳ�� ���� ����
	const ObservationInterface* observation = Observation();
	Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
	Units Oracles = observation->GetUnits(Unit::Alliance::Self, IsOracle());
	Units Tempests = observation->GetUnits(Unit::Alliance::Self, IsTempest()); //6.25 ��ǳ�� ��Ʈ�� �߰�
	Units Carriers = observation->GetUnits(Unit::Alliance::Self, IsCarrier());
	Units EnemyWorker = observation->GetUnits(Unit::Alliance::Enemy, IsWorker());
	Units AirAttackers = observation->GetUnits(Unit::Alliance::Enemy, AirAttacker()); //�� ��� ���� �� �ǹ�
	Units AirAttackers2 = observation->GetUnits(Unit::Alliance::Enemy, AirAttacker2());
	//Units ProxyEnemy = observation->GetUnits(Unit::Alliance::Enemy, ExceptBuilding());
	float rx = GetRandomScalar();
	float ry = GetRandomScalar();

	for (const auto& unit : Oracles) {
		if (unit == oracle_second)
			continue;
		bool OracleCanAttack = false;
		if (!unit->orders.empty()) { // �޼�����  ON / OFF
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

		float distance = std::numeric_limits<float>::max(); // 5.21 ��� �ǹ�,������ ��ó�� ���� �ʴ´�
		float UnitAttackRange = getAttackRange(unit); // 7.3 �� ������ ���ݻ����Ÿ�
		float TargetAttackRange = 0.0f; // 7.3 ���� ������ �� �ִ� ������ ���� �����Ÿ�

		for (const auto& u : AirAttackers2) {
			float d = Distance2D(u->pos, unit->pos);
			if (d < distance) {
				distance = d; // ���� ����� �Ÿ��� ���� ����
			}

			float TargetAttackRange = getAttackRange(u);

			Vector2D diff = unit->pos - u->pos; // 7.3 �� ���ְ��� �ݴ� �������� ����
			Normalize2D(diff);
			KitingLocation = unit->pos + diff * 7.0f;

			float add = OracleRange;

			if (u->unit_type == sc2::UNIT_TYPEID::PROTOSS_PHOTONCANNON || u->unit_type == sc2::UNIT_TYPEID::ZERG_SPORECRAWLER || u->unit_type == sc2::UNIT_TYPEID::TERRAN_MISSILETURRET)
			{
				add = 5.0f;
			}

			// �����ؾߵǴ��� ���ؾ� �Ǵ��� �Ǵ�.
			// ���� ������ �� �ְ� �� ��Ÿ����� �ָ� ���� �� �����Ѵ�
			// 7.5 OracleRange �� �����Ÿ� (���� ���� ���� �����̱� ����)
			if (distance <= TargetAttackRange + add) {
				evade = true;
				break;
			}
		}
		// �� ���� ���ϱ�
		if (evade) {
			Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
		}
		// ��ų ���ϱ�
		else if (EvadeEffect(unit)) {}
		// �� �ϲ� ����
		else
		{
			for (const auto& Proxy1 : EnemyWorker) {
				//Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
				Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, Proxy1);
				//Chat("Target Attack!"); 7.6 �ʹ� �ò����� ����
				break;
			}
		}

		/*
		if (distance < 11) {
		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation); // 5.21 ��¦��¦ ��°� �����ϰ� �ʹ� // 5.24 ������ �ȵ�
		} //6.25 ������
		if (!EnemyWorker.empty() && OracleCanAttack && distance > 10.5) { //6.26 ��
		for (const auto& Proxy1 : EnemyWorker) {
		//Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
		Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, Proxy1->pos);
		Chat("Target Attack!");
		}
		}
		*/
		/*
		else if (!ProxyEnemy.empty() && OracleCanAttack && distance > 10.5 && distance < 20) { //6.28 �����ڰ� �ǹ������°� ���ʿ��ϴ�
		for (const auto& Proxy2 : ProxyEnemy) {
		Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, ProxyEnemy.front()->pos);
		}
		}
		*/

		//}
	}

	for (const auto& unit : Tempests) {
		float distance = std::numeric_limits<float>::max(); // 6.25 ��ǳ���� ��Ÿ��� Ȱ���� ��� �ǹ�,���� ��ó�� ���� �ʴ´�
		float UnitAttackRange = getAttackRange(unit); // 7.3 �� ������ ���ݻ����Ÿ�
		float TargetAttackRange = 0.0f; // 7.3 ���� ������ �� �ִ� ������ ���� �����Ÿ�

		if (EvadeEffect(unit)) continue;

		for (const auto& u : AirAttackers) {
			float d = Distance2D(u->pos, unit->pos);
			if (d < distance) {
				distance = d;
			}

			float TargetAttackRange = getAttackRange(u);

			Vector2D diff = unit->pos - u->pos; // 7.3 �� ���ְ��� �ݴ� �������� ����
			Normalize2D(diff);
			KitingLocation = unit->pos + diff * 7.0f;

			if (unit->weapon_cooldown == 0.0f || TargetAttackRange + TempestRange < distance) // ���� ������ �� �ְ� �� ��Ÿ����� �ָ� ���� �� �����Ѵ�
			{

				for (const auto& Proxy2 : AirAttackers) {
					//Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
					Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, AirAttackers.front()->pos);

					break; // Ÿ���� ������ ã�� ã���� �����ϴ� �ɷ�
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
		if (!AirAttackers.empty() && distance > 10) { //6.26 ��
		for (const auto& Proxy2 : AirAttackers) {
		Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, AirAttackers.front()->pos);
		}
		}
		else if (!EnemyWorker.empty() && distance > 10 && distance < 20) { //6.26 ��
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
		float distance = std::numeric_limits<float>::max(); // 6.25 ĳ���� �Ÿ�����
		float UnitAttackRange = getAttackRange(unit); // 7.3 �� ������ ���ݻ����Ÿ�
		float TargetAttackRange = 0.0f; // 7.3 ���� ������ �� �ִ� ������ ���� �����Ÿ�

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

			Vector2D diff = unit->pos - u->pos; // 7.3 �� ���ְ��� �ݴ� �������� ����
			Normalize2D(diff);
			KitingLocation = unit->pos + diff * 7.0f;

			float RealCarrierRange = (unit->shield < 10) ? 6.0f : CarrierRange;

			// �ϳ��� ������ �ִ� ���.
			if (distance <= TargetAttackRange + RealCarrierRange) // ���� ������ �� �ְ� �� ��Ÿ����� �ָ� ���� �� �����Ѵ�
			{
				enemiesnear = true;
				break;
			}
		}


		if (CurrentCarrier <= 3) {

			// ���� ������ �� �ְ� �� ��Ÿ����� �ָ� ���� �� �����Ѵ�
			if (unit->weapon_cooldown == 0.0f || !enemiesnear) {
				if (unit->orders.empty())
					RetreatWithCarrier(unit);
			}
			// ������ ��������.
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
				// ���� ������ �� �ְ� �� ��Ÿ����� �ָ� ���� �� �����Ѵ�
				if (unit->weapon_cooldown == 0.0f || !enemiesnear) {
					Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, AirAttackers.front()->pos);
				}
				// ������ ��������.
				else {
					Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
				}
			}
			else // ������ �� ������ �ƿ� ���� ��Ȳ���� ĳ��� �ֵ������� Ž���ؾ���
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