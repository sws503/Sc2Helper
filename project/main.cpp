#include <sc2api/sc2_api.h>
#include "sc2lib/sc2_lib.h"
#include <sc2utils/sc2_manage_process.h>

#include <iostream>
#include <string>
#include <algorithm>
#include <random>
#include <iterator>
#include <typeinfo>
#include <map>

using namespace sc2;

// Control 시작
// 7.3 안좋은 효과들 목록
enum class EFFECT_ID
{
	INVALID = 0,
	PSISTORM = 1,
	GUARDIANSHIELD = 2,
	TEMPORALFIELDGROWING = 3,
	TEMPORALFIELD = 4,
	THERMALLANCES = 5,
	SCANNERSWEEP = 6,
	NUKEDOT = 7,
	LIBERATORMORPHING = 8,
	LIBERATORMORPHED = 9,
	BLINDINGCLOUD = 10,
	CORROSIVEBILE = 11,
	LURKERATTACK = 12
};
typedef SC2Type<EFFECT_ID>  EffectID;
// Control 끝


class Bot : public Agent {
public:
    virtual void OnGameStart() final {
        game_info_ = Observation()->GetGameInfo();
        std::cout << "Game started!" << std::endl;
        expansions_ = search::CalculateExpansionLocations(Observation(), Query());
        iter_exp = expansions_.begin();

        //Temporary, we can replace this with observation->GetStartLocation() once implemented
        startLocation_ = Observation()->GetStartLocation();
        staging_location_ = startLocation_;

        if (game_info_.enemy_start_locations.size() == 1) find_enemy_location = true;



        float minimum_distance = std::numeric_limits<float>::max();
        for (const auto& expansion : expansions_) {
            float current_distance = Distance2D(startLocation_, expansion);
            if (current_distance < .01f) {
                continue;
            }

            if (current_distance < minimum_distance) {
                if (Query()->Placement(ABILITY_ID::BUILD_NEXUS, expansion)) {
                    front_expansion = expansion;
                    minimum_distance = current_distance;
                }
            }
        }
        staging_location_ = Point3D(((staging_location_.x + front_expansion.x) / 2), ((staging_location_.y + front_expansion.y) / 2),
            ((staging_location_.z + front_expansion.z) / 2));
    }

    virtual void OnStep() final {

        const ObservationInterface* observation = Observation();
        Units units = observation->GetUnits(Unit::Self, IsArmy(observation));

        ManageWorkers(UNIT_TYPEID::PROTOSS_PROBE, ABILITY_ID::HARVEST_GATHER, UNIT_TYPEID::PROTOSS_ASSIMILATOR);

        if (!early_strategy) {
            EarlyStrategy();
        }
        if (iter_exp < expansions_.end() && find_enemy_location == true) {
            scoutprobe();
        }

        ManageUpgrades();
		
		// Control 시작
		Defend();
		//ManageArmy();
		ManageRush();


    }

    virtual void OnUnitIdle(const Unit* unit) override {
        switch (unit->unit_type.ToType()) {
        case UNIT_TYPEID::PROTOSS_PROBE: {
            MineIdleWorkers(unit, ABILITY_ID::HARVEST_GATHER, UNIT_TYPEID::PROTOSS_ASSIMILATOR);
            break;
        }
		case UNIT_TYPEID::PROTOSS_CARRIER: {
			ScoutWithUnit(unit, Observation());
			break;
		}
        default: {
            break;
        }
        }
        return;
    }

    GameInfo game_info_;
    std::vector<Point3D> expansions_;
    Point3D startLocation_;
    Point3D staging_location_;

    Point3D front_expansion;

    sc2::Point2D RushLocation;
    sc2::Point2D EnemyLocation;
    sc2::Point2D ReadyLocation1;
    sc2::Point2D ReadyLocation2;
    sc2::Point2D EscapeLocation;
    sc2::Point2D KitingLocation;
    uint64_t RushUnitTag;

    bool OracleCanAttack = false;
    int OracleCount = 0;
    int TempestCount = 0;
    int OracleStop = 0;
    int CarrierCount = 0;
	float base_range = 35;

private:

    void Chat(std::string Message) // 6.29 채팅 함수
    {
        Actions()->SendChat(Message);
    }

	const bool isBadEffect(const EffectID id) const
	{
		switch (id.ToType())
		{
		case EFFECT_ID::BLINDINGCLOUD:
		case EFFECT_ID::CORROSIVEBILE:
		case EFFECT_ID::LIBERATORMORPHED:
		case EFFECT_ID::LIBERATORMORPHING:
		case EFFECT_ID::LURKERATTACK:
		case EFFECT_ID::NUKEDOT:
		case EFFECT_ID::PSISTORM:
			//case EFFECT_ID::THERMALLANCES:
			return true;
		}
		return false;
	}
	bool EvadeEffect(Units units)
	{
		bool moving = false;
		for (const auto & unit : units)
		{
			moving |= EvadeEffect(unit);
		}
		return moving;
	}

	bool EvadeEffect(const Unit* unit)
	{
		bool moving = false;
		for (const auto & effect : Observation()->GetEffects())
		{
			if (isBadEffect(effect.effect_id))
			{
				const EffectData& ed = Observation()->GetEffectData().at(effect.effect_id);
				const float radius = ed.radius;
				for (const auto & pos : effect.positions)
				{
					const float dist = Distance2D(unit->pos, pos);
					if (dist < radius + unit->radius)
					{
						sc2::Point2D fleeingPos;
						if (dist > 0)
						{
							Vector2D diff = unit->pos - pos; // 7.3 적 유닛과의 반대 방향으로 도망
							Normalize2D(diff);
							fleeingPos = unit->pos + diff * 1.0f;
							//fleeingPos = pos + normalizeVector(rangedUnit->getPos() - pos, radius + 2.0f);
						}
						else
						{
							fleeingPos = Point2D(staging_location_);
						}
						Actions()->UnitCommand(unit, ABILITY_ID::MOVE, fleeingPos);
						Chat("Enemy Skill Run~");
						std::cout << "skill : " << ed.friendly_name << std::endl;
						moving = true;
						break;
					}
				}
			}
		}
		return moving;
	}

	bool early = true;
	bool EnemyRush;
    void Defend() { // 유닛 포인터 오류
		const ObservationInterface* observation = Observation();
		Units Oracles = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_ORACLE));
		Units nexus = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));
		int CurrentOracle = Oracles.size();

		// deal with nullptrs
		// remove oracles if dead
		if (oracle_second != nullptr && !oracle_second->is_alive) {
			oracle_second = nullptr;
		}
		// oracle first is alive and second is dead -> make second to first
		if (oracle_first != nullptr && !oracle_first->is_alive) {
			oracle_first = nullptr;
		}
		// assign oracles
		if (!Oracles.empty())
		{
			// find first oracle
			if (oracle_first == nullptr) {
				for (const auto & o : Oracles) {
					if (oracle_second == nullptr || o->tag != oracle_second->tag) {
						oracle_first = o;
						break;
					}
				}
			}
			// find second oracle
			if (oracle_second == nullptr && CurrentOracle >= 2) {
				for (const auto & o : Oracles) {
					if (oracle_first == nullptr || o->tag != oracle_first->tag) {
						oracle_second = o;
						break;
					}
				}
			}
		}
		// oracle second goes to
		if (oracle_second != nullptr)
		{
			if (oracle_second->energy > 50 &&
				(oracle_second->orders.empty() || oracle_second->orders.front().ability_id != ABILITY_ID::BUILD_STASISTRAP)) {
				Chat("OK~");
				float rx = GetRandomScalar();
				float ry = GetRandomScalar();
				StasisLocation = Point2D(pylonlocation.x + rx * 5, pylonlocation.y + ry * 5);
				Actions()->UnitCommand(oracle_second, ABILITY_ID::BUILD_STASISTRAP, StasisLocation);
			}
			else {
				ScoutWithUnit(oracle_second, observation);
			}
		}

        Units Workers = observation->GetUnits(Unit::Alliance::Self, IsWorker());
        Units EnemyWorkers = observation->GetUnits(Unit::Alliance::Enemy, IsWorker());
        int cannon_count = observation->GetUnits(Unit::Alliance::Enemy, Rusher(startLocation_)).size();
        int EnemyWorkercount = observation->GetUnits(Unit::Alliance::Enemy, IsWorker()).size();
        Units EnemyCannon = observation->GetUnits(Unit::Alliance::Enemy, Rusher(startLocation_));


        Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
        Units enemyUnitsInRegion;

        enemyUnitsInRegion.clear();
        for (const auto & unit : enemy_units)
        {
			if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_OVERLORD || unit->unit_type.ToType() == sc2::UNIT_TYPEID::PROTOSS_OBSERVER || unit->unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_REAPER)
			{
				continue;
			}

            if (Distance2D(startLocation_, unit->pos) < base_range + getAttackRangeGround(unit))
            {
                Chat("Enemy Captured 1");
                enemyUnitsInRegion.push_back(unit);
            }
        }

        if (!OracleTrained)
        {
            if (Killers.size() < enemyUnitsInRegion.size()) {
                Killers.resize(enemyUnitsInRegion.size(), nullptr);
            }

            if (enemyUnitsInRegion.size() > 0)
            {
                for (int i = 0; i<Killers.size();)
                {
                    auto& Killer = Killers.at(i);

                    if (Killer == nullptr || !Killer->is_alive)
                    {
                        Chat("Killer Captured 2");
                        GetRandomUnit(Killer, observation, UNIT_TYPEID::PROTOSS_PROBE);
                        if (Killer == probe_scout || Killer == probe_forge)
                            continue;
                    }
                    i++;
                }
                Chat("Killer Captured 2");
                Actions()->UnitCommand(Killers, ABILITY_ID::SMART, enemyUnitsInRegion.front());
            }
            else //enemyUnitsInRegion.size() == 0
            {
                //
            }

			if (!EnemyCannon.empty())
			{
				int PhotonRush = 1;
				for (const auto& worker : Workers) {
					Actions()->UnitCommand(worker, ABILITY_ID::ATTACK, EnemyCannon.front()->pos);
				}
			}
			else if (PhotonRush == 1)
			{
				for (const auto& worker : Workers) {
					Actions()->UnitCommand(worker, ABILITY_ID::STOP, EnemyCannon.front()->pos);
				}
				int PhotonRush = 0;
			}
        }

        if (enemyUnitsInRegion.size() > 3)
        {
            EnemyRush = true;
        }
		if (enemyUnitsInRegion.size() == 0)
			EnemyRush = false;
    }
    int PhotonRush = 0;

    void SetupRushLocation(const ObservationInterface *observation)
    {
        if (find_enemy_location) {
            ReadyLocation1 = game_info_.enemy_start_locations.front() + Point2D(30.0f, 0.0f);
            ReadyLocation2 = game_info_.enemy_start_locations.front() + Point2D(0.0f, 30.0f);
        }
        else
            ReadyLocation1 = startLocation_;
        ReadyLocation2 = startLocation_;
    }

    float OracleRange; // 절대적으로 생존
    float TempestRange = 3.0f;
    float CarrierRange = 1.9f;
    bool TimetoAttack = false;
    bool OracleTrained = false;
    Point2D pylonlocation;
    const Unit* oracle_second = nullptr;
    Point2D StasisLocation;

    Units Killers;
    const Unit* WorkerKiller = nullptr;
    const Unit* oracle_first = nullptr;

	void ManageRush() { // 5.17 오라클 유닛 관리 +6.25 폭풍함 유닛 관리
		const ObservationInterface* observation = Observation();
		Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
		Units Oracles = observation->GetUnits(Unit::Alliance::Self, IsOracle());
		Units Tempests = observation->GetUnits(Unit::Alliance::Self, IsTempest()); //6.25 폭풍함 컨트롤 추가
		Units Carriers = observation->GetUnits(Unit::Alliance::Self, IsCarrier());
		Units EnemyWorker = observation->GetUnits(Unit::Alliance::Enemy, IsWorker());
		Units AirAttackers = observation->GetUnits(Unit::Alliance::Enemy, AirAttacker()); //적 방어 유닛 및 건물
																						  //Units ProxyEnemy = observation->GetUnits(Unit::Alliance::Enemy, ExceptBuilding());
		float rx = GetRandomScalar();
		float ry = GetRandomScalar();

		for (const auto& unit : Oracles) {
			if (unit == oracle_second) continue;
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

			for (const auto& u : AirAttackers) {
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
			OracleRange = 5.5f;
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

				unit->weapon_cooldown == 0.0f;

				float RealCarrierRange = (unit->shield < 10)? 4.5 : CarrierRange;

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

    void RetreatWithCarrier(const Unit* unit) {
		if (pylonlocation != Point2D(0, 0))
			Actions()->UnitCommand(unit, ABILITY_ID::PATROL, pylonlocation);

        /*Location();
        float dist = Distance2D(unit->pos, CarrierLocation);
        if (dist < 10) {
        if (unit->orders.empty()) {
        return;
        }
        Actions()->UnitCommand(unit, ABILITY_ID::STOP);
        return;
        }
        if (unit->orders.empty() && dist > 14) {
        Actions()->UnitCommand(unit, ABILITY_ID::MOVE, CarrierLocation);
        }
        else if (!unit->orders.empty() && dist > 14) {
        if (unit->orders.front().ability_id != ABILITY_ID::MOVE) {
        Actions()->UnitCommand(unit, ABILITY_ID::MOVE, CarrierLocation);
        }
        }*/
    }

    const float getAttackRange(const Unit* target) const
    {
        sc2::Weapon groundWeapons;
        sc2::Weapon AirWeapons;

		if (target->unit_type.ToType() == sc2::UNIT_TYPEID::TERRAN_CYCLONE)
		{
			return 7.5f;
		}
		if (target->unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_VIPER)
		{
			return 9.0f;
		}
		if (target->unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_INFESTOR)
		{
			return 9.0f;
		}
		
        for (const auto & Weapon : Observation()->GetUnitTypeData()[target->unit_type].weapons)
        {
            if (Weapon.type == sc2::Weapon::TargetType::Air || Weapon.type == sc2::Weapon::TargetType::Any)
            {
                AirWeapons = Weapon;
            }
            if ((Weapon.type == sc2::Weapon::TargetType::Ground || Weapon.type == sc2::Weapon::TargetType::Any) && Weapon.range > groundWeapons.range)//Siege tanks
            {
                groundWeapons = Weapon;
                if (groundWeapons.range < 0.11f)//melee. Not exactly 0.1
                {
                    groundWeapons.range += target->radius;
                }
            }
        }
        return AirWeapons.range; // 7.5 이건 오로지 공중유닛을 위한 함수!!
                                 // return groundWeapons.range; // 7.5 사용하고 싶으면 쓸것
    }

    const float getAttackRangeGround(const Unit* target) const
    {
        sc2::Weapon groundWeapons;
        sc2::Weapon AirWeapons;

        for (const auto & Weapon : Observation()->GetUnitTypeData()[target->unit_type].weapons)
        {
            if (Weapon.type == sc2::Weapon::TargetType::Air || Weapon.type == sc2::Weapon::TargetType::Any)
            {
                AirWeapons = Weapon;
            }
            if ((Weapon.type == sc2::Weapon::TargetType::Ground || Weapon.type == sc2::Weapon::TargetType::Any) && Weapon.range > groundWeapons.range)//Siege tanks
            {
                groundWeapons = Weapon;
                if (groundWeapons.range < 0.11f)//melee. Not exactly 0.1
                {
                    groundWeapons.range += target->radius;
                }
            }
        }
        return groundWeapons.range;; //지상유닛을 위한 함수!!
                                     // return groundWeapons.range; // 7.5 사용하고 싶으면 쓸것
    }

    struct Rusher {
		Rusher(Point2D startLocation_) : sl(startLocation_) {
		}
        bool operator()(const Unit& unit) {
			return unit.unit_type.ToType() == UNIT_TYPEID::PROTOSS_PHOTONCANNON && Distance2D(sl, unit.pos) < 20;
        }
	private:
		Point2D sl;
    };

    struct AirAttacker { // 공중 공격 가능한 적들 (예언자가 기피해야하는 적 && 폭풍함이 우선 공격하는 적) //시간이 남으면 weapon.type == sc2::Weapon::TargetType::Air 으로 할수있지만 시간이 없음
        bool operator()(const Unit& unit) {
            switch (unit.unit_type.ToType()) {

            case UNIT_TYPEID::PROTOSS_STALKER: return true;
            case UNIT_TYPEID::PROTOSS_PHOTONCANNON: return true;

            case UNIT_TYPEID::TERRAN_MARINE: return true;
            case UNIT_TYPEID::TERRAN_MISSILETURRET: return true;
            case UNIT_TYPEID::TERRAN_BUNKER: return true;

            case UNIT_TYPEID::ZERG_SPORECRAWLER: return true;
            case UNIT_TYPEID::ZERG_QUEEN: return true;

            case UNIT_TYPEID::TERRAN_BATTLECRUISER:
            case UNIT_TYPEID::TERRAN_CYCLONE:
            case UNIT_TYPEID::TERRAN_GHOST:
            case UNIT_TYPEID::TERRAN_LIBERATOR:
            case UNIT_TYPEID::TERRAN_THOR:
            case UNIT_TYPEID::TERRAN_VIKINGASSAULT:
            case UNIT_TYPEID::TERRAN_VIKINGFIGHTER:
            case UNIT_TYPEID::TERRAN_WIDOWMINEBURROWED:
            case UNIT_TYPEID::TERRAN_WIDOWMINE:

            case UNIT_TYPEID::ZERG_HYDRALISK:
            case UNIT_TYPEID::ZERG_MUTALISK:
            case UNIT_TYPEID::ZERG_INFESTORTERRAN:
            case UNIT_TYPEID::ZERG_CORRUPTOR:


            case UNIT_TYPEID::PROTOSS_TEMPEST:
            case UNIT_TYPEID::PROTOSS_VOIDRAY:
            case UNIT_TYPEID::PROTOSS_PHOENIX:
            case UNIT_TYPEID::PROTOSS_CARRIER:
            case UNIT_TYPEID::PROTOSS_SENTRY:
            case UNIT_TYPEID::PROTOSS_ARCHON: return true;

            default: return false;
            }
        }
    };


    struct IsOracle { // 예언자인지 감지
        bool operator()(const Unit& unit) {

            switch (unit.unit_type.ToType()) {
            case UNIT_TYPEID::PROTOSS_ORACLE: return true;
            default: return false;
            }
        }
    };

    struct IsTempest {
        bool operator()(const Unit& unit) {
            switch (unit.unit_type.ToType()) {
            case UNIT_TYPEID::PROTOSS_TEMPEST: return true;
            default: return false;
            }
        }
    };

    struct IsCarrier {
        bool operator()(const Unit& unit) {
            switch (unit.unit_type.ToType()) {
            case UNIT_TYPEID::PROTOSS_CARRIER: return true;
            default: return false;
            }
        }
    };

    void AttackWithUnitType(UnitTypeID unit_type, const ObservationInterface* observation) {
        Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
        for (const auto& unit : units) {
            AttackWithUnit(unit, observation);
        }
    }

    void AttackWithUnit(const Unit* unit, const ObservationInterface* observation) {
        //If unit isn't doing anything make it attack.
        Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
        if (enemy_units.empty()) {
            return;
        }

        // 유닛이 하는게 없을 때 공격
        if (unit->orders.empty()) {
            Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front()->pos);
            return;
        }

        //If the unit is doing something besides attacking, make it attack. // 공격을 안하면 공격명령
        if (unit->orders.front().ability_id != ABILITY_ID::ATTACK) {
            Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front()->pos);
        }
    }

    void ScoutWithUnits(UnitTypeID unit_type, const ObservationInterface* observation) {
        Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
        for (const auto& unit : units) {
            ScoutWithUnit(unit, observation);
        }
    }

    void ScoutWithUnit(const Unit* unit, const ObservationInterface* observation) {
        Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy, IsAttackable());
        if (!unit->orders.empty()) {
            return;
        }
        Point2D target_pos;

        if (FindEnemyPosition(target_pos)) {
            if (Distance2D(unit->pos, target_pos) < 20 && enemy_units.empty()) {
                if (TryFindRandomPathableLocation(unit, target_pos)) {
                    Actions()->UnitCommand(unit, ABILITY_ID::SMART, target_pos);
                    return;
                }
            }
            else if (!enemy_units.empty())
            {
                Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front());
                return;
            }
            Actions()->UnitCommand(unit, ABILITY_ID::SMART, target_pos);
        }
        else {
            if (TryFindRandomPathableLocation(unit, target_pos)) {
                Actions()->UnitCommand(unit, ABILITY_ID::SMART, target_pos);
            }
        }
    }


    void RetreatWithUnits(UnitTypeID unit_type, Point2D retreat_position) {
        const ObservationInterface* observation = Observation();
        Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
        for (const auto& unit : units) {
            RetreatWithUnit(unit, retreat_position);
        }
    }

    void RetreatWithUnit(const Unit* unit, Point2D retreat_position) {
        float dist = Distance2D(unit->pos, retreat_position);

        if (dist < 10) {
            if (unit->orders.empty()) {
                return;
            }
            Actions()->UnitCommand(unit, ABILITY_ID::STOP);
            return;
        }

        if (unit->orders.empty() && dist > 14) {
            Actions()->UnitCommand(unit, ABILITY_ID::MOVE, retreat_position);
        }
        else if (!unit->orders.empty() && dist > 14) {
            if (unit->orders.front().ability_id != ABILITY_ID::MOVE) {
                Actions()->UnitCommand(unit, ABILITY_ID::MOVE, retreat_position);
            }
        }
    }


    bool FindEnemyPosition(Point2D& target_pos) {
        if (game_info_.enemy_start_locations.empty()) {
            return false;
        }
        target_pos = game_info_.enemy_start_locations.front();
        return true;
    }

    bool TryFindRandomPathableLocation(const Unit* unit, Point2D& target_pos) {
        // First, find a random point inside the playable area of the map.
        float playable_w = game_info_.playable_max.x - game_info_.playable_min.x;
        float playable_h = game_info_.playable_max.y - game_info_.playable_min.y;

        // The case where game_info_ does not provide a valid answer
        if (playable_w == 0 || playable_h == 0) {
            playable_w = 236;
            playable_h = 228;
        }

        target_pos.x = playable_w * GetRandomFraction() + game_info_.playable_min.x;
        target_pos.y = playable_h * GetRandomFraction() + game_info_.playable_min.y;

        // Now send a pathing query from the unit to that point. Can also query from point to point,
        // but using a unit tag wherever possible will be more accurate.
        // Note: This query must communicate with the game to get a result which affects performance.
        // Ideally batch up the queries (using PathingDistanceBatched) and do many at once.
        float distance = Query()->PathingDistance(unit, target_pos);

        return distance > 0.1f;
    }

    struct IsAttackable {
        bool operator()(const Unit& unit) {
            switch (unit.unit_type.ToType()) {
            case UNIT_TYPEID::ZERG_OVERLORD: return false;
            case UNIT_TYPEID::ZERG_OVERSEER: return false;
            case UNIT_TYPEID::PROTOSS_OBSERVER: return false;
            default: return true;
            }
        }
    };

    bool TryBuildUnitChrono(AbilityID ability_type_for_unit, UnitTypeID unit_type) {
        const ObservationInterface* observation = Observation();

        //If we are at supply cap, don't build anymore units, unless its an overlord.
        if (observation->GetFoodUsed() >= observation->GetFoodCap() && ability_type_for_unit != ABILITY_ID::TRAIN_OVERLORD) {
            return false;
        }
        const Unit* unit = nullptr;
        if (!GetRandomUnit(unit, observation, unit_type)) {
            return false;
        }
        if (!unit->orders.empty()) {

            return false;
        }
        if (unit->build_progress != 1) {
            return false;
        }

        Actions()->UnitCommand(unit, ability_type_for_unit);
        Chronoboost(unit);
        return true;
    }

    bool Chronoboost(const Unit * unit) {
        const ObservationInterface* observation = Observation();
        Units nexus = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));

        if (nexus.empty()) return false;
        else {
            for (int i = 0; i < nexus.size(); ++i) {
                if (nexus.at(i)->build_progress != 1) {
                    continue;
                }
                else {
                    if (i < nexus.size()) {
                        if (nexus.at(i)->energy >= 50) {
                            Actions()->UnitCommand(nexus.at(i), ABILITY_ID::EFFECT_CHRONOBOOST, unit);
                            return true;
                        }
                        else
                        {
                            return false;
                            //Chat("Not enough Energy for Chronoboost~"); Too loud
                        }
                    }
                }
            }
            return false;
        }
    }

    struct IsWorker {
        bool operator()(const Unit& unit) {
            switch (unit.unit_type.ToType()) {
            case UNIT_TYPEID::PROTOSS_PROBE: return true;
            case UNIT_TYPEID::ZERG_DRONE: return true;
            case UNIT_TYPEID::TERRAN_SCV: return true;
            default: return false;
            }
        }
    };

    struct IsArmy {
        IsArmy(const ObservationInterface* obs) : observation_(obs) {}

        bool operator()(const Unit& unit) {
            auto attributes = observation_->GetUnitTypeData().at(unit.unit_type).attributes;
            for (const auto& attribute : attributes) {
                if (attribute == Attribute::Structure) {
                    return false;
                }
            }
            switch (unit.unit_type.ToType()) {
            case UNIT_TYPEID::ZERG_OVERLORD: return false;
            case UNIT_TYPEID::PROTOSS_PROBE: return false;
            case UNIT_TYPEID::ZERG_DRONE: return false;
            case UNIT_TYPEID::TERRAN_SCV: return false;
            case UNIT_TYPEID::ZERG_QUEEN: return false;
            case UNIT_TYPEID::ZERG_LARVA: return false;
            case UNIT_TYPEID::ZERG_EGG: return false;
            case UNIT_TYPEID::TERRAN_MULE: return false;
            case UNIT_TYPEID::TERRAN_NUKE: return false;
            default: return true;
            }
        }

        const ObservationInterface* observation_;
    };

    struct IsTownHall {
        bool operator()(const Unit& unit) {

            switch (unit.unit_type.ToType()) {
            case UNIT_TYPEID::ZERG_HATCHERY: return true;
            case UNIT_TYPEID::ZERG_LAIR: return true;
            case UNIT_TYPEID::ZERG_HIVE: return true;
            case UNIT_TYPEID::TERRAN_COMMANDCENTER: return true;
            case UNIT_TYPEID::TERRAN_ORBITALCOMMAND: return true;
            case UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING: return true;
            case UNIT_TYPEID::TERRAN_PLANETARYFORTRESS: return true;
            case UNIT_TYPEID::PROTOSS_NEXUS: return true;
            default: return false;
            }
        }
    };

    struct IsStructure {
        IsStructure(const ObservationInterface* obs) : observation_(obs) {};

        bool operator()(const Unit& unit) {
            auto& attributes = observation_->GetUnitTypeData().at(unit.unit_type).attributes;
            bool is_structure = false;
            for (const auto& attribute : attributes) {
                if (attribute == Attribute::Structure) {
                    is_structure = true;
                }
            }
            return is_structure;
        }

        const ObservationInterface* observation_;
    };

    struct IsVespeneGeyser {
        bool operator()(const Unit& unit) {
            switch (unit.unit_type.ToType()) {
            case UNIT_TYPEID::NEUTRAL_VESPENEGEYSER: return true;
            case UNIT_TYPEID::NEUTRAL_SPACEPLATFORMGEYSER: return true;
            case UNIT_TYPEID::NEUTRAL_PROTOSSVESPENEGEYSER: return true;
            case UNIT_TYPEID::NEUTRAL_PURIFIERVESPENEGEYSER:    return true;
            case UNIT_TYPEID::NEUTRAL_RICHVESPENEGEYSER:    return true;
            case UNIT_TYPEID::NEUTRAL_SHAKURASVESPENEGEYSER:    return true;
            default: return false;
            }
        }
    };

    size_t CountUnitType(const ObservationInterface* observation, UnitTypeID unit_type) {
        return observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
    }

    bool GetRandomUnit(const Unit*& unit_out, const ObservationInterface* observation, UnitTypeID unit_type) {
        Units my_units = observation->GetUnits(Unit::Alliance::Self);
        std::random_shuffle(my_units.begin(), my_units.end()); // Doesn't work, or doesn't work well.
        for (const auto unit : my_units) {
            if (unit->unit_type == unit_type) {
                unit_out = unit;
                return true;
            }
        }
        return false;
    }

	const Unit* FindNearestMineralPatch(const Point2D& start) {
		Units units = Observation()->GetUnits(Unit::Alliance::Neutral);
		float distance = std::numeric_limits<float>::max();
		const Unit* target = nullptr;
		for (const auto& u : units) {
			if ([](const Unit& unit) {
				return unit.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD750 ||
					unit.unit_type == UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD750 ||
					unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERMINERALFIELD750 ||
					unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERRICHMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERRICHMINERALFIELD750 ||
					unit.unit_type == UNIT_TYPEID::NEUTRAL_LABMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_LABMINERALFIELD750 ||
					unit.unit_type == UNIT_TYPEID::NEUTRAL_BATTLESTATIONMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_BATTLESTATIONMINERALFIELD750 ||
					unit.unit_type == UNIT_TYPEID::NEUTRAL_VESPENEGEYSER || unit.unit_type == UNIT_TYPEID::NEUTRAL_PROTOSSVESPENEGEYSER ||
					unit.unit_type == UNIT_TYPEID::NEUTRAL_SPACEPLATFORMGEYSER || unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERVESPENEGEYSER ||
					unit.unit_type == UNIT_TYPEID::NEUTRAL_SHAKURASVESPENEGEYSER || unit.unit_type == UNIT_TYPEID::NEUTRAL_RICHVESPENEGEYSER;
			}(*u)) {
				float d = DistanceSquared2D(u->pos, start);
				if (d < distance) {
					distance = d;
					target = u;
				}
			}
		}
		//If we never found one return false;
		if (distance == std::numeric_limits<float>::max()) {
			return target;
		}
		return target;
	}

    bool TryBuildUnit(AbilityID ability_type_for_unit, UnitTypeID unit_type) {
        const ObservationInterface* observation = Observation();

        //If we are at supply cap, don't build anymore units, unless its an overlord.
        if (observation->GetFoodUsed() >= observation->GetFoodCap() && ability_type_for_unit != ABILITY_ID::TRAIN_OVERLORD) {
            return false;
        }
        const Unit* unit = nullptr;
        if (!GetRandomUnit(unit, observation, unit_type)) {
            return false;
        }
        if (!unit->orders.empty()) {
            return false;
        }

        if (unit->build_progress != 1) {
            return false;
        }

        Actions()->UnitCommand(unit, ability_type_for_unit);
        return true;
    }

    bool TryBuildStructure(AbilityID ability_type_for_structure, UnitTypeID unit_type, Point2D location, bool isExpansion = false) {

        const ObservationInterface* observation = Observation();
        Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));

        //if we have no workers Don't build
        if (workers.empty()) {
            return false;
        }

        // Check to see if there is already a worker heading out to build it
        for (const auto& worker : workers) {
            for (const auto& order : worker->orders) {
                if (order.ability_id == ability_type_for_structure) {
                    return false;
                }
            }
        }



        // If no worker is already building one, get a random worker to build one
        const Unit* unit = nullptr;
        for (const auto& worker : workers) {
            //전진 프로브는 제외
            if (worker == probe_scout) continue;
            for (const auto& order : worker->orders) {
                if (order.ability_id == ABILITY_ID::HARVEST_GATHER) {
                    unit = worker;
                    break;
                }
            }
            if (unit != nullptr) break;
        }
        if (unit == nullptr) return false;

        // Check to see if unit can make it there
        if (Query()->PathingDistance(unit, location) < 0.1f) {
            return false;
        }
        if (!isExpansion) {
            for (const auto& expansion : expansions_) {
                if (Distance2D(location, Point2D(expansion.x, expansion.y)) < 7) {
                    return false;
                }
            }
        }
        // Check to see if unit can build there
        if (Query()->Placement(ability_type_for_structure, location)) {
            Actions()->UnitCommand(unit, ability_type_for_structure, location);
            return true;
        }
        return false;

    }

    bool TryBuildStructure(AbilityID ability_type_for_structure, UnitTypeID unit_type, Tag location_tag) {
        const ObservationInterface* observation = Observation();
        Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
        const Unit* target = observation->GetUnit(location_tag);

        if (workers.empty()) {
            return false;
        }

        // Check to see if there is already a worker heading out to build it

        if (ability_type_for_structure != ABILITY_ID::BUILD_ASSIMILATOR) {
            for (const auto& worker : workers) {
                if (worker == probe_scout || worker == probe_forge) continue;
                for (const auto& order : worker->orders) {
                    if (order.ability_id == ability_type_for_structure) {
                        return false;
                    }
                }
            }
        }

        // If no worker is already building one, get a random worker to build one
        const Unit* unit = GetRandomEntry(workers);

        // Check to see if unit can build there
        if (Query()->Placement(ability_type_for_structure, target->pos)) {
            Actions()->UnitCommand(unit, ability_type_for_structure, target);
            return true;
        }
        return false;

    }

    bool TryBuildStructureNearPylon(AbilityID ability_type_for_structure, UnitTypeID) {
        const ObservationInterface* observation = Observation();

        //Need to check to make sure its a pylon instead of a warp prism
        std::vector<PowerSource> power_sources = observation->GetPowerSources();
        if (power_sources.empty()) {
            return false;
        }

        const PowerSource& random_power_source = GetRandomEntry(power_sources);
        if (observation->GetUnit(random_power_source.tag) != nullptr) {
            if (observation->GetUnit(random_power_source.tag)->unit_type == UNIT_TYPEID::PROTOSS_WARPPRISM) {
                return false;
            }
        }
        else {
            return false;
        }
        float radius = random_power_source.radius;
        float rx = GetRandomScalar();
        float ry = GetRandomScalar();
        Point2D build_location = Point2D(random_power_source.position.x + rx * radius, random_power_source.position.y + ry * radius);
        return TryBuildStructure(ability_type_for_structure, UNIT_TYPEID::PROTOSS_PROBE, build_location);
    }

    bool TryBuildStructureNearPylon(AbilityID ability_type_for_structure, UnitTypeID, const Unit* pylon) {
        const ObservationInterface* observation = Observation();

        //Need to check to make sure its a pylon instead of a warp prism
        std::vector<PowerSource> power_sources = observation->GetPowerSources();
        if (power_sources.empty()) {
            return false;
        }

        const PowerSource& random_power_source = GetRandomEntry(power_sources);
        if (observation->GetUnit(random_power_source.tag) != nullptr) {
            if (observation->GetUnit(random_power_source.tag)->unit_type == UNIT_TYPEID::PROTOSS_WARPPRISM) {
                return false;
            }
        }
        else {
            return false;
        }
        float radius = random_power_source.radius;
        float rx = GetRandomScalar();
        float ry = GetRandomScalar();
        Point2D build_location = Point2D(pylon->pos.x + rx * radius, pylon->pos.y + ry * radius);
        return TryBuildStructure(ability_type_for_structure, UNIT_TYPEID::PROTOSS_PROBE, build_location);
    }

    bool TryBuildStructureNearPylonWithUnit(const Unit* unit, AbilityID ability_type_for_structure, const Unit* pylon) {
		if (unit == nullptr) return false;
		if (pylon == nullptr) return false;
		
        const ObservationInterface* observation = Observation();
        std::vector<PowerSource> power_sources = observation->GetPowerSources();

        if (power_sources.empty()) {
            return false;
        }
		
        float radius = power_sources.front().radius;
        float rx = GetRandomScalar();
        float ry = GetRandomScalar();
        Point2D location = Point2D(pylon->pos.x + rx * radius, pylon->pos.y + ry * radius);

        // Check to see if unit can make it there
        if (Query()->PathingDistance(unit, location) < 0.1f) {
            return false;
        }
        // Check to see if unit can build there
        if (Query()->Placement(ability_type_for_structure, location)) {
            Actions()->UnitCommand(unit, ability_type_for_structure, location);
            return true;
        }
        return false;

    }

    bool TryBuildForge(const Unit* unit, const Unit* pylon) {

		if (unit == nullptr) return false;
		if (pylon == nullptr) return false;
        const ObservationInterface* observation = Observation();
        Units bases = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));

        std::vector<PowerSource> power_sources = observation->GetPowerSources();
        if (power_sources.empty()) {
            return false;
        }
        float radius = power_sources.front().radius;
        float rx = GetRandomScalar();
        float ry = GetRandomScalar();
        Point2D location = Point2D(pylon->pos.x + rx * radius, pylon->pos.y + ry * radius);

        if (Distance2D(location, front_expansion)<bases.front()->radius*1.3) {
            return false;
        }
        // Check to see if unit can make it there
        if (Query()->PathingDistance(unit, location) < 0.1f) {
            return false;
        }
        // Check to see if unit can build there
        if (Query()->Placement(ABILITY_ID::BUILD_FORGE, location)) {
            Actions()->UnitCommand(unit, ABILITY_ID::BUILD_FORGE, location);
            return true;
        }
        return false;
    }

    bool TryBuildGas(AbilityID build_ability, UnitTypeID worker_type, Point2D base_location) {
        const ObservationInterface* observation = Observation();
        Units geysers = observation->GetUnits(Unit::Alliance::Neutral, IsVespeneGeyser());

        //only search within this radius
        float minimum_distance = 15.0f;
        Tag closestGeyser = 0;
        for (const auto& geyser : geysers) {
            float current_distance = Distance2D(base_location, geyser->pos);
            if (current_distance < minimum_distance) {
                if (Query()->Placement(build_ability, geyser->pos)) {
                    minimum_distance = current_distance;
                    closestGeyser = geyser->tag;
                }
            }
        }

        // In the case where there are no more available geysers nearby
        if (closestGeyser == 0) {
            return false;
        }
        return TryBuildStructure(build_ability, worker_type, closestGeyser);
    }

    bool TryBuildPylonIfNeeded(int MaxBuildAtOnce = 1) {
		if (MaxBuildAtOnce < 1) MaxBuildAtOnce = 1;
        const ObservationInterface* observation = Observation();

		
		int i;
		for (i = 1; i <= MaxBuildAtOnce; i++) {
			// If we are not supply capped, don't build a supply depot.
			int margin = i * 8 - 2;
			if (observation->GetFoodUsed() < observation->GetFoodCap() - margin) {
				i--;
				break;
			}
		}
		if (i == 0) return false;
		if (observation->GetFoodCap() == 200) return false;

		return TryBuildPylon(staging_location_, 15, i);
    }
	bool TryBuildPylon(Point2D location, int radius = 3, int MaxBuildAtOnce = 1) {
		const ObservationInterface* observation = Observation();

		if (observation->GetMinerals() < 100) {
			return false;
		}

		//check to see if there is already on building
		Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PYLON));

		int NumBuildInProgress = 0;
		for (const auto& unit : units) {
			if (unit->build_progress != 1) {
				NumBuildInProgress++;
				if (NumBuildInProgress >= MaxBuildAtOnce) {
					return false;
				}
			}
		}
		

		// Try and build a pylon. Find a random Probe and give it the order.
		float rx = GetRandomScalar();
		float ry = GetRandomScalar();
		Point2D build_location = Point2D(location.x + rx * radius, location.y + ry * radius);
		return TryBuildStructure(ABILITY_ID::BUILD_PYLON, UNIT_TYPEID::PROTOSS_PROBE, build_location);
	}

    bool TryBuildPylonWide(Point2D location, int MaxBuildAtOnce = 1) {
        return TryBuildPylon(location, 8, MaxBuildAtOnce);
    }

    void MineIdleWorkers(const Unit* worker, AbilityID worker_gather_command, UnitTypeID vespene_building_type) {
        if (worker == probe_scout || worker == probe_forge) return;

        const ObservationInterface* observation = Observation();
        Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
        Units geysers = observation->GetUnits(Unit::Alliance::Self, IsUnit(vespene_building_type));

        const Unit* valid_mineral_patch = nullptr;

        if (bases.empty()) {
            return;
        }

        for (const auto& geyser : geysers) {
            if (geyser->assigned_harvesters < geyser->ideal_harvesters) {
                Actions()->UnitCommand(worker, worker_gather_command, geyser);
                return;
            }
        }
        //Search for a base that is missing workers.
        for (const auto& base : bases) {
            //If we have already mined out here skip the base.
            if (base->ideal_harvesters == 0 || base->build_progress != 1) {
                continue;
            }
            if (base->assigned_harvesters < base->ideal_harvesters) {
                valid_mineral_patch = FindNearestMineralPatch(base->pos);
                Actions()->UnitCommand(worker, worker_gather_command, valid_mineral_patch);
                return;
            }
        }

        if (!worker->orders.empty()) {
            return;
        }

        //If all workers are spots are filled just go to any base.
        const Unit* random_base = GetRandomEntry(bases);
        valid_mineral_patch = FindNearestMineralPatch(random_base->pos);
        Actions()->UnitCommand(worker, worker_gather_command, valid_mineral_patch);
    }

    int GetExpectedWorkers(UNIT_TYPEID vespene_building_type) {
        const ObservationInterface* observation = Observation();
        Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
        Units geysers = observation->GetUnits(Unit::Alliance::Self, IsUnit(vespene_building_type));
        int expected_workers = 0;
        for (const auto& base : bases) {
            if (base->build_progress != 1) {
                continue;
            }
            expected_workers += base->ideal_harvesters;
        }

        for (const auto& geyser : geysers) {
            if (geyser->vespene_contents > 0) {
                if (geyser->build_progress != 1) {
                    continue;
                }
                expected_workers += geyser->ideal_harvesters;
            }
        }

        return expected_workers;
    }

    void ManageWorkers(UNIT_TYPEID worker_type, AbilityID worker_gather_command, UNIT_TYPEID vespene_building_type) {
        const ObservationInterface* observation = Observation();
        Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
        Units geysers = observation->GetUnits(Unit::Alliance::Self, IsUnit(vespene_building_type));

        if (bases.empty()) {
            return;
        }

        for (const auto& base : bases) {
            //If we have already mined out or still building here skip the base.
            if (base->ideal_harvesters == 0 || base->build_progress != 1) {
                continue;
            }
            //if base is
            if (base->assigned_harvesters > base->ideal_harvesters) {
                Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(worker_type));

                for (const auto& worker : workers) {
                    if (worker == probe_scout) continue;
                    if (!worker->orders.empty()) {
                        if (worker->orders.front().target_unit_tag == base->tag) {
                            //This should allow them to be picked up by mineidleworkers()
                            MineIdleWorkers(worker, worker_gather_command, vespene_building_type);
                            return;
                        }
                    }
                }
            }
        }
        Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(worker_type));

        for (const auto& geyser : geysers) {
            if (geyser->ideal_harvesters == 0 || geyser->build_progress != 1) {
                continue;
            }
            if (geyser->assigned_harvesters > geyser->ideal_harvesters) {
                for (const auto& worker : workers) {
                    if (worker == probe_scout) continue;
                    if (!worker->orders.empty()) {
                        if (worker->orders.front().target_unit_tag == geyser->tag) {
                            //This should allow them to be picked up by mineidleworkers()
                            MineIdleWorkers(worker, worker_gather_command, vespene_building_type);
                            return;
                        }
                    }
                }
            }
            else if (geyser->assigned_harvesters < geyser->ideal_harvesters) {
                for (const auto& worker : workers) {
                    if (!worker->orders.empty()) {
                        //This should move a worker that isn't mining gas to gas
                        const Unit* target = observation->GetUnit(worker->orders.front().target_unit_tag);
                        if (target == nullptr) {
                            continue;
                        }
                        if (target->unit_type != vespene_building_type) {
                            //This should allow them to be picked up by mineidleworkers()
                            MineIdleWorkers(worker, worker_gather_command, vespene_building_type);
                            return;
                        }
                    }
                }
            }
        }
    }

    void ManageUpgrades() {
        const ObservationInterface* observation = Observation();
        auto upgrades = observation->GetUpgrades();
        for (const auto& upgrade : upgrades) {
            if (upgrade == UPGRADE_ID::PROTOSSAIRWEAPONSLEVEL1 || upgrade == UPGRADE_ID::PROTOSSAIRWEAPONSLEVEL2) {
                TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRWEAPONS, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
            }
            else if (upgrade == UPGRADE_ID::PROTOSSAIRWEAPONSLEVEL3 || upgrade == UPGRADE_ID::PROTOSSAIRARMORSLEVEL1 || upgrade == UPGRADE_ID::PROTOSSAIRARMORSLEVEL2) {
                TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRARMOR, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
            }
        }
    }

    bool TryExpand(AbilityID build_ability, UnitTypeID worker_type) {
        const ObservationInterface* observation = Observation();
        float minimum_distance = std::numeric_limits<float>::max();
        Point3D closest_expansion;
        for (const auto& expansion : expansions_) {
            float current_distance = Distance2D(startLocation_, expansion);
            if (current_distance < .01f) {
                continue;
            }

            if (current_distance < minimum_distance) {
                if (Query()->Placement(build_ability, expansion)) {
                    closest_expansion = expansion;
                    minimum_distance = current_distance;
                }
            }
        }
        //only update staging location up till 3 bases.
        if (TryBuildStructure(build_ability, worker_type, closest_expansion, true) && observation->GetUnits(Unit::Self, IsTownHall()).size() < 4) {
            staging_location_ = Point3D(((staging_location_.x + closest_expansion.x) / 2), ((staging_location_.y + closest_expansion.y) / 2),
                ((staging_location_.z + closest_expansion.z) / 2));
            return true;
        }
        return false;

    }

    bool TryBuildExpansionNexus() {
        const ObservationInterface* observation = Observation();

        //Don't have more active bases than we can provide workers for
        if (GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR) > max_worker_count_) {
            return false;
        }
        // If we have extra workers around, try and build another nexus.
        if (GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR) < observation->GetFoodWorkers() - 16) {
            return TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
        }
        //Only build another nexus if we are floating extra minerals
		int CurrentStargate = CountUnitType(observation, UNIT_TYPEID::PROTOSS_STARGATE);
		
		if (!EnemyRush && observation->GetMinerals() > CurrentStargate * 350 + 400) {
            return TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
        }
        return false;
    }

    bool TryBuildAssimilator() {
        const ObservationInterface* observation = Observation();
        Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());

        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_ASSIMILATOR) >= observation->GetUnits(Unit::Alliance::Self, IsTownHall()).size() * 2) {
            return false;
        }

        for (const auto& base : bases) {
            if (base->assigned_harvesters >= base->ideal_harvesters) {
                if (base->build_progress == 1) {
                    if (TryBuildGas(ABILITY_ID::BUILD_ASSIMILATOR, UNIT_TYPEID::PROTOSS_PROBE, base->pos)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

	bool TryBuildProbe() {
		const ObservationInterface* observation = Observation();
		Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
		if (observation->GetFoodWorkers() >= max_worker_count_) {
			return false;
			
		}
		
		if (observation->GetFoodUsed() >= observation->GetFoodCap()) {
			return false;
			
		}
		
		if (observation->GetFoodWorkers() > GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR)) {
			return false;
			
		}
		
		for (const auto& base : bases) {
			//if there is a base with less than ideal workers
			if (base->assigned_harvesters < base->ideal_harvesters && base->build_progress == 1) {
				if (observation->GetMinerals() >= 50) {
					return TryBuildUnitChrono(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
					
				}
				
			}
			
		}
		return false;
		
	}
	
    bool EarlyStrategy() {
        const ObservationInterface* observation = Observation();
        Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PROBE));
        Units bases = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));
        Units pylons = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PYLON));
        Units enemy_structures = observation->GetUnits(Unit::Alliance::Enemy, IsStructure(observation));
        Units enemy_townhalls = observation->GetUnits(Unit::Alliance::Enemy, IsTownHall());


        size_t forge_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_FORGE);
        size_t cannon_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_PHOTONCANNON);
        size_t gateway_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_GATEWAY) + CountUnitType(observation, UNIT_TYPEID::PROTOSS_WARPGATE);
        size_t assimilator_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ASSIMILATOR);
        size_t cybernetics_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
        size_t stargate_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_STARGATE);

        Point2D probe_scout_dest = Point2D((double)game_info_.width / 2, (double)game_info_.height / 2);

        std::cout << stage_number << std::endl;

        if (stage_number>1) {
            if (find_enemy_location == false && pylons.size()>0) {
                Actions()->UnitCommand(probe_scout, ABILITY_ID::MOVE, game_info_.enemy_start_locations.front());
                if (!enemy_townhalls.empty() || enemy_structures.size()>2 || game_info_.enemy_start_locations.size() == 1) {
                    if (Distance2D(probe_scout->pos, game_info_.enemy_start_locations.front())<10 || game_info_.enemy_start_locations.size() == 1) {
                        find_enemy_location = true;
                        std::cout << "find!" << std::endl;
                        Actions()->UnitCommand(probe_scout, ABILITY_ID::STOP);
                        float minimum_distance = std::numeric_limits<float>::max();
                        for (const auto& expansion : expansions_) {
                            float current_distance = Distance2D(game_info_.enemy_start_locations.front(), expansion);
                            if (current_distance < 3) {
                                continue;
                            }

                            if (current_distance < minimum_distance) {
                                enemy_expansion = expansion;
                                minimum_distance = current_distance;
                            }
                        }
                    }
                }
                else {
                    if (Distance2D(probe_scout->pos, game_info_.enemy_start_locations.front())<7) {
                        iter_esl = game_info_.enemy_start_locations.begin();
                        game_info_.enemy_start_locations.erase(iter_esl);
                    }
                }
            }
        }

        switch (stage_number) {
        case 4:
            break;
        case 5:
            break;
        default:
            if (observation->GetFoodWorkers()<25) {
                TryBuildUnitChrono(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
            }
            else {
                if (GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR) > observation->GetFoodWorkers() && observation->GetFoodWorkers() < max_worker_count_) {
                    TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
                }
            }
			if (bases.size() > 3) {
				TryBuildProbe();
			}

            if (stage_number>9 && assimilator_count<4) {
                for (const auto& b : bases) {
                    if (b != base) {
                        if (b->assigned_harvesters >= 10) {
                            TryBuildGas(ABILITY_ID::BUILD_ASSIMILATOR, UNIT_TYPEID::PROTOSS_PROBE, b->pos);
                        }
                    }
                }
            }

            if (stage_number>18) {
                TryBuildPylonIfNeeded(CountUnitType(observation, UNIT_TYPEID::PROTOSS_STARGATE));
            }

            if (stage_number>18) {
                TryBuildUnitChrono(ABILITY_ID::TRAIN_CARRIER, UNIT_TYPEID::PROTOSS_STARGATE);
            }

            if (stage_number>26) {
                TryBuildExpansionNexus();
                TryBuildAssimilator();
                if (stargate_count<bases.size()) {
                    TryBuildStructureNearPylon(ABILITY_ID::BUILD_STARGATE, UNIT_TYPEID::PROTOSS_PROBE);
                }
				int CurrentStargate = CountUnitType(observation, UNIT_TYPEID::PROTOSS_STARGATE);
				
				if (observation->GetMinerals() > CurrentStargate * 350 + 150 && bases.size() * 8 > CountUnitType(observation, UNIT_TYPEID::PROTOSS_PHOTONCANNON)) {
                    TryBuildStructureNearPylon(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE);
                }
            }
			if (stage_number > 2 && pylon_first != nullptr && !pylon_first->is_alive) {
				bool rebuilding_pylon = false;
				for (const auto & p : observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PYLON))) {
					if (Point2D(p->pos) == pylonlocation) {
						rebuilding_pylon = true;
						if (p->build_progress == 1.0) {
							pylon_first = p;
						}
						break;
					}
				}
				if (!rebuilding_pylon && !EnemyRush) {
					TryBuildPylon(pylonlocation, 0);
				}
			}
        }


        switch (stage_number) {
        case 0:
            if (bases.size()>1) {
                for (const auto& b : bases) {
                    if (Distance2D(b->pos, startLocation_)<3) {
                        base = b;
                    }
                }
            }
            else {
                base = bases.front();
            }//본진 넥서스 지정

            if (probe_scout == nullptr || !probe_scout->is_alive) {
                probe_scout_dest = front_expansion;
                GetRandomUnit(probe_scout, observation, UNIT_TYPEID::PROTOSS_PROBE);
            }//정찰 프로브 지정
            if (probe_forge == nullptr || !probe_forge->is_alive) {
                GetRandomUnit(probe_forge, observation, UNIT_TYPEID::PROTOSS_PROBE);
                if (probe_scout == probe_forge) probe_forge = nullptr;
            }//건물 지을 프로브 지정
            if (observation->GetMinerals()>80) {
                Actions()->UnitCommand(probe_scout, ABILITY_ID::MOVE, probe_scout_dest);
                stage_number++;
            }
            return false;
        case 1:
            if (pylons.size()>0) {
                stage_number++;
                return false;
            }
            if (Distance2D(probe_scout->pos, front_expansion)<5) {
                probe_scout_dest = Point2D((double)game_info_.width / 2, (double)game_info_.height / 2);
                Actions()->UnitCommand(probe_scout, ABILITY_ID::MOVE, probe_scout_dest);
            }
            if (Distance2D(probe_scout->pos, front_expansion) > 10 && Distance2D(probe_scout->pos, startLocation_) > 25) {
                probe_scout_dest = probe_scout->pos;
                float rx = GetRandomScalar();
                float ry = GetRandomScalar();
                Point2D build_location = Point2D(probe_scout->pos.x + rx * 3, probe_scout->pos.y + ry * 3);
                if (Query()->PathingDistance(probe_scout, build_location) < 0.1f) {
                    return false;
                }
                if (Query()->Placement(ABILITY_ID::BUILD_PYLON, build_location)) {
                    Actions()->UnitCommand(probe_scout, ABILITY_ID::BUILD_PYLON, build_location);
                }
            }
            return false;
        case 2:
            if (forge_count>0) {
                stage_number++;
                return false;
            }
            if (pylon_first == nullptr && !pylons.empty()) {
                pylon_first = pylons.front();
				pylonlocation = pylon_first->pos;
            }
            if (pylon_first != nullptr && observation->GetMinerals()>100) {
                Actions()->UnitCommand(probe_forge, ABILITY_ID::MOVE, pylon_first->pos);
            }
            if (probe_forge->orders.empty()) {
                TryBuildForge(probe_forge, pylon_first);
            }
            return false;
        case 3:
            if (bases.size()>1) {
                stage_number++;
                return false;
            }
            if (observation->GetMinerals()>400) {
                Actions()->UnitCommand(probe_forge, ABILITY_ID::BUILD_NEXUS, front_expansion);
            }
            if (observation->GetMinerals()>300) {
                Actions()->UnitCommand(probe_forge, ABILITY_ID::MOVE, front_expansion);
            }
            return false;
        case 4:
            if (cannon_count>1 + early * 2) {
                stage_number++;
                return false;
            }
            if (observation->GetMinerals()>150 && !probe_forge->orders.size()) {
                TryBuildStructureNearPylonWithUnit(probe_forge, ABILITY_ID::BUILD_PHOTONCANNON, pylon_first);
            }
            return false;
        case 5:
            if (observation->GetMinerals()>300) {
                if (pylons.size()<2) {
                    TryBuildPylon(staging_location_);
                }
                TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_PROBE);

            }

            if (gateway_count>0) {
                stage_number++;
                return false;
            }
            if (observation->GetMinerals()>150 && !probe_forge->orders.size()) {
                TryBuildStructureNearPylonWithUnit(probe_forge, ABILITY_ID::BUILD_GATEWAY, pylon_first);
            }
            return false;
        case 6:
            if (assimilator_count>1) {
                stage_number++;
                return false;
            }
            if (observation->GetMinerals()>75) {
                TryBuildGas(ABILITY_ID::BUILD_ASSIMILATOR, UNIT_TYPEID::PROTOSS_PROBE, base->pos);
            }
            return false;
        case 7:
            if (pylons.size()>1) {
                stage_number++;
                return false;
            }
            if (observation->GetMinerals()>100) {
                TryBuildPylon(staging_location_);
            }
            return false;
        case 8:
            if (cybernetics_count>0) {
                stage_number++;
                return false;
            }
            if (observation->GetMinerals()>150) {
                TryBuildStructureNearPylon(ABILITY_ID::BUILD_CYBERNETICSCORE, UNIT_TYPEID::PROTOSS_PROBE);
            }
            return false;
        case 9:
            if (cannon_count>3 + early * 2) {
                stage_number++;
                return false;
            }
            if (observation->GetMinerals()>150) {
                TryBuildStructureNearPylonWithUnit(probe_forge, ABILITY_ID::BUILD_PHOTONCANNON, pylon_first);
            }
            return false;
        case 10:
            if (stargate_count>0) {
                stage_number++;
                return false;
            }
            if (observation->GetMinerals()>150 && observation->GetVespene()>150) {
                TryBuildStructureNearPylon(ABILITY_ID::BUILD_STARGATE, UNIT_TYPEID::PROTOSS_PROBE);
            }
            return false;
        case 11:
            if (TryBuildUnitChrono(ABILITY_ID::TRAIN_ORACLE, UNIT_TYPEID::PROTOSS_STARGATE)) {
                OracleTrained = true;
                stage_number++;
            }
            return false;
        case 12:
            if (pylons.size()>2) {
                stage_number++;
                return false;
            }
            if (observation->GetMinerals()>100) {
                TryBuildPylon(Point2D((FindNearestMineralPatch(base->pos)->pos.x + base->pos.x) / 2, (FindNearestMineralPatch(base->pos)->pos.y + base->pos.y) / 2));
            }
            return false;
        case 13:
            if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_FLEETBEACON)>0) {
                stage_number++;
                return false;
            }

            if (observation->GetMinerals()>300 && observation->GetVespene()>200) {
                TryBuildStructureNearPylon(ABILITY_ID::BUILD_FLEETBEACON, UNIT_TYPEID::PROTOSS_PROBE);
            }
            return false;
        case 14:
            if (TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRWEAPONS, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE)) {
                stage_number++;
            }
            return false;
        case 15:
            if (TryBuildUnitChrono(ABILITY_ID::TRAIN_ORACLE, UNIT_TYPEID::PROTOSS_STARGATE)) {
                stage_number++;
            }
            return false;
        case 16:
            if (stargate_count>1) {
                stage_number++;
                return false;
            }
            if (observation->GetMinerals()>150 && observation->GetVespene()>150) {
                TryBuildStructureNearPylon(ABILITY_ID::BUILD_STARGATE, UNIT_TYPEID::PROTOSS_PROBE);
            }
            return false;
        case 17:
            if (cannon_count>5 + early * 2) {
                stage_number++;
                return false;
            }
            if (observation->GetMinerals()>150) {
                TryBuildStructureNearPylon(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE, pylon_first);
            }
            return false;
        case 18:
            if (pylons.size()>3) {
                stage_number++;
                return false;
            }
            for (const auto& b : bases) {
                if (b != base) {
                    TryBuildPylon(FindNearestMineralPatch(b->pos)->pos);
                }
            }
            return false;
        case 19:
            if (TryBuildUnit(ABILITY_ID::RESEARCH_INTERCEPTORGRAVITONCATAPULT, UNIT_TYPEID::PROTOSS_FLEETBEACON)) {
                stage_number++;
            }
            return false;
        case 20:
            if (TryBuildPylonWide(front_expansion)) {
                stage_number++;
            }
            return false;
        case 21:
            for (const auto& pylon : pylons) {
                if (Distance2D(pylon->pos, base->pos)<10) {
                    if (TryBuildStructureNearPylon(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE, pylon)) {
                        stage_number++;
                        return false;
                    }
                }
            }
            return false;
        case 22:
            for (const auto& pylon : pylons) {
                if (Distance2D(pylon->pos, base->pos)<10) {
                    if (TryBuildStructureNearPylon(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE, pylon)) {
                        stage_number++;
                        return false;
                    }
                }
            }
            return false;
        case 23:
            for (const auto& pylon : pylons) {
                if (Distance2D(pylon->pos, front_expansion)<10) {
                    if (TryBuildStructureNearPylon(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE, pylon)) {
                        stage_number++;
                        return false;
                    }
                }
            }
            return false;
        case 24:
            for (const auto& pylon : pylons) {
                if (Distance2D(pylon->pos, front_expansion)<10) {
                    if (TryBuildStructureNearPylon(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE, pylon)) {
                        stage_number++;
                        return false;
                    }
                }
            }
            return false;
        case 25:
            for (const auto& pylon : pylons) {
                if (Distance2D(pylon->pos, front_expansion)<10) {
                    if (TryBuildStructureNearPylon(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE, pylon)) {
                        stage_number++;
                        return false;
                    }
                }
            }
            return false;
        case 26:
            for (const auto& pylon : pylons) {
                if (Distance2D(pylon->pos, front_expansion)<10) {
                    if (TryBuildStructureNearPylon(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE, pylon)) {
                        stage_number++;
                        return false;
                    }
                }
            }
            return false;
            /*case 27 :
            if(TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE)){
            stage_number++;
            return false;
            }

            case 28 :
            for(const auto& b : bases){
            if(b->build_progress<1){
            if(TryBuildPylonWide(b->pos)){
            stage_number++;
            return false;
            }
            }
            }*/
        }



        return false;
    }

	void scoutprobe() {
		const ObservationInterface* observation = Observation();
		if (observation->GetFoodUsed() < 15) return;

		const Unit* mineralp = FindNearestMineralPatch(*iter_exp);
		if (mineralp == nullptr) {
			return;
		}
		Point2D tag_pos = mineralp->pos;

		if (Distance2D(game_info_.enemy_start_locations.front(), tag_pos)<7 || Distance2D(enemy_expansion, tag_pos)<7) {
			iter_exp++;
			return;
		}
		Actions()->UnitCommand(probe_scout, ABILITY_ID::MOVE, tag_pos);
		if (Distance2D(probe_scout->pos, tag_pos)<1) {
			iter_exp++;
		}

	}

    bool early_strategy = false;
    const Unit* probe_scout = nullptr;
    const Unit* pylon_first = nullptr;
    const Unit* probe_forge = nullptr;

    bool find_enemy_location = false;
    std::vector<Point2D>::iterator iter_esl = game_info_.enemy_start_locations.begin();
    std::vector<Point3D>::iterator iter_exp;
    Point3D enemy_expansion;

    uint16_t stage_number = 0;
    const Unit* base = nullptr;
    int max_worker_count_ = 65;
};

class Human : public sc2::Agent {
public:
    void OnGameStart() final {
        Debug()->DebugTextOut("Human");
        Debug()->SendDebug();
    }
};
static bool VsHuman = false;

int main(int argc, char* argv[])
{
    if (VsHuman)
    {
        sc2::Coordinator coordinator;
        if (!coordinator.LoadSettings(argc, argv)) {
            return 1;
        }

        coordinator.SetMultithreaded(true);
        if (VsHuman) {
            //coordinator.SetRealtime(true);
        }

        // Add the custom bot, it will control the players.
        Bot bot1, bot2;
        Human human_bot;

        sc2::Agent* player_one = &bot1;
        if (VsHuman) {
            player_one = &human_bot;
        }

        coordinator.SetParticipants({
            CreateParticipant(sc2::Race::Protoss, player_one),
            CreateParticipant(sc2::Race::Protoss, &bot2),
            });

        // Start the game.
        coordinator.LaunchStarcraft();

        bool do_break = false;
        while (!do_break) {
            if (!coordinator.StartGame("(2)LostandFoundLE.sc2map")) {
                break;
            }
            while (coordinator.Update() && !do_break) {
                if (sc2::PollKeyPress()) {
                    do_break = true;
                }
            }
        }

        bot1.Control()->DumpProtoUsage();
        bot2.Control()->DumpProtoUsage();

        return 0;
    }
    else
    {
        Coordinator coordinator;
        coordinator.LoadSettings(argc, argv);

        Bot bot;
        coordinator.SetParticipants({
            CreateParticipant(Race::Protoss, &bot),
            CreateComputer(Race::Random,Difficulty::CheatInsane)
            });

        coordinator.SetStepSize(10); //Control
                                     //게임속도 빠르게 speed faster
        coordinator.LaunchStarcraft();
        coordinator.StartGame("(2)LostandFoundLE.sc2map");

        while (coordinator.Update()) {
            // Slow down game speed for better look & feel while making experiments.
            //sc2::SleepFor(30);
        }


        return 0;
    }
}