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

	//TODO ���尩�� ���尩�� �����ִ� ��� �����ϱ� & ���� �����ϱ�
	float gap = !OracleCanAttack * 25;
	float OracleDPS = 15 * 1.1;
	float OracleDeal = (Oracle->energy - gap) / 2 * OracleDPS;

	float EnemyHP = 0.0f;

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
			}
		}
		else //���� ���߰����� �Ҽ� ����  > ������ ���� �̱�
		{
			dps = 0.0;
		}
		TotalDPS += dps;
	}
#ifdef DEBUG
	std::cout << TotalEnemyHP << "<" << OracleDeal << (TotalEnemyHP < OracleDeal) << std::endl;

	std::cout << TotalEnemyHP << "/" << OracleDPS << "<" <<  OracleHP << "/" << TotalDPS << "=" << ((TotalEnemyHP / OracleDPS) < (OracleHP / TotalDPS)) << std::endl;
#endif


	if (TotalEnemyHP < OracleDeal) // �ϴ� ���� ���ϸ�ŭ ���� ���� ���
		if (TotalEnemyHP / OracleDPS < OracleHP / TotalDPS) // ���� ������ ���� ���� �� ������
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

void MEMIBot::ManageRush() { // 5.17 ����Ŭ ���� ���� +6.25 ��ǳ�� ���� ����

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
	Units AirAttackers = observation->GetUnits(Unit::Alliance::Enemy, AirAttacker()); //�� ��� ���� �� �ǹ�
	Units AirAttackers2 = observation->GetUnits(Unit::Alliance::Enemy, AirAttacker2());
	//Units ProxyEnemy = observation->GetUnits(Unit::Alliance::Enemy, ExceptBuilding());
	float rx = GetRandomScalar();
	float ry = GetRandomScalar();

	

	for (const auto& unit : Oracles) {
		bool OracleFight = false; //false

		if (unit == oracle_second)
			continue;
		bool OracleCanAttack = false;


		if (!unit->orders.empty()) { // �޼�����  ON / OFF
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
				add = 1.0f;
			}

			// �����ؾߵǴ��� ���ؾ� �Ǵ��� �Ǵ�.
			// ���� ������ �� �ְ� �� ��Ÿ����� �ָ� ���� �� �����Ѵ�
			// 7.5 OracleRange �� �����Ÿ� (���� ���� ���� �����̱� ����)
			if (distance <= TargetAttackRange + add) {
				evade = true;
				break;
			}
		}

		//������ ���� �̱�� �ִ��� �Ǵ�
		if (!OracleCanWin(unit, AirAttackers2, OracleCanAttack)) // �̱�������� �ƿ� ����������
		{
			OracleFight = false;
			//Chat("OMG! It's too scary..@@@@@@@@@@@@@@@");
		}
		/*if (!OracleCanAttack && unit->energy < 60) // �������� ���� �ƴѵ� �޼����� �� �������� ���ڶ�� ����������
		{
			OracleFight = false;
			Chat("Not enough energy!!!!!!!!!!!!!!!!!!!");
		}*/
		else // �������� ���� �ƴҶ� �������� ����ϸ� ������
		{
			OracleFight = true;
			//Chat("Come on~ Let's fight#################");
		}

		// �� ���� ���ϱ�
		// �޼����� �������� �� 
		if (!OracleCanAttack) {				
			//�̱�� ���� ���� ������ �ڷ� ��������
			if (!OracleFight && evade)
			{
				Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
				Chat("It's dangerous EVADE!!");
			}
			//�̱� �� �ִ� ���� ������ ������ �ٰ�����
			else
				ScoutWithUnit(unit, observation);
		}

		// ��ų ���ϱ�
		else if (EvadeEffect(unit)) {}

		//�޼����� �������� �� 
		else if (OracleCanAttack)
		{
			//�̱� �� ���� �� ������ ������ ����
			if (!OracleFight && evade)
			{
				//��������
				Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
				Chat("It's dangerous EVADE!!");
			}
			//�ݴ�� �̱�� �ִ� ��Ȳ����
			else
			{
				//���� �����Ϸ� ����
				if (evade)
				{
					//�� ������ ���δ�
					Chat("Come on~ fight");
					for (const auto& Proxy1 : AirAttackers2) {
						if (!Proxy1->is_alive) continue;
						Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, Proxy1);
						break;
					}
				}
				//������ ���� ��Ȳ����
				else
				{
					//�ϲ��� �켱 ���δ�
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
		float distance = std::numeric_limits<float>::max(); // 6.25 �������ݱ� �Ÿ�����
		float UnitAttackRange = getAttackRange(unit); // 7.3 �� ������ ���ݻ����Ÿ�
		float TargetAttackRange = 0.0f; // 7.3 ���� ������ �� �ִ� ������ ���� �����Ÿ�

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

			Vector2D diff = unit->pos - u->pos; // 7.3 �� ���ְ��� �ݴ� �������� ����
			Normalize2D(diff);
			KitingLocation = unit->pos + diff * 7.0f;

			if (distance < 6) // �������ݱ�� �ǵ��� �ִ� ��Ÿ�(=6)���� �����Ѵ�
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

		// ���� ������ �� �ְ� �� ��Ÿ����� �ָ� ���� �� �����Ѵ�
		if (unit->weapon_cooldown == 0.0f || !enemiesnear) {
			if(IsArmored)
				Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_VOIDRAYPRISMATICALIGNMENT); // �� ���� �� ���尩 ������ ������ �б����� ���
			if(EnemyRush)
				RetreatWithVoidray(unit);

			if (unit->orders.empty())
				RetreatWithCarrier(unit);
		}
		// ������ ��������.
		else {
			Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
		}


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

			float RealCarrierRange = (unit->shield < 10) ? 7.5f : CarrierRange;

			// �ϳ��� ������ �ִ� ���.
			if (distance <= TargetAttackRange + RealCarrierRange) // ���� ������ �� �ְ� �� ��Ÿ����� �ָ� ���� �� �����Ѵ�
			{
				enemiesnear = true;
				break;
			}
		}

		
		if (CurrentCarrier < 10 && !mustattack) {
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
		else if (CurrentCarrier < 4 && mustattack) {
			mustattack = false;
			if (unit->weapon_cooldown == 0.0f || !enemiesnear) {
				if (unit->orders.empty())
					RetreatWithCarrier(unit);
			}
			// ������ ��������.
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
	}
}