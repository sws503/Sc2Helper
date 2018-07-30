#pragma once
#include "sc2api/sc2_api.h"
#include "sc2lib/sc2_lib.h"
#include "sc2utils/sc2_manage_process.h"

#include <iostream>
#include <string>
#include <algorithm>
#include <iterator>
#include <typeinfo>

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

struct Rusher {
	Rusher(Point2D startLocation_) : sl(startLocation_) {
	}
	bool operator()(const Unit& unit) {
		return unit.unit_type.ToType() == UNIT_TYPEID::PROTOSS_PHOTONCANNON && Distance2D(sl, unit.pos) < 20;
		return unit.unit_type.ToType() == UNIT_TYPEID::PROTOSS_PYLON && Distance2D(sl, unit.pos) < 20;
		return unit.unit_type.ToType() == UNIT_TYPEID::ZERG_HATCHERY && Distance2D(sl, unit.pos) < 20;
		return unit.unit_type.ToType() == UNIT_TYPEID::TERRAN_BUNKER && Distance2D(sl, unit.pos) < 20;
		return unit.unit_type.ToType() == UNIT_TYPEID::TERRAN_BARRACKS && Distance2D(sl, unit.pos) < 20;
	}
private:
	Point2D sl;
};

struct AirAttacker { // 공중 공격 가능한 적들 (폭풍함이 우선 공격하는 적) //시간이 남으면 weapon.type == sc2::Weapon::TargetType::Air 으로 할수있지만 시간이 없음
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

struct AirAttacker2 { // 공중 공격 가능한 적들: 예언자가 기피해야하는 적. 완성이 아직 안 된 건물들은 안 피하도록
	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_PHOTONCANNON:
		case UNIT_TYPEID::TERRAN_MISSILETURRET:
		case UNIT_TYPEID::TERRAN_BUNKER:
		case UNIT_TYPEID::ZERG_SPORECRAWLER:
			return (unit.build_progress == 1.0);

		case UNIT_TYPEID::PROTOSS_STALKER:
		case UNIT_TYPEID::TERRAN_MARINE:
		case UNIT_TYPEID::ZERG_QUEEN:
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

struct IsRanged {
	IsRanged(const ObservationInterface* obs) : observation_(obs) {}

	bool operator()(const Unit& unit) {
		auto Weapon = observation_->GetUnitTypeData().at(unit.unit_type).weapons;
		for (const auto& weapon : Weapon) {
			if (weapon.range < 1.0f) {
				return false;
			}
		}
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_ADEPTPHASESHIFT: return false;
		default: return true;
		}
	}
private:
	const ObservationInterface* observation_;
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
private:
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
private:
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

struct IsMineral {
	bool operator()(const Unit& unit) {
		return unit.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD750 ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD750 ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERMINERALFIELD750 ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERRICHMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERRICHMINERALFIELD750 ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_LABMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_LABMINERALFIELD750 ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_BATTLESTATIONMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_BATTLESTATIONMINERALFIELD750 ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_VESPENEGEYSER || unit.unit_type == UNIT_TYPEID::NEUTRAL_PROTOSSVESPENEGEYSER ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_SPACEPLATFORMGEYSER || unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERVESPENEGEYSER ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_SHAKURASVESPENEGEYSER || unit.unit_type == UNIT_TYPEID::NEUTRAL_RICHVESPENEGEYSER;
	}
};

struct HasBuff {
	HasBuff(BuffID buff): buff_(buff) {};

	bool operator()(const Unit& unit) {
		// is buffed?
		for (const auto& buff : unit.buffs) {
			if (buff == buff_) return true;
		}
		return false;
	}

private:
	const BuffID buff_;
};

class MEMIBot : public Agent {
public:
	virtual void OnGameStart() final {
		game_info_ = Observation()->GetGameInfo();
		std::cout << "Game started!" << std::endl;
		search::ExpansionParameters ep;
		ep.radiuses_.push_back(0.0f);
		ep.radiuses_.push_back(1.0f);
		ep.radiuses_.push_back(2.1f);
		ep.radiuses_.push_back(3.2f);
		ep.radiuses_.push_back(4.3f);
		ep.radiuses_.push_back(4.9f);
		ep.radiuses_.push_back(5.9f);
		expansions_ = search::CalculateExpansionLocations(Observation(), Query(), ep);

		iter_exp = expansions_.begin();

		//Temporary, we can replace this with observation->GetStartLocation() once implemented
		startLocation_ = Observation()->GetStartLocation();
		staging_location_ = startLocation_;

		if (game_info_.enemy_start_locations.size() == 1)
		{
			find_enemy_location = true;

			float minimum_distance = std::numeric_limits<float>::max();
			for (const auto& expansion : expansions_) {
				float Enemy_distance = Distance2D(game_info_.enemy_start_locations.front(), expansion);
				if (Enemy_distance < .01f) {
					continue;
				}

				if (Enemy_distance < minimum_distance) {
					if (Query()->Placement(ABILITY_ID::BUILD_NEXUS, expansion)) {
						Enemy_front_expansion = expansion;
						minimum_distance = Enemy_distance;
					}
				}
			}
			//std::cout << Enemy_front_expansion.x << "  " << Enemy_front_expansion.y << "  " << Enemy_front_expansion.z << std::endl;
		}


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
		//std::cout << front_expansion.x << "  " << front_expansion.y << "  " << front_expansion.z << std::endl;
		staging_location_ = Point3D(((staging_location_.x + front_expansion.x) / 2), ((staging_location_.y + front_expansion.y) / 2),
			((staging_location_.z + front_expansion.z) / 2));
	}

	virtual void OnStep() final {



		const ObservationInterface* observation = Observation();
		ActionInterface* action = Actions();

		Units units = observation->GetUnits(Unit::Self, IsArmy(observation));
		ConvertGateWayToWarpGate();


		ManageWorkers(UNIT_TYPEID::PROTOSS_PROBE);

		if (!early_strategy) {
			EarlyStrategy();
		}
		if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_GATEWAY)>0 && iter_exp < expansions_.end() && find_enemy_location == true) {
			scoutprobe();
		}

		ManageUpgrades();

		// Control 시작
		Defend();
		//ManageArmy();
		ManageRush();


		TryChronoboost(IsUnit(UNIT_TYPEID::PROTOSS_STARGATE));
		//TryChronoboost(IsUnit(UNIT_TYPEID::PROTOSS_CYBERNETICSCORE));
		//TryChronoboost(IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));
	}

	virtual void OnUnitIdle(const Unit* unit) override {
		switch (unit->unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_PROBE: {
			MineIdleWorkers(unit);
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
    void OnUpgradeCompleted(UpgradeID upgrade) {
        switch (upgrade.ToType()) {
            case UPGRADE_ID::BLINKTECH: {
				std::cout << "BLINK UPGRADE DONE!!";
				BlinkResearched = true;
				return;
			}
            case UPGRADE_ID::WARPGATERESEARCH: {
                warpgate_researched = true;
                return;
            }
            default:
                break;
        }
    }

	GameInfo game_info_;
	std::vector<Point3D> expansions_;
	Point3D startLocation_;
	Point3D staging_location_;

	Point3D front_expansion;
	Point3D Enemy_front_expansion;

	Point2D RushLocation;
	Point2D EnemyLocation;
	Point2D ReadyLocation1;
	Point2D ReadyLocation2;
	Point2D KitingLocation;


	float base_range = 35;

private:
	void Chat(std::string Message) // 6.29 채팅 함수
	{
#ifdef DEBUG
		Actions()->SendChat(Message);
#endif
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
	bool PhotonRush;
	void Defend();

	void SetupRushLocation(const ObservationInterface *observation)
	{
		if (find_enemy_location) {
			ReadyLocation1 = game_info_.enemy_start_locations.front() + Point2D(30.0f, 0.0f);
			ReadyLocation2 = game_info_.enemy_start_locations.front() + Point2D(0.0f, 30.0f);
		}
		else {
			ReadyLocation1 = startLocation_;
			ReadyLocation2 = startLocation_;
		}
	}

	float OracleRange = 3.0; // 절대적으로 생존
	float TempestRange = 3.0f;
	float CarrierRange = 3.0f;
	bool TimetoAttack = false;
	bool OracleTrained = false;
	Point2D pylonlocation;
	const Unit* oracle_second = nullptr;
	Point2D StasisLocation;

	Units Killers;
	const Unit* WorkerKiller = nullptr;
	const Unit* oracle_first = nullptr;

	void ManageRush();

	void AdeptPhaseShift(const Unit * unit, Units ShadeNearEnemies, Units NearbyEnemies, bool & ComeOn);

	void AdeptPhaseToLocation(const Unit * unit, Point2D Location, bool & Timer, bool & ComeOn);

	void ManageBlink(const Unit * unit, const Unit * enemyarmy);

	void StalkerBlinkEscape(const Unit * unit, const Unit * enemyarmy);

	void StalkerBlinkForward(const Unit * unit, const Unit * enemyarmy);

	void FrontKiting(const Unit * unit, const Unit * enemyarmy);

	void ComeOnKiting(const Unit * unit, const Unit * enemyarmy);

	void Kiting(const Unit * unit, const Unit * enemyarmy);

	void KiteEnemy(const Unit * unit, Units enemy_army, Units enemy_units, Point2D KitingLocation, bool enemiesnear, const ObservationInterface * observation);


	float MinimumDistance2D(const Unit * unit, const Unit * enemyarmy);


	Point2D CalcKitingPosition(Point2D Mypos, Point2D EnemyPos);

	bool GetPosition(Units Enemyunits, Unit::Alliance alliace, Point2D & position);

	bool GetPosition(UNIT_TYPEID unit_type, Unit::Alliance alliace, Point2D & position);

	int getAttackPriority(const Unit * u);

	const Unit * GetHighPrioTarget(const Unit * rangedUnit, Units & targets);

	const Unit * GetTarget(const Unit * rangedUnit, Units & targets);

	const float getDpsGROUND(const Unit * target) const;

	const float getAttackRangeGROUND(const Unit * target) const;

	void RetreatWithCarrier(const Unit* unit) {
		if (pylonlocation != Point2D(0, 0))
			Actions()->UnitCommand(unit, ABILITY_ID::PATROL, pylonlocation);
	}

	void RetreatWithVoidray(const Unit* unit) {
			Actions()->UnitCommand(unit, ABILITY_ID::PATROL, startLocation_);
	}
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
		return groundWeapons.range; //지상유닛을 위한 함수!!
									 // return groundWeapons.range; // 7.5 사용하고 싶으면 쓸것
	}

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

		if (FindEnemyPosition(target_pos)) { //적 기지를 알고있는 상황이면
			if (Distance2D(unit->pos, target_pos) < 20 && enemy_units.empty()) { //적 유닛이 없는 상황에서 적 기지가 근처에 있으면
				if (TryFindRandomPathableLocation(unit, target_pos)) { //유닛별로 맵 전체적으로 퍼지는 위치를 배정받고
					Actions()->UnitCommand(unit, ABILITY_ID::SMART, target_pos); //그 위치로 간다
					return;
				}
			}
			else if (!enemy_units.empty()) // 적 유닛이 있는 상황이면 또는 이 유닛이 적 기지 근처에 없는상황이면
			{
				Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front()); //적 유닛을 공격하러 간다
				return;
			}
			// TODO : 가장 마지막으로 본 적의 위치를 target_pos 로 리턴하는 함수를 만들자

			Actions()->UnitCommand(unit, ABILITY_ID::SMART, target_pos); //위 작업이 끝나면 적 기지를 다시한번 간다
		}
		else { //적 기지도 모르면 막 돌아다녀라
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
		return find_enemy_location;
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


	bool TryBuildUnitChrono(AbilityID ability_type_for_unit, UnitTypeID unit_type) {
		const ObservationInterface* observation = Observation();

		//If we are at supply cap, don't build anymore units, unless its an overlord.
		if (observation->GetFoodUsed() >= observation->GetFoodCap() && ability_type_for_unit != ABILITY_ID::TRAIN_OVERLORD) {
			return false;
		}
		const Unit* unit = nullptr;
		Units target_units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
		for (const auto& candidate_unit : target_units) {
			// is completely built?
			if (candidate_unit->build_progress != 1.0f) continue;
			// is doing something?
			if (!candidate_unit->orders.empty()) continue;

			unit = candidate_unit;
			// pick prioritized structures first
			if (!HasBuff(BUFF_ID::TIMEWARPPRODUCTION)(*unit)) {
				break;
			}
		}
		if (unit == nullptr) {
			return false;
		}
		Actions()->UnitCommand(unit, ability_type_for_unit);
		Chronoboost(unit);
		return true;
	}

	void TryChronoboost(Filter f = {}) {
		const ObservationInterface* observation = Observation();
		Units structures = observation->GetUnits(Unit::Alliance::Self, f);
		for (const auto& structure : structures) {
			TryChronoboost(structure);
		}
	}

	bool TryChronoboost(const Unit * unit) {
		const ObservationInterface* observation = Observation();
		// is structure?
		if (!IsStructure(observation)(*unit)) return false;
			// is completely built?
		if (unit->build_progress != 1.0f) return false;
		// is doing nothing?
		if (unit->orders.empty()) return false;
		// is already buffed?
		if (HasBuff(BUFF_ID::TIMEWARPPRODUCTION)(*unit)) return false;
		Chronoboost(unit);
		return true;
	}

	bool Chronoboost(const Unit * unit) {
		const ObservationInterface* observation = Observation();
		Units nexuses = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));

		if (nexuses.empty()) return false;
		for (const auto& nexus : nexuses) {
			if (nexus->build_progress != 1.0f) {
				continue;
			}
			if (nexus->energy >= 50) {
				Actions()->UnitCommand(nexus, 3755, unit);
				return true;
			}
		}
		return false;
	}


	size_t CountUnitType(const ObservationInterface* observation, UnitTypeID unit_type) {
		return observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
	}

	size_t CountUnitTypeNearLocation(const ObservationInterface* observation, UnitTypeID unit_type, Point2D location, float radius = 10.0f) {
		Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
		int number=0;
		for (const auto& u : units){
            if (Distance2D(u->pos,location)<=radius) {
                number++;
            }
		}
		return number;
	}

	bool GetRandomUnit(const Unit*& unit_out, const ObservationInterface* observation, UnitTypeID unit_type) {
		Units my_units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
		int num = static_cast<int>(my_units.size());
		if (num == 0) return false;
		unit_out = my_units[GetRandomInteger(0, num - 1)];
		return true;
	}

	void ConvertGateWayToWarpGate() {
        const ObservationInterface* observation = Observation();
        Units gateways = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_GATEWAY));

        if (warpgate_researched) {
            for (const auto& gateway : gateways) {
                if (gateway->build_progress == 1) {
                    Actions()->UnitCommand(gateway, ABILITY_ID::MORPH_WARPGATE);
                }
            }
        }
    }

	const Unit* FindNearestMineralPatch(const Point2D& start) {
		Units units = Observation()->GetUnits(Unit::Alliance::Neutral);
		float distance = std::numeric_limits<float>::max();
		const Unit* target = nullptr;
		for (const auto& u : units) {
			if (IsMineral()(*u)) {
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

	bool TryBuildUnit(AbilityID ability_type_for_unit, UnitTypeID unit_type, Filter priority_filter = {}) {
		const ObservationInterface* observation = Observation();

		//If we are at supply cap, don't build anymore units, unless its an overlord.
		if (observation->GetFoodUsed() >= observation->GetFoodCap() && ability_type_for_unit != ABILITY_ID::TRAIN_OVERLORD) {
			return false;
		}
		const Unit* unit = nullptr;
		Units target_units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
		for (const auto& candidate_unit : target_units) {
			// is completely built?
			if (candidate_unit->build_progress != 1.0f) continue;
			// is doing something?
			if (!candidate_unit->orders.empty()) continue;

			unit = candidate_unit;
			// pick prioritized structures first
			if (!priority_filter || priority_filter(*candidate_unit)) {
				break;
			}
		}
		if (unit == nullptr) {
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

		// If no worker is already building one, get a nearest worker to build one
		Tag builder_tag = NullTag;
		std::vector<QueryInterface::PathingQuery> query_vector;
		for (const auto& worker : workers) {
			if (worker == probe_scout && !probe_scout->orders.empty()) continue;
			// consider idle or mining workers only
			if (!worker->orders.empty()) {
				auto abilityid = worker->orders.front().ability_id;
				if (abilityid != ABILITY_ID::HARVEST_GATHER &&
					abilityid != ABILITY_ID::HARVEST_RETURN) {
					continue;
				}
			}
			// add queries
			QueryInterface::PathingQuery query;
			query.start_unit_tag_ = worker->tag;
			query.start_ = worker->pos;
			query.end_ = location;
			query_vector.push_back(query);
		}
		size_t size = query_vector.size();
		if (size == 0) return false;
		std::vector<float> distances = Query()->PathingDistance(query_vector);

		// find workers that are nearest.
		float min_distance = std::numeric_limits<float>::max();
		for (size_t i = 0; i < size; i++) {
			float d = distances.at(i);
			// Check to see if unit can make it there
			if (d < 0.1f) {
				continue;
			}
			if (d < min_distance) {
				min_distance = d;
				builder_tag = query_vector.at(i).start_unit_tag_;
			}
		}
		if (builder_tag == NullTag) return false;
		const Unit* builder = observation->GetUnit(builder_tag);
		if (builder == nullptr) {
			return false;
		}


		if (!isExpansion) {
			for (const auto& expansion : expansions_) {
				if (Distance2D(location, Point2D(expansion.x, expansion.y)) < 7) {
					return false;
				}
			}
		}
		std::vector<UnitOrder> builder_orders;
		for (auto& o : builder->orders) {
			builder_orders.push_back(o);
		}
		// Check to see if unit can build there
		if (Query()->Placement(ability_type_for_structure, location)) {
			Actions()->UnitCommand(builder, ability_type_for_structure, location);
			Actions()->UnitCommand(builder, ABILITY_ID::HARVEST_RETURN, true);
			for (auto& o : builder_orders) {
				if (o.target_unit_tag != NullTag) {
					Actions()->UnitCommand(builder, o.ability_id, observation->GetUnit(o.target_unit_tag), true);
				}
				else if (o.target_pos != Point2D(0.0f, 0.0f)) {
					Actions()->UnitCommand(builder, o.ability_id, o.target_pos, true);
				}
				else {
					Actions()->UnitCommand(builder, o.ability_id, true);
				}
			}
			return true;
		}
		return false;

	}

	bool BuildGas(Tag geyser_tag) {
		const ObservationInterface* observation = Observation();
		Units workers = observation->GetUnits(Unit::Alliance::Self, IsWorker());
		const Unit* geyser = observation->GetUnit(geyser_tag);

		if (workers.empty()) {
			return false;
		}

		// Check to see if there is already a worker heading out to build it
		for (const auto& worker : workers) {
			for (const auto& order : worker->orders) {
				if (order.target_unit_tag == geyser_tag &&
					   (order.ability_id == ABILITY_ID::BUILD_ASSIMILATOR ||
						order.ability_id == ABILITY_ID::BUILD_REFINERY ||
						order.ability_id == ABILITY_ID::BUILD_EXTRACTOR)) {
					return false;
				}
			}
		}
		float minimum_distance = 15.0f;
		const Unit* builder = nullptr;

		// find nearest worker near geyser
		for (const auto& worker : workers) {
			if (worker == probe_scout || worker == probe_forward) continue;
			// consider idle or mining workers only
			if (!worker->orders.empty()) {
				auto abilityid = worker->orders.front().ability_id;
				if (abilityid != ABILITY_ID::HARVEST_GATHER &&
					abilityid != ABILITY_ID::HARVEST_RETURN) {
					continue;
				}
			}
			float distance = Distance2D(worker->pos, geyser->pos);

			if (distance < minimum_distance)
			{
				minimum_distance = distance;
				builder = worker;
			}
		}
		if (builder == nullptr) {
			builder = GetRandomEntry(workers);
		}

		ABILITY_ID build_ability = ABILITY_ID::INVALID;
		if (IsUnit(UNIT_TYPEID::PROTOSS_PROBE)(*builder)) {
			build_ability = ABILITY_ID::BUILD_ASSIMILATOR;
		}
		else if (IsUnit(UNIT_TYPEID::TERRAN_SCV)(*builder)) {
			build_ability = ABILITY_ID::BUILD_REFINERY;
		}
		else if (IsUnit(UNIT_TYPEID::ZERG_DRONE)(*builder)) {
			build_ability = ABILITY_ID::BUILD_EXTRACTOR;
		}
		else {
			return false;
		}

		// Check to see if unit can build there
		if (Query()->Placement(build_ability, geyser->pos)) {
			Actions()->UnitCommand(builder, build_ability, geyser);
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
		if (advance_pylon !=nullptr && Distance2D(random_power_source.position,advance_pylon->pos)<10) {
            return false;
		}
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

		if (abs(location.x - front_expansion.x) < 3.99f || abs(location.y - front_expansion.y) < 3.99f) {
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

	bool TryBuildGas(Point2D base_location) {
		const ObservationInterface* observation = Observation();
		Units geysers = observation->GetUnits(Unit::Alliance::Neutral, IsVespeneGeyser());
		ABILITY_ID build_ability = ABILITY_ID::BUILD_ASSIMILATOR;

		//only search within this radius
		float minimum_distance = 15.0f;
		Tag closestGeyser = NullTag;
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
		if (closestGeyser == NullTag) {
			return false;
		}

		return BuildGas(closestGeyser);
	}

	bool TryBuildPylonIfNeeded(size_t MaxBuildAtOnce = 1) {
		if (MaxBuildAtOnce < 1) MaxBuildAtOnce = 1;
		const ObservationInterface* observation = Observation();

		int i;
		for (i = 0; i < MaxBuildAtOnce; i++) {
			// If we are not supply capped, don't build a supply depot. (test for (i+1) pylons)
			int margin = (i + 1) * 8 - 2;
			if (observation->GetFoodUsed() < observation->GetFoodCap() - margin) {
				break;
			}
		}
		if (i == 0) return false;
		if (observation->GetFoodCap() == 200) return false;

		if (TryBuildPylon(startLocation_, 15.0f, i)) return true;
		Units bases = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));
		for (const auto& b : bases) {
			if (TryBuildPylon(b->pos, 7.0f, i)) return true;
		}
		return false;
	}
	bool TryBuildPylon(Point2D location, float radius = 3.0f, size_t MaxBuildAtOnce = 1) {
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

	bool TrybuildFirstPylon() {
        const ObservationInterface* observation = Observation();

        Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PYLON));

        if (observation->GetMinerals() < 100) {
			return false;
		}

		if (units.size()>0) {
            return false;
		}

		float x = 8.0f;
		float y = 8.0f;
		if ((float)game_info_.width/2 < startLocation_.x) {
            x = -8.0f;
		}
		if ((float)game_info_.height/2 < startLocation_.y) {
            y = -8.0f;
		}
		Point2D build_location = Point2D(startLocation_.x + x, startLocation_.y + y);
        return TryBuildStructure(ABILITY_ID::BUILD_PYLON, UNIT_TYPEID::PROTOSS_PROBE, build_location);
	}

	void MineIdleWorkers(const Unit* worker) {
		if (worker == probe_scout || worker == probe_forward) return;

		const ObservationInterface* observation = Observation();
		Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
		Units geysers = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_ASSIMILATOR));

		const Unit* valid_mineral_patch = nullptr;

		if (bases.empty()) {
			return;
		}

		for (const auto& geyser : geysers) {
			if (geyser->build_progress != 1.0) continue;
			if (geyser->assigned_harvesters < geyser->ideal_harvesters) {
				Actions()->UnitCommand(worker, ABILITY_ID::HARVEST_GATHER, geyser);
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
				Actions()->UnitCommand(worker, ABILITY_ID::HARVEST_GATHER, valid_mineral_patch);
				return;
			}
		}

		if (!worker->orders.empty()) {
			return;
		}

		//If all workers are spots are filled just go to any base.
		const Unit* random_base = GetRandomEntry(bases);
		valid_mineral_patch = FindNearestMineralPatch(random_base->pos);
		Actions()->UnitCommand(worker, ABILITY_ID::HARVEST_GATHER, valid_mineral_patch);
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

	void ManageWorkers(UNIT_TYPEID worker_type) {
		const ObservationInterface* observation = Observation();
		Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
		Units geysers = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_ASSIMILATOR));

		if (bases.empty()) {
			return;
		}

		for (const auto& base : bases) {
			//If we have already mined out or still building here skip the base.
			if (base->ideal_harvesters == 0 || base->build_progress != 1.0f) {
				continue;
			}
			//if base is
			if (base->assigned_harvesters > base->ideal_harvesters) {
				Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(worker_type));

				for (const auto& worker : workers) {
					if (worker == probe_scout || worker == probe_forward) continue;
					if (!worker->orders.empty()) {
						if (worker->orders.front().target_unit_tag == base->tag){
							//This should allow them to be picked up by mineidleworkers()
							MineIdleWorkers(worker);
							return;
						}
					}
				}
			}
		}
		Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(worker_type));

		for (const auto& geyser : geysers) {
			if (geyser->ideal_harvesters == 0 || geyser->build_progress != 1.0f) {
				for (const auto& worker : workers) {
					if (!worker->orders.empty()) {
						if (worker->orders.front().target_unit_tag == geyser->tag) {
							//This should allow them to be picked up by mineidleworkers()
							MineIdleWorkers(worker);
						}
					}
				}
				continue;
			}
			if (geyser->assigned_harvesters > geyser->ideal_harvesters) {
				for (const auto& worker : workers) {
					if (worker == probe_scout || worker == probe_forward) continue;
					if (!worker->orders.empty()) {
						if (worker->orders.front().target_unit_tag == geyser->tag) {
							//This should allow them to be picked up by mineidleworkers()
							MineIdleWorkers(worker);
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
						if (target->unit_type != UNIT_TYPEID::PROTOSS_ASSIMILATOR && target->unit_type != UNIT_TYPEID::PROTOSS_NEXUS) {
							//This should allow them to be picked up by mineidleworkers()
							MineIdleWorkers(worker);
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
		TryBuildUnit(ABILITY_ID::RESEARCH_ADEPTRESONATINGGLAIVES, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
		TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSGROUNDWEAPONS, UNIT_TYPEID::PROTOSS_FORGE);
		for (const auto& upgrade : upgrades) {
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
			if (base->build_progress == 1.0 && base->assigned_harvesters >= base->ideal_harvesters) {
				if (TryBuildGas(base->pos)) {
					return true;
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
				if (observation->GetMinerals() >= 50 && observation->GetFoodCap() - observation->GetFoodUsed() >= 1) {
					return TryBuildUnitChrono(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);

				}

			}

		}
		return false;

	}

	bool EarlyStrategy();

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

	bool TryWarpAdept(){
        const ObservationInterface* observation = Observation();
        std::vector<PowerSource> power_sources = observation->GetPowerSources();
        Units warpgates = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_WARPGATE));

        if (power_sources.empty()) {
            return false;
        }

        const PowerSource& random_power_source = GetRandomEntry(power_sources);

        float radius = random_power_source.radius;
        float rx = GetRandomScalar();
        float ry = GetRandomScalar();
        Point2D build_location = Point2D(advance_pylon->pos.x + rx * radius, advance_pylon->pos.y + ry * radius);


        if (Query()->PathingDistance(build_location, game_info_.enemy_start_locations.front())) {
            return false;
        }

        for (const auto& warpgate : warpgates) {
            //Actions()->UnitCommand(warpgate, ABILITY_ID::TRAINWARP_ADEPT, build_location);
            if (warpgate->build_progress == 1) {
                AvailableAbilities abilities = Query()->GetAbilitiesForUnit(warpgate);
                for (const auto& ability : abilities.abilities) {
                    if (ability.ability_id == ABILITY_ID::TRAINWARP_ADEPT) {
                        Actions()->UnitCommand(warpgate, ABILITY_ID::TRAINWARP_ADEPT, build_location);
                        return true;
                    }
                }
            }
        }
        return false;
    }
    bool TryWarpStalker(){
        const ObservationInterface* observation = Observation();
        std::vector<PowerSource> power_sources = observation->GetPowerSources();
        Units warpgates = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_WARPGATE));

        if (power_sources.empty()) {
            return false;
        }

        const PowerSource& random_power_source = GetRandomEntry(power_sources);
        if (Distance2D(random_power_source.position,front_expansion)>20) {
            return false;
        }

        float radius = random_power_source.radius;
        float rx = GetRandomScalar();
        float ry = GetRandomScalar();
        Point2D build_location = Point2D(random_power_source.position.x + rx * radius, random_power_source.position.y + ry * radius);


        if (Query()->PathingDistance(build_location, game_info_.enemy_start_locations.front())) {
            return false;
        }

        for (const auto& warpgate : warpgates) {
            //Actions()->UnitCommand(warpgate, ABILITY_ID::TRAINWARP_ADEPT, build_location);
            if (warpgate->build_progress == 1) {
                AvailableAbilities abilities = Query()->GetAbilitiesForUnit(warpgate);
                for (const auto& ability : abilities.abilities) {
                    if (ability.ability_id == ABILITY_ID::TRAINWARP_STALKER) {
                        Actions()->UnitCommand(warpgate, ABILITY_ID::TRAINWARP_STALKER, build_location);
                        return true;
                    }
                }
            }
        }
        return false;
    }


	bool early_strategy = false;
	bool warpgate_researched = false;
	bool BlinkResearched = false;
	const Unit* advance_pylon = nullptr;
	const Unit* probe_scout = nullptr;
	const Unit* pylon_first = nullptr;
	const Unit* probe_forge = nullptr;
	const Unit* probe_forward = nullptr;
	Point2D probe_scout_dest = Point2D(0,0);
	Point2D advance_pylon_location = Point2D((float)game_info_.width/2,(float)game_info_.height/2);

	bool find_enemy_location = false;
	std::vector<Point2D>::iterator iter_esl = game_info_.enemy_start_locations.begin();
	std::vector<Point3D>::iterator iter_exp;
	Point3D enemy_expansion;

	uint16_t stage_number = 0;
	uint16_t branch = 1;
	const Unit* base = nullptr;
	const size_t max_worker_count_ = 65;
	/*
	bool GetAllUnitsNear(Units& units, const Unit* unit, ) {
		for (unit)
	}*/
};
