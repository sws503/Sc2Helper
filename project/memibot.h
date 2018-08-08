#pragma once
#include "sc2api/sc2_api.h"
#include "sc2lib/sc2_lib.h"
#include "sc2utils/sc2_manage_process.h"

#include <iostream>
#include <string>
#include <algorithm>
#include <iterator>
#include <typeinfo>
#include <list>
#include <unordered_map>
#include <stack>
#include "flag.h"
//#include "balance_unit.h"

static inline float round_to_halfint(float f) {
	return float(std::floor(double(f))+0.5);
}

static inline float round_to_int(float f) {
	return float(std::round(double(f)));
}

static inline float building_abs(float location, float expansion, bool IsLocEven=false, bool IsMineral=false) {
	float f1 = IsLocEven ? round_to_int(location) : round_to_halfint(location);
	float f2 = IsMineral ? round_to_int(expansion) : round_to_halfint(expansion);
	return abs(f1 - f2);
}

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
	THERMALLANCES = 5, // 거신
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
		return (unit.unit_type.ToType() == UNIT_TYPEID::PROTOSS_PHOTONCANNON ||
			unit.unit_type.ToType() == UNIT_TYPEID::PROTOSS_PYLON ||
			unit.unit_type.ToType() == UNIT_TYPEID::ZERG_HATCHERY ||
			unit.unit_type.ToType() == UNIT_TYPEID::TERRAN_BUNKER ||
			unit.unit_type.ToType() == UNIT_TYPEID::TERRAN_BARRACKS)
			&& Distance2D(sl, unit.pos) < 20;
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
			return (unit.build_progress >= 0.95);

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

struct IsRanged {
	IsRanged(const ObservationInterface* obs) : observation_(obs) {}

	bool operator()(const Unit& unit) {
		auto Weapon = observation_->GetUnitTypeData().at(unit.unit_type).weapons;
		for (const auto& weapon : Weapon) {
			if (weapon.range > 2.0f) {
				return true;
			}
		}
		switch (unit.unit_type.ToType()) {
		default: return false;
		}
	}
private:
	const ObservationInterface* observation_;
};

struct IsWorker {
	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_PROBE: return true;
		case UNIT_TYPEID::ZERG_DRONE: return true;
		case UNIT_TYPEID::ZERG_DRONEBURROWED: return true;
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
		case UNIT_TYPEID::PROTOSS_WARPPRISM: return false;
		case UNIT_TYPEID::PROTOSS_WARPPRISMPHASING: return false;
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
		case UNIT_TYPEID::TERRAN_COMMANDCENTERFLYING: return true;
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
			unit.unit_type == UNIT_TYPEID::NEUTRAL_BATTLESTATIONMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_BATTLESTATIONMINERALFIELD750;
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

struct IsUnpowered {

	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_GATEWAY:
		case UNIT_TYPEID::PROTOSS_PHOTONCANNON:
		case UNIT_TYPEID::PROTOSS_SHIELDBATTERY:
		case UNIT_TYPEID::PROTOSS_FORGE:
		case UNIT_TYPEID::PROTOSS_CYBERNETICSCORE:
		case UNIT_TYPEID::PROTOSS_STARGATE:
		case UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY:
		case UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL:
		case UNIT_TYPEID::PROTOSS_FLEETBEACON:
		case UNIT_TYPEID::PROTOSS_DARKSHRINE:
		case UNIT_TYPEID::PROTOSS_ROBOTICSBAY:
		case UNIT_TYPEID::PROTOSS_TEMPLARARCHIVE:
		case UNIT_TYPEID::PROTOSS_WARPGATE:
			return !(unit.is_powered);
		default:
			return false;
		}
	}
};

class MEMIBot : public Agent {
public:


	MEMIBot(std::string botname, std::string version)
		: botname(botname), version(version) {}

	virtual void OnGameStart() final override {
		game_info_ = Observation()->GetGameInfo();
		std::cout << "Game started!" << std::endl;
		ChatVersion();
		search::ExpansionParameters ep;
		ep.radiuses_.push_back(1.0f);
		ep.radiuses_.push_back(2.1f);
		ep.radiuses_.push_back(3.2f);
		ep.radiuses_.push_back(4.3f);
		ep.radiuses_.push_back(4.9f);
		ep.radiuses_.push_back(5.9f);
		expansions_ = search::CalculateExpansionLocations(Observation(), Query(), ep);

        Observation()->GetUnitTypeData();


		//상대 종족
		branch = 2;
		for (const auto& p : game_info_.player_info) {
            if(p.race_requested == Race::Terran) {
                branch = 5;
            }
		}

		stage_number = 0;
		iter_exp = expansions_.begin();
		advance_pylon_location = Point2D((float)game_info_.width / 2, (float)game_info_.height / 2);
		Enemy_front_expansion = Point3D(0, 0, 0);
		recent_probe_scout_location = Point2D(0, 0);
		recent_probe_scout_loop = 0;

		early_strategy = false;
		warpgate_researched = false;
		BlinkResearched = false;
		timing_attack = false;
		advance_pylon = nullptr;
		probe_scout = nullptr;
		pylon_first = nullptr;
		probe_forward = nullptr;
		base = nullptr;
		find_enemy_location = false;
		work_probe_forward = true;

		last_map_renewal = 0;
		resources_to_nearest_base.clear();

		flags.reset();

		enemy_units_scouter_seen.clear();
		enemy_townhalls_scouter_seen.clear();
		adept_map.clear();

		//Temporary, we can replace this with observation->GetStartLocation() once implemented
		startLocation_ = Observation()->GetStartLocation();
		staging_location_ = startLocation_;

		float minimum_distance = std::numeric_limits<float>::max();
		for (const auto& expansion : expansions_) {
			float current_distance = Distance2D(startLocation_, expansion);
			if (current_distance < 5.0f) {
				continue;
			}

			if (current_distance < minimum_distance) {
				if (Query()->Placement(ABILITY_ID::BUILD_NEXUS, expansion)) {
					front_expansion = expansion;
					minimum_distance = current_distance;
				}
			}
		}

		// 본진 좌표가 (0,0)으로 나오는 것 수정.
		for (auto& e : expansions_) {
			if (Point2D(e) == Point2D(0, 0)) {
				e.x = startLocation_.x;
				e.y = startLocation_.y;
				break;
			}
		}

		for (const auto& e : expansions_) {
			std::cout << e.x << ", " << e.y << ", " << e.z << std::endl;
		}

		staging_location_ = Point3D(((staging_location_.x + front_expansion.x) / 2), ((staging_location_.y + front_expansion.y) / 2),
			((staging_location_.z + front_expansion.z) / 2));
	}

	virtual void OnStep() final override {


		const ObservationInterface* observation = Observation();
		ActionInterface* action = Actions();

		Units units = observation->GetUnits(Unit::Self, IsArmy(observation));
		if (warpgate_researched) {
            ConvertGateWayToWarpGate();
		}

		ManageWorkers(UNIT_TYPEID::PROTOSS_PROBE);

		if (!early_strategy && observation->GetGameLoop()%10==0) {
			EarlyStrategy();
		}

		scout_all();

#ifdef DEBUG
		//TEST 하려고 송우석이 주석처리함
		//발견시 주석 제거 추천
		//PrintCursor();
#endif

		ManageUpgrades();

		// Control 시작
		Defend();
		//ManageArmy();
		ManageRush();

		//TryChronoboost(IsUnit(UNIT_TYPEID::PROTOSS_STARGATE));
		//TryChronoboost(IsUnit(UNIT_TYPEID::PROTOSS_CYBERNETICSCORE));
		//TryChronoboost(IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));
	}

	virtual void OnUnitIdle(const Unit* unit) final override {
		switch (unit->unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_PROBE: {
			if (probe_scout != nullptr && probe_scout->tag == unit->tag) {
				return;
			}
			if (probe_forward != nullptr && probe_forward->tag == unit->tag && !work_probe_forward) {
				if (EnemyRush) {
					SmartMove(unit, startLocation_);
				}
				// goto near base or center of mass
				else {
					const ObservationInterface* observation = Observation();
					Units bases = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));
					Units mystructures = observation->GetUnits(Unit::Alliance::Self, IsStructure(observation));
					size_t mystructures_size = mystructures.size();
					bool nearbase = false;
					for (const auto& e : bases) {
						if (nearbase |= (DistanceSquared2D(e->pos, unit->pos) < 200)) break;
					}
					if (nearbase) return;
					Point2D avg(0, 0);
					if (mystructures_size) {
						for (const auto& e : mystructures) {
							avg += e->pos;
						}
						avg /= static_cast<float>(mystructures_size);
					}
					if (DistanceSquared2D(probe_forward->pos, avg) > 200 && advance_pylon != nullptr) {
						SmartMove(unit, avg);
					}
				}
				return;
			}
			if (IsCarryingMinerals(*unit) || IsCarryingVespene(*unit)) {
				Actions()->UnitCommand(unit, ABILITY_ID::HARVEST_RETURN);
			}
			else {
				MineIdleWorkers(unit);
			}
			break;
		}
		case UNIT_TYPEID::PROTOSS_CARRIER: {

			break;
		}
		default: {
			break;
		}
		}
		return;
	}

    virtual void OnUpgradeCompleted(UpgradeID upgrade) final override {
        switch (upgrade.ToType()) {
            case UPGRADE_ID::BLINKTECH: {
				std::cout << "BLINK UPGRADE DONE!!";
				BlinkResearched = true;
				return;
			}
			case UPGRADE_ID::EXTENDEDTHERMALLANCE: {
				std::cout << "COLOSSUS UPGRADE DONE!!";
				ColossusRangeUp = true;
				return;
			}
			case UPGRADE_ID::PROTOSSGROUNDWEAPONSLEVEL1: {
				std::cout << "attack1";
				timing_attack = true;
				return;
			}
			case UPGRADE_ID::PROTOSSGROUNDWEAPONSLEVEL2: {
				std::cout << "attack2";
				timing_attack = true;
				Attackers.clear();
				return;
			}
			case UPGRADE_ID::PROTOSSGROUNDWEAPONSLEVEL3: {
				std::cout << "attack3";
				timing_attack = true;
				Attackers.clear();
				return;
			}
			case UPGRADE_ID::PROTOSSGROUNDARMORSLEVEL1: {
				std::cout << "attack3";
				timing_attack = true;
				Attackers.clear();
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

	virtual void OnBuildingConstructionComplete(const Unit* u) final override {
		std::cout << UnitTypeToName(u->unit_type.ToType()) << std::endl;
		if (u->alliance == Unit::Alliance::Self) {
			switch (u->unit_type.ToType()) {
			default:
				break;
			}
		}
	}

	// only allies. also geyser probes, passengers etc.
	virtual void OnUnitCreated(const Unit* u) final override {
		switch (u->unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_ADEPT:
			num_adept++;
			break;
		case UNIT_TYPEID::PROTOSS_STALKER:
			num_stalker++;
			break;

		default:

			break;
		}
	}

	virtual void OnUnitDestroyed(const Unit* u) final override {
		std::cout << UnitTypeToName(u->unit_type.ToType()) << std::endl;
		if (u->alliance == Unit::Alliance::Self) {
			switch (u->unit_type.ToType()) {
			default:
				break;
			}
		}
		if (u->alliance == Unit::Alliance::Enemy) {
			for (auto& it = enemy_units_scouter_seen.begin(); it != enemy_units_scouter_seen.end(); ++it) {
				if ((*it)->tag == u->tag) {
					enemy_units_scouter_seen.erase(it);
					break;
				}
			}
			for (auto& it = enemy_townhalls_scouter_seen.begin(); it != enemy_townhalls_scouter_seen.end(); ++it) {
				if ((*it)->tag == u->tag) {
					enemy_townhalls_scouter_seen.erase(it);
					break;
				}
			}
		}
	}

	// only enemies: add buildings we saw
	virtual void OnUnitEnterVision(const Unit* u) final override {
		if (IsStructure(Observation())(*u)) {
			// 전진 게이트 등등
			if (Distance2D(u->pos, startLocation_) < 50 && flags.status("search_branch") != 1) {
				flags.set("search_branch", 1);
				flags.set("search_result", 3);
			}
			bool duplicated = false;
			for (auto& l : enemy_units_scouter_seen) {
				if (duplicated |= (l->tag == u->tag)) {
					break;
				}
			}
			if (!duplicated) {
				enemy_units_scouter_seen.push_back(u);
			}
		}
		if (IsTownHall()(*u)) {
			bool duplicated = false;
			for (auto& l : enemy_townhalls_scouter_seen) {
				if (duplicated |= (l->tag == u->tag)) {
					break;
				}
			}
			if (!duplicated) {
				enemy_townhalls_scouter_seen.push_back(u);
			}
		}
	}

	void SmartAttackMove(const Unit * attacker, const sc2::Point2D & targetPosition)
	{
		if (attacker->orders.empty() || attacker->orders.back().ability_id != sc2::ABILITY_ID::ATTACK || Distance2D(attacker->orders.front().target_pos, targetPosition) > 1.0f)
		{
			Actions()->UnitCommand(attacker, sc2::ABILITY_ID::ATTACK_ATTACK, targetPosition);
		}
	}

	void SmartAttackUnit(const Unit * attacker, const Unit * target)
	{
		if (attacker == nullptr) return;
		if (!attacker->orders.empty() && attacker->orders.front().target_unit_tag == target->tag)
		{
			return;
		}
		Actions()->UnitCommand(attacker, ABILITY_ID::ATTACK, target);
	}

	void SmartMove(const Unit * attacker, Unit * target)
	{
		if (attacker == nullptr) return;
		if (!attacker->orders.empty() && attacker->orders.front().ability_id == sc2::ABILITY_ID::MOVE && attacker->orders.front().target_unit_tag == target->tag)
		{
			return;
		}
		Actions()->UnitCommand(attacker, sc2::ABILITY_ID::MOVE, target);
	}

	void SmartMove(const Unit * attacker, const Point3D & targetPosition)
	{
		SmartMove(attacker, Point2D(targetPosition));
	}
	void SmartMove(const Unit * attacker, const Point2D & targetPosition)
	{
		if (attacker == nullptr) return;
		if (!attacker->orders.empty() && attacker->orders.front().ability_id == sc2::ABILITY_ID::MOVE && Distance2D(attacker->orders.front().target_pos, targetPosition) < 0.01f || Distance2D(attacker->pos, targetPosition) < 0.01f)
		{
			return;
		}
		if (attacker->is_flying)
		{
			sc2::Point2D targetPositionNew = targetPosition;
			float x_min = static_cast<float>(Observation()->GetGameInfo().playable_min.x);
			float x_max = static_cast<float>(Observation()->GetGameInfo().playable_max.x);
			float y_min = static_cast<float>(Observation()->GetGameInfo().playable_min.y);
			float y_max = static_cast<float>(Observation()->GetGameInfo().playable_max.y);

			if (targetPosition.x < x_min)
			{
				if (targetPosition.y > attacker->pos.y)
				{
					targetPositionNew = sc2::Point2D(x_min, y_max);
				}
				else
				{
					targetPositionNew = sc2::Point2D(x_min, y_min);
				}
			}
			else if (targetPosition.x > x_max)
			{
				if (targetPosition.y > attacker->pos.y)
				{
					targetPositionNew = sc2::Point2D(x_max, y_max);
				}
				else
				{
					targetPositionNew = sc2::Point2D(x_max, y_min);
				}
			}
			else if (targetPosition.y < y_min)
			{
				if (targetPosition.x > attacker->pos.x)
				{
					targetPositionNew = sc2::Point2D(x_max, y_min);
				}
				else
				{
					targetPositionNew = sc2::Point2D(x_min, y_min);
				}
			}
			else if (targetPosition.y > y_max)
			{
				if (targetPosition.x > attacker->pos.x)
				{
					targetPositionNew = sc2::Point2D(x_max, y_max);
				}
				else
				{
					targetPositionNew = sc2::Point2D(x_min, y_max);
				}
			}
			Actions()->UnitCommand(attacker, sc2::ABILITY_ID::MOVE, targetPositionNew);
		}
		else
		{
			Actions()->UnitCommand(attacker, sc2::ABILITY_ID::MOVE, targetPosition);
		}
	}


	GameInfo game_info_;
	std::vector<Point3D> expansions_;
	Point3D startLocation_;
	Point3D staging_location_;

	Point3D front_expansion;
	Point3D Enemy_front_expansion;
	Point2D EnemyBaseLocation;

	Point2D RushLocation;
	Point2D EnemyLocation;
	Point2D ReadyLocation1;
	Point2D ReadyLocation2;
	Point2D KitingLocation;
	Units enemyUnitsInRegion;
	Units Attackers;
private:
	void ChatVersion() {
		Actions()->SendChat(botname + " " + version);
	}

	void Print(std::string Message) {
#ifdef DEBUG
		std::cout << Message << std::endl;
#endif
	}

	void Chat(std::string Message) // 6.29 채팅 함수
	{
#ifdef DEBUG
		Actions()->SendChat(Message);
#endif
	}

	void PrintCursor() {
		const ObservationInterface* observation = Observation();
		for (const auto & p : observation->GetUnits([](const Unit& u) {return u.is_selected;})) {
			std::cout << "Selected unit : " << UnitTypeToName(p->unit_type) << ", "
				<< "Position : (" << p->pos.x << ", " << p->pos.y << ", " << p->pos.z << "), "
				<< "Radius : " << p->radius << std::endl;
			const Unit* a = observation->GetUnit(p->engaged_target_tag);
			if (a != nullptr) {
				std::cout << "Engaging : " << UnitTypeToName(a->unit_type) << std::endl;
			}
			std::cout << "buffs : ";
			for (const auto & b : p->buffs) {
				std::cout << BuffIDToName(b) << " ";
			}
			std::cout << std::endl;
			std::cout << "orders : " << std::endl;
			for (const auto & o : p->orders) {
				std::cout << AbilityTypeToName(o.ability_id) << " " << o.progress << " ";
				const Unit* b = observation->GetUnit(o.target_unit_tag);
				if (b != nullptr) {
					std::cout << UnitTypeToName(b->unit_type);
				}
				if (Point2D(0, 0) != o.target_pos) {
					std::cout << "(" << o.target_pos.x << ", " << o.target_pos.y << ")";
				}
				std::cout << std::endl;
			}
		}
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
				const float radius = ed.radius + 1.0f;
				/*
				if (EffectID(effect.effect_id).ToType() == EFFECT_ID::LIBERATORMORPHED)
				{
					const float radius = ed.radius + 1.0f;
				}
				*/
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

						if (EffectID(effect.effect_id).ToType() == EFFECT_ID::LIBERATORMORPHED || EffectID(effect.effect_id).ToType() == EFFECT_ID::LIBERATORMORPHING)
						{
							Units bases = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));

							for (const auto & base : bases)
							{
								if (Distance2D(unit->pos, pos) < 10)
								{
									return false;
								}
							}
						}
						else
						{
							SmartMove(unit, fleeingPos);
						}
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

	bool DefendDuty(const Unit * unit);

	void Defend();

	void ManageWarpBlink(const Unit * unit);

	bool IsUnitInUnits(const Unit * unit, Units & units);

	void OrganizeSquad();

	void DoGuerrillaWarp(const Unit * unit);

	void ManageRush();

	void Roam_randombase(const Unit * unit);

	void AdeptPhaseShift(const Unit * unit, Units ShadeNearEnemies, Units NearbyEnemies, bool & ComeOn);

	void AdeptPhaseToLocation(const Unit * unit, Point2D Location, bool & Timer, bool & ComeOn);

	void ManageBlink(const Unit * unit, const Unit * enemyarmy);

	void StalkerBlinkEscape(const Unit * unit, const Unit * enemyarmy);

	void StalkerBlinkForward(const Unit * unit, const Unit * enemyarmy);

	void FleeKiting(const Unit * unit, const Unit * enemyarmy);

	void DistanceKiting(const Unit * unit, const Unit * enemyarmy, const Unit * army);

	void FrontKiting(const Unit * unit, const Unit * enemyarmy);

	bool CanHitMe(const Unit * unit);

	void ComeOnKiting(const Unit * unit, const Unit * enemyarmy);

	void Kiting(const Unit * unit, const Unit * enemyarmy);

	void SmartMoveEfficient(const Unit * unit, Point2D KitingLocation, const Unit * enemyarmy);


	void EmergencyKiting(const Unit * unit, const Unit * enemyarmy);

	bool GetPosition(Units & units, Point2D & position);

	void KiteEnemy(const Unit * unit, Units enemy_army, Units enemy_units, Point2D KitingLocation, bool enemiesnear, const ObservationInterface * observation);


	float MinimumDistance2D(const Unit * unit, const Unit * enemyarmy);


	Point2D CalcKitingPosition(Point2D Mypos, Point2D EnemyPos);

	bool GetPosition(Units Enemyunits, Unit::Alliance alliace, Point2D & position);

	bool GetPosition(UNIT_TYPEID unit_type, Unit::Alliance alliace, Point2D & position);

	int getAttackPriority(const Unit * u);

	bool LoadUnit(const Unit * unit, const Unit * passenger);

	bool LoadUnitWeaponCooldown(const Unit * unit, const Unit * passenger);

	const Unit * GetPassenger(const Unit * shuttle, Units & targets);

	const Unit * GetNearShuttle(const Unit * unit);

	void ManageWarpBlink(const Unit * unit, const Unit * shuttle);

	const Unit * GetTarget(const Unit * rangedUnit, Units & targets);

	const Unit * GetNearTarget(const Unit * rangedUnit, Units & targets);

	const float getunitsDpsGROUND(Units targets) const;

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
			SmartAttackUnit(unit, enemy_units.front());
			return;
		}

		//If the unit is doing something besides attacking, make it attack. // 공격을 안하면 공격명령
		if (unit->orders.front().ability_id != ABILITY_ID::ATTACK) {
			SmartAttackUnit(unit, enemy_units.front());
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
		/*if (!unit->orders.empty()) {
			return;
		}*/
		Point2D target_pos;

		if (FindEnemyPosition(target_pos)) { //적 기지를 알고있는 상황이면
			if (Distance2D(unit->pos, target_pos) < 20 && enemy_units.empty()) { //적 유닛이 없는 상황에서 적 기지가 근처에 있으면
				if (TryFindRandomPathableLocation(unit, target_pos)) { //유닛별로 맵 전체적으로 퍼지는 위치를 배정받고
					SmartAttackMove(unit, target_pos); //그 위치로 간다
					return;
				}
			}
			else if (!enemy_units.empty()) // 적 유닛이 있는 상황이면 또는 이 유닛이 적 기지 근처에 없는상황이면
			{
				SmartAttackUnit(unit, enemy_units.front());
				return;
			}
			// TODO : 가장 마지막으로 본 적의 위치를 target_pos 로 리턴하는 함수를 만들자

			SmartAttackMove(unit, target_pos); //위 작업이 끝나면 적 기지를 다시한번 간다
		}
		else { //적 기지도 모르면 막 돌아다녀라
			if (TryFindRandomPathableLocation(unit, target_pos)) {
				SmartAttackMove(unit, target_pos);
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

	bool RetreatSmart(const Unit* unit, Point2D retreat_position) {
		bool moving = false;

		float dist = Distance2D(unit->pos, retreat_position);

		if (dist >= 2) // 멀리있으면
		{
			if (!unit->orders.empty()) //뭔가를 하는게
			{
				if (unit->orders.front().ability_id != ABILITY_ID::UNLOADALLAT_WARPPRISM) //내리는게 아니라면
				{
					SmartMove(unit, retreat_position); // 움직여라
					moving = true;
				}
			}
			else //너가 아무것도 안하고 있었다면
			{
				SmartMove(unit, retreat_position); // 움직여라
			}

		}
		return moving;
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
			SmartMove(unit, retreat_position);
		}
		else if (!unit->orders.empty() && dist > 14) {
			if (unit->orders.front().ability_id != ABILITY_ID::MOVE) {
				SmartMove(unit, retreat_position);
			}
		}
	}


	bool FindEnemyPosition(Point2D& target_pos) {
		if (game_info_.enemy_start_locations.empty()) {
			return false;
		}
		target_pos = game_info_.enemy_start_locations.front();
		EnemyBaseLocation = game_info_.enemy_start_locations.front();
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

		return distance > 0.01f;
	}

	bool IsUnitAction(const Unit* unit, AbilityID ability_id) {
		if (unit->orders.empty()) return false;
		if (unit->orders.front().ability_id != ability_id) return false;
		return true;
	}

	bool IsUnitAction(const Unit* unit, AbilityID ability_id, Point2D& target_pos) {
		if (unit->orders.empty()) return false;
		if (unit->orders.front().ability_id != ability_id) return false;
		if (target_pos != unit->orders.front().target_pos) return false;
		return true;
	}

	bool IsUnitAction(const Unit* unit, AbilityID ability_id, Unit* target_unit) {
		if (unit->orders.empty()) return false;
		if (unit->orders.front().ability_id == ability_id) return false;
		if (Observation()->GetUnit(unit->orders.front().target_unit_tag) != target_unit) return false;
		return true;
	}

	size_t CountUnitType(UnitTypeID unit_type) {
		return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
	}

	size_t CountUnitType(const ObservationInterface* observation, UnitTypeID unit_type) {
		return observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
	}

	size_t CountUnitTypeNearLocation(UnitTypeID unit_type, Point2D location, float distance = 15) {
		Units units = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
		return FindUnitsNear(location, distance, units).size();
	}

	Units FindUnitsNear(const Point2D& start, float distance, Filter f = {}) const {
		if (distance < 0) return Units();
		float distance_squared = (distance == std::numeric_limits<float>::max()) ? distance : distance * distance;
		Filter f2 = [start, distance_squared, f](const Unit& u) {
			return ( !f || f(u) ) && DistanceSquared2D(u.pos, start) <= distance_squared;
		};
		return Observation()->GetUnits(f2);
	}

	Units FindUnitsNear(const Point2D& start, float distance, Unit::Alliance a, Filter f = {}) const {
		if (distance < 0) return Units();
		float distance_squared = (distance == std::numeric_limits<float>::max()) ? distance : distance * distance;
		Filter f2 = [start, distance_squared, f](const Unit& u) {
			return ( !f || f(u) ) && DistanceSquared2D(u.pos, start) <= distance_squared;
		};
		return Observation()->GetUnits(a, f2);
	}

	Units FindUnitsNear(const Point2D& start, float distance, const Units& units) const {
		Units target;
		if (distance < 0) return target;
		float distance_squared = (distance == std::numeric_limits<float>::max()) ? distance : distance * distance;
		for (const auto& u : units) {
			if (DistanceSquared2D(u->pos, start) <= distance_squared)
				target.push_back(u);
		}
		return target;
	}

	Units FindUnitsNear(const Unit* from_me, float distance, Filter f = {}) const {
		if (from_me == nullptr) return Units();
		return FindUnitsNear(from_me->pos, distance, f);
	}

	Units FindUnitsNear(const Unit* from_me, float distance, Unit::Alliance a, Filter f = {}) const {
		if (from_me == nullptr) return Units();
		return FindUnitsNear(from_me->pos, distance, a, f);
	}

	Units FindUnitsNear(const Unit* from_me, float distance, const Units& units) const {
		if (from_me == nullptr) return Units();
		return FindUnitsNear(from_me->pos, distance, units);
	}

	Units FindUnitsNear(const Units& from_us, float distance, Filter f = {}) const {
		if (distance < 0) return Units();
		float distance_squared = (distance == std::numeric_limits<float>::max()) ? distance : distance * distance;
		Filter f2 = [from_us, distance_squared, f](const Unit& u) {
			if (f && !f(u)) return false;
			for (const auto& me : from_us) {
				if (DistanceSquared2D(u.pos, me->pos) <= distance_squared) {
					return true;
				}
			}
			return false;
		};
		return Observation()->GetUnits(f2);
	}

	Units FindUnitsNear(const Units& from_us, float distance, Unit::Alliance a, Filter f = {}) const {
		if (distance < 0) return Units();
		float distance_squared = (distance == std::numeric_limits<float>::max()) ? distance : distance * distance;
		Filter f2 = [from_us, distance_squared, f](const Unit& u) {
			if (f && !f(u)) return false;
			for (const auto& me : from_us) {
				if (DistanceSquared2D(u.pos, me->pos) <= distance_squared) {
					return true;
				}
			}
			return false;
		};
		return Observation()->GetUnits(a, f2);
	}

	Units FindUnitsNear(const Units& from_us, float distance, const Units& units) const {
		Units target;
		if (distance < 0) return target;
		float distance_squared = (distance == std::numeric_limits<float>::max()) ? distance : distance * distance;
		for (const auto& u : units) {
			for (const auto& me : from_us) {
				if (DistanceSquared2D(u->pos, me->pos) <= distance_squared) {
					target.push_back(u);
					break;
				}
			}
		}
		return target;
	}

	bool GetRandomUnit(const Unit*& unit_out, const ObservationInterface* observation, Units& my_units) {
		int num = static_cast<int>(my_units.size());
		if (num == 0) return false;
		unit_out = my_units[GetRandomInteger(0, num - 1)];
		return true;
	}

	bool GetRandomUnit(const Unit*& unit_out, const ObservationInterface* observation, UnitTypeID unit_type) {
		Units my_units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
		int num = static_cast<int>(my_units.size());
		if (num == 0) return false;
		unit_out = my_units[GetRandomInteger(0, num - 1)];
		return true;
	}

	const Unit* FindNearestMineralPatch(const Point2D& start, float max_d = std::numeric_limits<float>::max()) const {
		return FindNearestUnit(start, Unit::Alliance::Neutral, IsMineral(), max_d);
	}

	const Unit* FindNearestUnit(const Point2D& start, Filter f = {}, float max_d = std::numeric_limits<float>::max()) const {
		const Units units = FindUnitsNear(start, max_d, f);
		return FindNearestUnit(start, units, max_d);
	}

	const Unit* FindNearestUnit(const Point2D& start, Unit::Alliance a, Filter f = {}, float max_d = std::numeric_limits<float>::max()) const {
		const Units units = FindUnitsNear(start, max_d, a, f);
		return FindNearestUnit(start, units, max_d);
	}

	const Unit* FindNearestUnit(const Point2D& start, const Units& units, float max_d = std::numeric_limits<float>::max()) const {
		float distance_squared = (max_d == std::numeric_limits<float>::max()) ? max_d : max_d * max_d;
		const Unit* target = nullptr;
		for (const auto& u : units) {
			float d = DistanceSquared2D(u->pos, start);
			if (d < distance_squared) {
				distance_squared = d;
				target = u;
			}
		}
		//If we never found one return nullptr
		return target;
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
		if (unit == nullptr) return false;
		// is structure?
		if (!IsStructure(observation)(*unit)) return false;
		// is completely built?
		if (unit->build_progress != 1.0f) return false;
		// is doing something?
		if (unit->orders.empty()) return false;
		// is powered?
		if (IsUnpowered()(*unit)) return false;
		// is not buffed?
		//if (HasBuff(BUFF_ID::TIMEWARPPRODUCTION)(*unit)) return false;
		if (!unit->buffs.empty()) return false;

		// then chronoboost
		return Chronoboost(unit);
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
				Actions()->UnitCommand(nexus, ABILITY_ID(3755), unit);
				return true;
			}
		}
		return false;
	}

	bool TryBuildUnit(AbilityID ability_type_for_unit, UnitTypeID building_type, UnitTypeID unit_type, Filter priority_filter = {}) {
		const ObservationInterface* observation = Observation();

		//If we are at supply cap, don't build anymore units, unless its an overlord.
		if (observation->GetFoodUsed()+observation->GetUnitTypeData().at(unit_type).food_required > observation->GetFoodCap()) {
			return false;
		}

		if (observation->GetMinerals() < observation->GetUnitTypeData().at(unit_type).mineral_cost || observation->GetVespene() < observation->GetUnitTypeData().at(unit_type).vespene_cost) {
            return false;
		}

		const Unit* unit = nullptr;
		Units target_units = observation->GetUnits(Unit::Alliance::Self, IsUnit(building_type));
		for (const auto& candidate_unit : target_units) {
			// is not completely built?
			if (candidate_unit->build_progress != 1.0f) continue;
			// is doing something?
			if (!candidate_unit->orders.empty()) continue;
			// is unpowered?
			if (IsUnpowered()(*candidate_unit)) continue;
			unit = candidate_unit;
			// pick prioritized structures first
			/*if (!HasBuff(BUFF_ID::TIMEWARPPRODUCTION)(*unit)) {
				break;
			}*/
			if (!unit->buffs.empty()) break;
		}
		if (unit == nullptr) {
			return false;
		}
		Actions()->UnitCommand(unit, ability_type_for_unit);
		return true;
	}

	bool TryBuildUnitChrono(AbilityID ability_type_for_unit, UnitTypeID building_type, UnitTypeID unit_type) {
		const ObservationInterface* observation = Observation();

		//If we are at supply cap, don't build anymore units, unless its an overlord.
		if (observation->GetFoodUsed()+observation->GetUnitTypeData().at(unit_type).food_required > observation->GetFoodCap()) {
			return false;
		}
		if (observation->GetMinerals() < observation->GetUnitTypeData().at(unit_type).mineral_cost || observation->GetVespene() < observation->GetUnitTypeData().at(unit_type).vespene_cost) {
            return false;
		}


		const Unit* unit = nullptr;
		Units target_units = observation->GetUnits(Unit::Alliance::Self, IsUnit(building_type));
		for (const auto& candidate_unit : target_units) {
			// is not completely built?
			if (candidate_unit->build_progress != 1.0f) continue;
			// is doing something?
			if (!candidate_unit->orders.empty()) continue;
			// is unpowered?
			if (IsUnpowered()(*candidate_unit)) continue;
			unit = candidate_unit;
			// pick prioritized structures first
			/*if (!HasBuff(BUFF_ID::TIMEWARPPRODUCTION)(*unit)) {
				break;
			}*/
			if (!unit->buffs.empty()) break;
		}
		if (unit == nullptr) {
			return false;
		}
		Actions()->UnitCommand(unit, ability_type_for_unit);
		TryChronoboost(unit);
		return true;
	}

	bool TryBuildUpgradeChrono(AbilityID ability_type_for_unit, UnitTypeID building_type, UpgradeID upgrade_type) {
		const ObservationInterface* observation = Observation();

		if (observation->GetMinerals() < observation->GetUpgradeData().at(upgrade_type).mineral_cost || observation->GetVespene() < observation->GetUpgradeData().at(upgrade_type).vespene_cost) {
            return false;
		}

		const Unit* unit = nullptr;
		Units target_units = observation->GetUnits(Unit::Alliance::Self, IsUnit(building_type));
		for (const auto& candidate_unit : target_units) {
			// is not completely built?
			if (candidate_unit->build_progress != 1.0f) continue;
			// is doing something?
			if (!candidate_unit->orders.empty()){
                if (observation->GetAbilityData().at(ability_type_for_unit).ability_id == candidate_unit->orders.front().ability_id) {
                    return TryChronoboost(candidate_unit);
                }
                continue;
			}
			unit = candidate_unit;
		}


		if (unit == nullptr) {
			return false;
		}
		Actions()->UnitCommand(unit, ability_type_for_unit);
		TryChronoboost(unit);
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

	bool TryBuildUpgrade(AbilityID ability_type_for_unit, UnitTypeID building_type, UpgradeID upgrade_type) {
        const ObservationInterface* observation = Observation();

        if (observation->GetMinerals()<observation->GetUpgradeData().at(upgrade_type).mineral_cost || observation->GetVespene()<observation->GetUpgradeData().at(upgrade_type).vespene_cost) {
            return false;
		}

		const Unit* unit = nullptr;
		Units target_units = observation->GetUnits(Unit::Alliance::Self, IsUnit(building_type));

		for (const auto& candidate_unit : target_units) {
			// is not completely built?
			if (candidate_unit->build_progress != 1.0f) continue;
			// is doing something?
			if (!candidate_unit->orders.empty()) continue;
			// is unpowered?
			if (IsUnpowered()(*candidate_unit)) continue;

			unit = candidate_unit;
		}
		if (unit == nullptr) {
			return false;
		}
		Actions()->UnitCommand(unit, ability_type_for_unit);
		return true;
	}

	bool TryWarpUnitPrism(AbilityID ability_type_for_unit) {
        const ObservationInterface* observation = Observation();
        std::vector<PowerSource> power_sources = observation->GetPowerSources();
        Units warpgates = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_WARPGATE));
        if (observation->GetFoodUsed() >= observation->GetFoodCap()) {
			return false;
		}
		if (power_sources.empty()) {
            return false;
        }

        float radius = 0.0f;
        Point2D build_location;
        for (const auto& power__source : power_sources) {
            if (observation->GetUnit(power__source.tag) != nullptr) {
                if (observation->GetUnit(power__source.tag)->unit_type == UNIT_TYPEID::PROTOSS_WARPPRISMPHASING) {
                    radius = power__source.radius;
                    build_location = power__source.position;
                    break;
                }
            }
        }

        if (radius < 0.1f) {
            return false;
        }


        float rx = GetRandomScalar();
        float ry = GetRandomScalar();
        build_location = Point2D(build_location.x + rx * radius, build_location.y + ry * radius);
		if (!observation->IsPathable(build_location)) return false;

        for (const auto& warpgate : warpgates) {
            if (warpgate->build_progress == 1) {
                AvailableAbilities abilities = Query()->GetAbilitiesForUnit(warpgate);
                for (const auto& ability : abilities.abilities) {
                    if (ability.ability_id == ability_type_for_unit) {
                        Actions()->UnitCommand(warpgate, ability_type_for_unit, build_location);

                        return true;
                    }
                }
            }
        }
        return false;


	}

	// if isExpansion is false, then consider expansion sites and avoid these places.
	bool TryBuildStructure(AbilityID ability_type_for_structure, UnitTypeID building_type, UnitTypeID unit_type, Point2D location, bool isExpansion = false) {
		const ObservationInterface* observation = Observation();
		Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));

		if (observation->GetMinerals() < observation->GetUnitTypeData().at(building_type).mineral_cost || observation->GetVespene() < observation->GetUnitTypeData().at(building_type).vespene_cost) {
            return false;
		}

		//if we have no workers Don't build
		if (workers.empty()) {
			return false;
		}

		bool expansion_building =
			ability_type_for_structure == ABILITY_ID::BUILD_NEXUS ||
			ability_type_for_structure == ABILITY_ID::BUILD_COMMANDCENTER ||
			ability_type_for_structure == ABILITY_ID::BUILD_HATCHERY;

		bool Is2x2 =
			ability_type_for_structure == ABILITY_ID::BUILD_PYLON ||
			ability_type_for_structure == ABILITY_ID::BUILD_SHIELDBATTERY ||
			ability_type_for_structure == ABILITY_ID::BUILD_DARKSHRINE ||
			ability_type_for_structure == ABILITY_ID::BUILD_MISSILETURRET ||
			ability_type_for_structure == ABILITY_ID::BUILD_SPIRE ||
			ability_type_for_structure == ABILITY_ID::BUILD_SPINECRAWLER ||
			ability_type_for_structure == ABILITY_ID::BUILD_SPORECRAWLER;

		// keep expansion site clean. todo: wrong expansions than expectation.
		if (!isExpansion && !expansion_building) {
			bool nearExpansion = false;
			for (const auto& expansion : expansions_) {
				float absx = building_abs(location.x, expansion.x, Is2x2);
				float absy = building_abs(location.y, expansion.y, Is2x2);
				if (absx < 3.49f && absy < 3.49f) {
					return false;
				}
				if (absx < 4.01f && absy < 4.01f) {
					nearExpansion = true;
					break;
				}
			}
			// prevent building next to minerals / geysers
			if (nearExpansion) {
				Filter f = [](const Unit& u) {
					return IsMineral()(u) || IsVespeneGeyser()(u)
						|| (IsUnits({UNIT_TYPEID::PROTOSS_ASSIMILATOR, UNIT_TYPEID::TERRAN_REFINERY, UNIT_TYPEID::ZERG_EXTRACTOR})(u)
							&& u.alliance == Unit::Alliance::Self);
				};
				Units ms = observation->GetUnits(f);
				for (const auto& m : ms) {
					bool is_mineral = IsMineral()(*m);
					float absx = building_abs(location.x, m->pos.x, Is2x2, is_mineral);
					float absy = building_abs(location.y, m->pos.y, Is2x2);
					if ((is_mineral && absx < 2.51f && absy < 2.01f) ||
						(!is_mineral && absx < 3.01f && absy < 3.01f)) {
						return false;
					}
				}
			}
		}
		// even for the expansions
		if (!isExpansion && expansion_building) {
			for (const auto& expansion : expansions_) {
				// build on one of the expansion sites
				if (building_abs(location.x, expansion.x) < 0.5f && building_abs(location.y, expansion.y) < 0.5f) {
					break;
				}
				// do not build near expansion sites
				if (building_abs(location.x, expansion.x) < 4.99f && building_abs(location.y, expansion.y) < 4.99f) {
					return false;
				}
			}
		}

		// Check to see if there is already a worker heading out to build it
		for (const auto& worker : workers) {
			for (const auto& order : worker->orders) {
				if (order.ability_id == ability_type_for_structure) {
					return false;
				}
			}
		}

		// Check to see if unit can build there
		if (!Query()->Placement(ability_type_for_structure, location)) {
			return false;
		}

		// If no worker is already building one, get a nearest worker to build one
		Tag probe_forward_tag = (probe_forward == nullptr) ? NullTag : probe_forward->tag;
		Tag builder_tag = NullTag;
		std::vector<QueryInterface::PathingQuery> query_vector;
		for (const auto& worker : workers) {
			if (probe_scout != nullptr && worker->tag == probe_scout->tag && !probe_scout->orders.empty()) continue;
			// consider idle or mining workers only
			if (worker->tag != probe_forward_tag && !worker->orders.empty()) {
				auto ability_id = worker->orders.front().ability_id;
				if (ability_id != ABILITY_ID::HARVEST_GATHER &&
					ability_id != ABILITY_ID::HARVEST_RETURN) {
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

		// find worker that is nearest.
		float min_distance = std::numeric_limits<float>::max();
		for (size_t i = 0; i < size; i++) {
			float d = distances.at(i);

			// Check to see if unit can make it there
			if (d < 0.01f) {
				continue;
			}

			Tag current_tag = query_vector.at(i).start_unit_tag_;

			// prioritize probe_forward
			if (current_tag == probe_forward_tag) {
				d = (work_probe_forward) ? (d - 4.0f) : (d * 0.5f - 6.0f);
			}

			if (d < min_distance) {
				min_distance = d;
				builder_tag = current_tag;
			}
		}
		if (builder_tag == NullTag) return false;
		const Unit* builder = observation->GetUnit(builder_tag);
		if (builder == nullptr) {
			return false;
		}

		Actions()->UnitCommand(builder, ability_type_for_structure, location);
		return true;
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
		else if (IsUnits({UNIT_TYPEID::ZERG_DRONE, UNIT_TYPEID::ZERG_DRONEBURROWED})(*builder)) {
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

	bool TryBuildStructureNearPylon(AbilityID ability_type_for_structure, UnitTypeID building_type, const Unit* pylon) {
		const ObservationInterface* observation = Observation();
		if (pylon == nullptr) return false;
		if (!pylon->is_alive) return false;

		if (observation->GetMinerals() < observation->GetUnitTypeData().at(building_type).mineral_cost || observation->GetVespene() < observation->GetUnitTypeData().at(building_type).vespene_cost) {
            return false;
		}

		const std::vector<PowerSource>& power_sources = observation->GetPowerSources();

		float radius = 0;
		for (const auto& p : power_sources) {
			if (p.tag == pylon->tag) {
				radius = p.radius;
				break;
			}
		}
		if (radius == 0) return false;

		float rx = GetRandomScalar();
		float ry = GetRandomScalar();
		Point2D build_location = Point2D(pylon->pos.x + rx * radius, pylon->pos.y + ry * radius);
		return TryBuildStructure(ability_type_for_structure, building_type, UNIT_TYPEID::PROTOSS_PROBE, build_location);
	}

	bool TryBuildStructureNearPylon(AbilityID ability_type_for_structure, UnitTypeID building_type) {
		const ObservationInterface* observation = Observation();

		if (observation->GetMinerals() < observation->GetUnitTypeData().at(building_type).mineral_cost || observation->GetVespene() < observation->GetUnitTypeData().at(building_type).vespene_cost) {
            return false;
		}

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
		return TryBuildStructure(ability_type_for_structure, building_type, UNIT_TYPEID::PROTOSS_PROBE, build_location);
	}

	/*bool TryBuildStructureNearPylon(AbilityID ability_type_for_structure, UnitTypeID, const Unit* pylon) {
		return TryBuildStructureNearPylon(ability_type_for_structure, pylon);
	}

	bool TryBuildStructureNearPylonWithUnit(const Unit* unit, AbilityID ability_type_for_structure, const Unit* pylon) {
		return TryBuildStructureNearPylon(ability_type_for_structure, pylon);
	}*/

	bool TryBuildForge(const Unit* unit, const Unit* pylon) {
		return TryBuildStructureNearPylon(ABILITY_ID::BUILD_FORGE, UNIT_TYPEID::PROTOSS_FORGE,pylon);
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
		return TryBuildStructure(ABILITY_ID::BUILD_PYLON, UNIT_TYPEID::PROTOSS_PYLON, UNIT_TYPEID::PROTOSS_PROBE, build_location);
	}

	bool TrybuildFirstPylon() {
        const ObservationInterface* observation = Observation();

        Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PYLON));
		if (observation->GetFoodUsed()<14) {
            return false;
        }

        if (observation->GetMinerals() < 100) {
			return false;
		}

		if (units.size()>0) {
            return false;
		}

		float x = 7.0f;
		float y = 7.0f;
		if ((float)game_info_.width/2 < startLocation_.x) {
            x = -7.0f;
		}
		if ((float)game_info_.height/2 < startLocation_.y) {
            y = -7.0f;
		}
		Point2D build_location = Point2D(startLocation_.x + x, startLocation_.y + y);
        return TryBuildStructure(ABILITY_ID::BUILD_PYLON, UNIT_TYPEID::PROTOSS_PYLON, UNIT_TYPEID::PROTOSS_PROBE,build_location);
	}

	void MakeBaseResourceMap() {
		const ObservationInterface* observation = Observation();

		if (last_map_renewal == observation->GetGameLoop() + 1) {
			return;
		}

		last_map_renewal = observation->GetGameLoop() + 1;

		resources_to_nearest_base.clear();
		resources_to_nearest_base.emplace(NullTag, NullTag);

		Filter filter_geyser = [](const Unit& u) {
			return u.build_progress == 1.0f &&
				IsUnits({ UNIT_TYPEID::PROTOSS_ASSIMILATOR,
					UNIT_TYPEID::TERRAN_REFINERY,
					UNIT_TYPEID::ZERG_EXTRACTOR })(u);
		};

		Filter filter_bases = [](const Unit& u) {
			return u.build_progress == 1.0f &&
				IsTownHall()(u);
		};

		Units geysers = observation->GetUnits(Unit::Alliance::Self, filter_geyser);
		Units bases = observation->GetUnits(Unit::Alliance::Self, filter_bases);
		Units minerals = observation->GetUnits(Unit::Alliance::Neutral, IsMineral());

		for (const auto& m : minerals) {
			const Unit* b = FindNearestUnit(m->pos, bases, 11.5);
			Tag b_tag = (b != nullptr) ? b->tag : NullTag;
			resources_to_nearest_base.emplace(m->tag, b_tag);
		}

		for (const auto& g : geysers) {
			const Unit* b = FindNearestUnit(g->pos, bases, 11.5);
			Tag b_tag = (b != nullptr) ? b->tag : NullTag;
			resources_to_nearest_base.emplace(g->tag, b_tag);
		}
	}

	void MineIdleWorkers(const Unit* worker, bool reassigning = false) {
		const ObservationInterface* observation = Observation();

		Filter filter_geyser = [](const Unit& u) {
			return u.build_progress == 1.0f &&
				IsUnits({ UNIT_TYPEID::PROTOSS_ASSIMILATOR,
					UNIT_TYPEID::TERRAN_REFINERY,
					UNIT_TYPEID::ZERG_EXTRACTOR })(u);
		};

		Filter filter_bases = [](const Unit& u) {
			return u.build_progress == 1.0f &&
				IsTownHall()(u) &&
				u.ideal_harvesters != 0;
		};

		Units geysers = observation->GetUnits(Unit::Alliance::Self, filter_geyser);
		Units bases = observation->GetUnits(Unit::Alliance::Self, filter_bases);
		Units minerals = observation->GetUnits(Unit::Alliance::Neutral, IsMineral());
		size_t geysers_size = geysers.size();
		size_t bases_size = bases.size();
		size_t minerals_size = minerals.size();

		if (bases.empty()) {
			return;
		}

		MakeBaseResourceMap();

		bool has_space_for_half_mineral = true;
		bool has_space_for_gas = false;
		bool has_space_for_mineral = false;
		const Unit* gas_base = nullptr;
		const Unit* mineral_base = nullptr;
		const Unit* valid_mineral_patch = nullptr;

		// If there are very few workers gathering minerals.
		for (const auto& base : bases) {
			if (base->assigned_harvesters >= base->ideal_harvesters / 2 && base->assigned_harvesters >= 4) {
				has_space_for_half_mineral = false;
				break;
			}
		}
		// Search for a base that is missing workers.
		for (const auto& base : bases) {
			if (base->assigned_harvesters < base->ideal_harvesters) {
				has_space_for_mineral = true;
				mineral_base = base;
				break;
			}
		}
		// Search for a base that is missing workers.
		for (const auto& geyser : geysers) {
			Tag base_tag = resources_to_nearest_base.count(geyser->tag) ? resources_to_nearest_base.at(geyser->tag) : NullTag;
			if (base_tag == NullTag) continue;
			if (geyser->assigned_harvesters < geyser->ideal_harvesters) {
				has_space_for_gas = true;
				gas_base = observation->GetUnit(base_tag);
				break;
			}
		}

		float min_distance = std::numeric_limits<float>::max();
		const Unit* target_resource = nullptr;

		// Search for a base that is missing mineral workers.
		if (has_space_for_half_mineral && !EnemyRush) {
			for (const auto& mineral : minerals) {
				Tag b = resources_to_nearest_base.count(mineral->tag) ? resources_to_nearest_base.at(mineral->tag) : NullTag;
				if (b == NullTag) continue;
				const Unit* base = observation->GetUnit(b);
				if (base == nullptr) continue;
				if (base->assigned_harvesters >= base->ideal_harvesters / 2) continue;
				float current_distance = DistanceSquared2D(mineral->pos, worker->pos);
				if (current_distance < min_distance) {
					min_distance = current_distance;
					target_resource = mineral;
				}
			}
		}

		if (target_resource != nullptr) {
			Actions()->UnitCommand(worker, ABILITY_ID::HARVEST_GATHER, target_resource);
			return;
		}

		// Search for a base that does not have full of gas workers.
		if (has_space_for_gas && !EnemyRush) {
			for (const auto& geyser : geysers) {
				if (geyser->assigned_harvesters >= geyser->ideal_harvesters) continue;
				Tag b = resources_to_nearest_base.count(geyser->tag) ? resources_to_nearest_base.at(geyser->tag) : NullTag;
				if (b == NullTag) continue;
				const Unit* base = observation->GetUnit(b);
				if (base == nullptr) continue;
				float current_distance = DistanceSquared2D(geyser->pos, worker->pos);
				if (current_distance < min_distance) {
					min_distance = current_distance;
					target_resource = geyser;
				}
			}
		}

		if (target_resource != nullptr) {
			Actions()->UnitCommand(worker, ABILITY_ID::HARVEST_GATHER, target_resource);
			return;
		}

		// Search for a base that does not have full of mineral workers.
		if (has_space_for_mineral && !EnemyRush) {
			for (const auto& mineral : minerals) {
				Tag b = resources_to_nearest_base.count(mineral->tag) ? resources_to_nearest_base.at(mineral->tag) : NullTag;
				if (b == NullTag) continue;
				const Unit* base = observation->GetUnit(b);
				if (base == nullptr) continue;
				if (base->assigned_harvesters >= base->ideal_harvesters) continue;
				float current_distance = DistanceSquared2D(mineral->pos, worker->pos);
				if (current_distance < min_distance) {
					min_distance = current_distance;
					target_resource = mineral;
				}
			}
		}

		if (target_resource != nullptr) {
			Actions()->UnitCommand(worker, ABILITY_ID::HARVEST_GATHER, target_resource);
			return;
		}

		if (!worker->orders.empty()) {
			return;
		}

		//If all workers are spots are filled just go to any base.
		const Unit* random_base = GetRandomEntry(bases);
		valid_mineral_patch = FindNearestMineralPatch(random_base->pos);
		if (valid_mineral_patch == nullptr) return;
		if (reassigning) return;
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

		Filter filter_geyser = [](const Unit& u) {
			return u.build_progress == 1.0f &&
				IsUnits({ UNIT_TYPEID::PROTOSS_ASSIMILATOR,
					UNIT_TYPEID::TERRAN_REFINERY,
					UNIT_TYPEID::ZERG_EXTRACTOR })(u);
		};

		Filter filter_bases = [](const Unit& u) {
			return u.build_progress == 1.0f &&
				IsTownHall()(u) &&
				u.ideal_harvesters != 0;
		};


		Units geysers = observation->GetUnits(Unit::Alliance::Self, filter_geyser);
		Units bases = observation->GetUnits(Unit::Alliance::Self, filter_bases);
		Units minerals = observation->GetUnits(Unit::Alliance::Neutral, IsMineral());
		Units workers = observation->GetUnits(Unit::Alliance::Self, IsWorker());
		size_t geysers_size = geysers.size();
		size_t bases_size = bases.size();
		size_t minerals_size = minerals.size();

		if (bases.empty()) {
			return;
		}

		MakeBaseResourceMap();

		bool has_space_for_half_mineral = true;
		bool has_space_for_gas = false;
		bool has_space_for_mineral = false;
		const Unit* gas_base = nullptr;
		const Unit* mineral_base = nullptr;
		const Unit* valid_mineral_patch = nullptr;

		// If there are very few workers gathering minerals.
		for (const auto& base : bases) {
			if (base->assigned_harvesters >= base->ideal_harvesters / 2 && base->assigned_harvesters >= 4) {
				has_space_for_half_mineral = false;
				break;
			}
		}
		// Search for a base that is missing workers.
		for (const auto& base : bases) {
			if (base->assigned_harvesters < base->ideal_harvesters) {
				has_space_for_mineral = true;
				mineral_base = base;
				break;
			}
		}
		// Search for a base that is missing workers.
		for (const auto& geyser : geysers) {
			Tag base_tag = resources_to_nearest_base.count(geyser->tag) ? resources_to_nearest_base.at(geyser->tag) : NullTag;
			if (base_tag == NullTag) continue;
			if (geyser->assigned_harvesters < geyser->ideal_harvesters) {
				has_space_for_gas = true;
				gas_base = observation->GetUnit(base_tag);
				break;
			}
		}

		if (has_space_for_mineral || has_space_for_gas) {
			for (const auto& worker : workers) {
				if (worker == probe_scout) continue;
				if (worker == probe_forward && !work_probe_forward) continue;
				if (worker->orders.empty()) continue;
				const UnitOrder& o = worker->orders.front();
				if (o.ability_id != ABILITY_ID::HARVEST_GATHER) continue;

				Tag target_tag = o.target_unit_tag;
				Tag nearest_base_tag = resources_to_nearest_base.count(target_tag) ? resources_to_nearest_base.at(target_tag) : NullTag;

				// reassign workers that mines resources far from nexuses. (get all)
				if (nearest_base_tag == NullTag) {
					MineIdleWorkers(worker);
					Print("reassigning no nexus workers");
					return;
				}

				const Unit* nearest_base = observation->GetUnit(nearest_base_tag);
				const Unit* target_resource = observation->GetUnit(target_tag);
				if (target_resource == nullptr) continue;
				if (nearest_base == nullptr) continue;

				// reassign overflowing workers (geysers)
				if (!IsMineral()(*target_resource)) {
					if (target_resource->assigned_harvesters - target_resource->ideal_harvesters <= 0) continue;
					MineIdleWorkers(worker);
					return;
				}
				// if there is a space
				// reassign overflowing workers (minerals)
				else {
					if (nearest_base->assigned_harvesters - nearest_base->ideal_harvesters <= 0) continue;
					MineIdleWorkers(worker);
					return;
				}
			}
		}

		// if few workers are mining minerals, then mine mineral rather than gas
		if (has_space_for_half_mineral && !EnemyRush) {
			for (const auto& geyser : geysers) {
				if (geyser->assigned_harvesters == 0) continue;

				for (const auto& worker : workers) {
					if (worker == probe_scout) continue;
					if (worker == probe_forward && !work_probe_forward) continue;
					// pick gas mining workers
					if (worker->orders.empty()) continue;
					const UnitOrder& o = worker->orders.front();
					if (o.ability_id != ABILITY_ID::HARVEST_GATHER) continue;
					if (o.target_unit_tag != geyser->tag) continue;

					MineIdleWorkers(worker);
					Print("reassigning for mineral workers");
					return;
				}
			}
		}

		// mine gas.
		if (has_space_for_gas && !EnemyRush) {
			// sort by distance
			Point2D gas_base_pos = gas_base->pos;
			std::function<bool(const Unit*, const Unit*)> f =
				[gas_base_pos](const Unit* b1, const Unit* b2) {
				return DistanceSquared2D(gas_base_pos, b1->pos) < DistanceSquared2D(gas_base_pos, b2->pos);
			};

			std::sort(bases.begin(), bases.end(), f);

			for (const auto& base : bases) {
				if (base->assigned_harvesters - 1 < (base->ideal_harvesters / 2)) continue;

				const Unit* target_worker = nullptr;
				float min_distance = std::numeric_limits<float>::max();

				for (const auto& worker : workers) {
					if (worker == probe_scout) continue;
					if (worker == probe_forward && !work_probe_forward) continue;
					// pick mineral mining workers first.
					if (worker->orders.empty()) continue;
					const UnitOrder& o = worker->orders.front();
					if (o.ability_id != ABILITY_ID::HARVEST_GATHER) continue;
					const Unit* target_resource = observation->GetUnit(o.target_unit_tag);
					Tag target_tag = o.target_unit_tag;
					if (target_resource == nullptr) continue;
					if (target_tag == NullTag) continue;
					if (!IsMineral()(*target_resource)) continue;

					Tag nearest_base = resources_to_nearest_base.count(target_tag) ? resources_to_nearest_base.at(target_tag) : NullTag;
					if (base->tag != nearest_base) continue;

					float current_distance = DistanceSquared2D(gas_base->pos, worker->pos);
					if (current_distance < min_distance) {
						target_worker = worker;
						min_distance = current_distance;
					}
				}

				if (target_worker != nullptr) {
					MineIdleWorkers(target_worker);
					Print("reassigning for gas workers");
					return;
				}
			}
		}
	}

	void ManageUpgrades() {
		const ObservationInterface* observation = Observation();
		size_t forge_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_FORGE);
		auto upgrades = observation->GetUpgrades();
		if (branch == 0 || branch == 1) {
		TryBuildUpgrade(ABILITY_ID::RESEARCH_ADEPTRESONATINGGLAIVES,UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL,UPGRADE_ID::ADEPTPIERCINGATTACK);
		TryBuildUpgradeChrono(ABILITY_ID::RESEARCH_EXTENDEDTHERMALLANCE, UNIT_TYPEID::PROTOSS_ROBOTICSBAY, UPGRADE_ID::EXTENDEDTHERMALLANCE);
		}
		//TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSGROUNDWEAPONS, UNIT_TYPEID::PROTOSS_FORGE);
		if (1) {
            if (forge_count ==0) {
                return;
            }
            TryBuildUpgradeChrono(ABILITY_ID::RESEARCH_PROTOSSGROUNDWEAPONS, UNIT_TYPEID::PROTOSS_FORGE, UPGRADE_ID::PROTOSSGROUNDWEAPONSLEVEL1);
            TryBuildUpgradeChrono(ABILITY_ID::RESEARCH_PROTOSSSHIELDS, UNIT_TYPEID::PROTOSS_FORGE, UPGRADE_ID::PROTOSSSHIELDSLEVEL1);

            for (const auto& upgrade : upgrades) {
                if (upgrade == UPGRADE_ID::PROTOSSGROUNDWEAPONSLEVEL1) {
                    TryBuildUpgradeChrono(ABILITY_ID::RESEARCH_PROTOSSGROUNDWEAPONS, UNIT_TYPEID::PROTOSS_FORGE, UPGRADE_ID::PROTOSSGROUNDWEAPONSLEVEL2);
                }
                else if (upgrade == UPGRADE_ID::PROTOSSGROUNDWEAPONSLEVEL2) {
                    TryBuildUpgradeChrono(ABILITY_ID::RESEARCH_PROTOSSGROUNDWEAPONS, UNIT_TYPEID::PROTOSS_FORGE, UPGRADE_ID::PROTOSSGROUNDWEAPONSLEVEL3);
                }
                else if (upgrade == UPGRADE_ID::PROTOSSSHIELDSLEVEL1) {
                    TryBuildUpgradeChrono(ABILITY_ID::RESEARCH_PROTOSSSHIELDS, UNIT_TYPEID::PROTOSS_FORGE, UPGRADE_ID::PROTOSSSHIELDSLEVEL2);
                }
                else if (upgrade == UPGRADE_ID::PROTOSSSHIELDSLEVEL2) {
                    TryBuildUpgradeChrono(ABILITY_ID::RESEARCH_PROTOSSSHIELDS, UNIT_TYPEID::PROTOSS_FORGE, UPGRADE_ID::PROTOSSSHIELDSLEVEL3);
                }
                else if (upgrade == UPGRADE_ID::PROTOSSGROUNDARMORSLEVEL1) {
                    TryBuildUpgradeChrono(ABILITY_ID::RESEARCH_PROTOSSGROUNDARMOR, UNIT_TYPEID::PROTOSS_FORGE, UPGRADE_ID::PROTOSSGROUNDARMORSLEVEL2);
                }
                else if (upgrade == UPGRADE_ID::PROTOSSGROUNDARMORSLEVEL2) {
                    TryBuildUpgradeChrono(ABILITY_ID::RESEARCH_PROTOSSGROUNDARMOR, UNIT_TYPEID::PROTOSS_FORGE, UPGRADE_ID::PROTOSSGROUNDARMORSLEVEL3);
                }
                else if (upgrade == UPGRADE_ID::PROTOSSGROUNDWEAPONSLEVEL3) {
                    TryBuildUpgradeChrono(ABILITY_ID::RESEARCH_PROTOSSGROUNDARMOR, UNIT_TYPEID::PROTOSS_FORGE, UPGRADE_ID::PROTOSSGROUNDARMORSLEVEL1);
                }
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
		if (TryBuildStructure(build_ability, UNIT_TYPEID::PROTOSS_NEXUS,worker_type, closest_expansion, true) && observation->GetUnits(Unit::Self, IsTownHall()).size() < 4) {
			staging_location_ = Point3D(((staging_location_.x + closest_expansion.x) / 2), ((staging_location_.y + closest_expansion.y) / 2),
				((staging_location_.z + closest_expansion.z) / 2));
			return true;
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
					return TryBuildUnitChrono(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);

				}

			}

		}
		return false;

	}

	bool EarlyStrategy();

	void scoutprobe();

	void determine_enemy_expansion();

	void manageobserver();

	bool TryWarpAdept() {
		return TryWarpUnitPosition(ABILITY_ID::TRAINWARP_ADEPT, advance_pylon_location);
	}
	bool TryWarpStalker() {
		return TryWarpUnitPosition(ABILITY_ID::TRAINWARP_STALKER, front_expansion);
	}
    bool TryWarpTemplar(){
		return TryWarpUnitPosition(ABILITY_ID::TRAINWARP_HIGHTEMPLAR);
    }
    bool TryWarpUnitPosition(AbilityID ability_type_for_unit, const Point2D& pos = Point2D(0,0)) {
        const ObservationInterface* observation = Observation();
        std::vector<PowerSource> power_sources = observation->GetPowerSources();

		UnitTypeID unit_type;
		switch (ability_type_for_unit.ToType()) {
		case ABILITY_ID::TRAINWARP_ZEALOT:
			unit_type = UNIT_TYPEID::PROTOSS_ZEALOT;
			break;
		case ABILITY_ID::TRAINWARP_ADEPT:
			unit_type = UNIT_TYPEID::PROTOSS_ADEPT;
			break;
		case ABILITY_ID::TRAINWARP_SENTRY:
			unit_type = UNIT_TYPEID::PROTOSS_SENTRY;
			break;
		case ABILITY_ID::TRAINWARP_STALKER:
			unit_type = UNIT_TYPEID::PROTOSS_STALKER;
			break;
		case ABILITY_ID::TRAINWARP_DARKTEMPLAR:
			unit_type = UNIT_TYPEID::PROTOSS_DARKTEMPLAR;
			break;
		case ABILITY_ID::TRAINWARP_HIGHTEMPLAR:
			unit_type = UNIT_TYPEID::PROTOSS_HIGHTEMPLAR;
			break;
		default:
			return false;
		}

		//If we are at supply cap, don't build anymore units, unless its an overlord.
		if (observation->GetFoodUsed() + observation->GetUnitTypeData().at(unit_type).food_required > observation->GetFoodCap()) {
			return false;
		}
		if (observation->GetMinerals() < observation->GetUnitTypeData().at(unit_type).mineral_cost || observation->GetVespene() < observation->GetUnitTypeData().at(unit_type).vespene_cost) {
			return false;
		}
        if (observation->GetFoodUsed() >= observation->GetFoodCap()) {
			return false;
		}
		if (power_sources.empty()) {
            return false;
        }

		Filter f = [](const Unit& u) {
			return IsUnit(UNIT_TYPEID::PROTOSS_WARPGATE)(u) && u.build_progress == 1.0f
				&& !IsUnpowered()(u) && u.orders.empty();
		};
		Units warpgates = observation->GetUnits(Unit::Alliance::Self, f);
		size_t warpgates_size = warpgates.size();

        PowerSource& power_source = GetRandomEntry(power_sources);
		if (Point2D(0, 0) != pos) {
			float min_dist = DistanceSquared2D(pos, power_source.position);
			for (const auto& p : power_sources) {
				float current_dist = DistanceSquared2D(pos, p.position);
				if (current_dist < min_dist) {
					power_source = p;
					min_dist = current_dist;
				}
			}
		}

        float radius = power_source.radius;
        float rx = GetRandomScalar();
        float ry = GetRandomScalar();
        Point2D build_location = Point2D(power_source.position.x + rx * radius, power_source.position.y + ry * radius);
		if (!observation->IsPathable(build_location)) return false;

		std::vector<AvailableAbilities> abilities_vector = Query()->GetAbilitiesForUnits(warpgates);

		for (const auto& abilities : abilities_vector) {
			const Unit* target_warpgate = observation->GetUnit(abilities.unit_tag);
			if (target_warpgate == nullptr) continue;
			for (const auto& ability : abilities.abilities) {
				if (ability.ability_id == ability_type_for_unit) {
					Actions()->UnitCommand(target_warpgate, ability_type_for_unit, build_location);
					return true;
				}
			}
		}
        return false;
    }
    bool TryBuildArmyBranch0(){
        const ObservationInterface* observation = Observation();
        Units robotics = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY));
        for (const auto& r : robotics) {
            if (r->orders.empty()) {
                if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_OBSERVER) < 2) {
                    TryBuildUnit(ABILITY_ID::TRAIN_OBSERVER, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_OBSERVER);
                }
                else{
                    TryBuildUnit(ABILITY_ID::TRAIN_COLOSSUS, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_COLOSSUS);
                }
            }
            else {
                TryWarpStalker();
            }
        }
        return false;
    }

    bool TryBuildArmyBranch5(){
        const ObservationInterface* observation = Observation();
        Units robotics = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY));
        for (const auto& r : robotics) {
            if (r->orders.empty()) {
                TryBuildUnit(ABILITY_ID::TRAIN_IMMORTAL, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_IMMORTAL);
            }
            else {
                TryWarpUnitPosition(ABILITY_ID::TRAINWARP_STALKER, front_expansion);
            }
        }
        return false;
    }

    /*bool TryBuildArmyBalance(){
        const ObservationInterface* observation = Observation();
        Units enemy_army = observation->GetUnits(Unit::Alliance::Enemy, IsArmy(observation));
        Units robotics = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY));
        size_t zealot = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ZEALOT);
        size_t stalker = CountUnitType(observation, UNIT_TYPEID::PROTOSS_STALKER);
        size_t immortal = CountUnitType(observation, UNIT_TYPEID::PROTOSS_IMMORTAL);

        double enemy_unit_number[1000]={0,};
        double enemy_unit_all=0;

        if(!enemy_army.empty()&&enemy_army.size()>10){

			if (!try_initialbalance) {
				try_initialbalance = true;
				initial_balance_unit();
			}
			initial_balance_unit();
			initial_build_unit();
            //if(enemy_army.size()>10){
            for (const auto& enemy_unit : enemy_army){
                enemy_unit_number[static_cast<int>(enemy_unit->unit_type)]+=observation->GetUnitTypeData().at(enemy_unit->unit_type).food_required;
                enemy_unit_all+=observation->GetUnitTypeData().at(enemy_unit->unit_type).food_required;
            }
            for (int i=0;i<1000;i++){
                if(enemy_unit_number[i]!=0){
                    for(int j=0;j<1000;j++){
                        if(balance_unit[j][i]==1) build_unit[j]=build_unit[j]+enemy_unit_number[i]/enemy_unit_all;
                        else if(balance_unit[j][i]==-1) build_unit[j]=build_unit[j]-enemy_unit_number[i]/enemy_unit_all;
                    }
                }
            }
        }
            //광전사73 추적자74 불멸자83

        if (build_unit[83]>0.1) {
            for (const auto& r : robotics) {
                if (r->orders.empty()) {
                    TryBuildUnit(ABILITY_ID::TRAIN_IMMORTAL, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_IMMORTAL);
                }
            }
        }

        if (build_unit[73]>build_unit[74]) {
            TryWarpUnitPosition(ABILITY_ID::TRAINWARP_ZEALOT, front_expansion);
        }
        else {
            TryWarpUnitPosition(ABILITY_ID::TRAINWARP_STALKER, front_expansion);
        }
        return false;



    }*/

    uint16_t TryBuildCannonNexus(){
        const ObservationInterface* observation = Observation();
        Units bases = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));
        Units pylons = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PYLON));
        uint16_t cannon = 0;

        for (const auto& b :bases) {
            const Unit* mineral = FindNearestMineralPatch(b->pos);
            if (CountUnitTypeNearLocation(UNIT_TYPEID::PROTOSS_PHOTONCANNON, mineral->pos, 6)>0 && CountUnitTypeNearLocation(UNIT_TYPEID::PROTOSS_PHOTONCANNON, b->pos, 10)>0) {
                cannon++;
                continue;
            }
            if (CountUnitTypeNearLocation(UNIT_TYPEID::PROTOSS_PYLON, mineral->pos, 6)==0) {
                TryBuildPylon(mineral->pos,6,3);
                continue;
            }
            else {
                float rx = GetRandomScalar();
                float ry = GetRandomScalar();
                const Unit* pylon = FindNearestUnit(mineral->pos, pylons);
                Point2D build_location = Point2D(pylon->pos.x + rx * 7, pylon->pos.y + ry * 7);
                if (Distance2D(build_location,mineral->pos)>3) {
                    continue;
                }
                TryBuildStructure(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE, build_location);
            }
        }
        return cannon;
    }

	void scout_all();

	void scoutenemylocation();

	uint32_t last_map_renewal;
	std::unordered_map<Tag, Tag> resources_to_nearest_base;
	std::list<const Unit *> enemy_units_scouter_seen;
	std::list<const Unit *> enemy_townhalls_scouter_seen;
	Point2D recent_probe_scout_location;
	uint32_t recent_probe_scout_loop;

	Point2D advance_pylon_location;

	std::map<Tag, uint32_t> adept_map;
	Flags flags;

	std::string version;
	std::string botname;
	bool EnemyRush;
	bool PhotonRush;
	Point2D pylonlocation;
	Units Killers;

	bool early_strategy;
	bool warpgate_researched;
	bool BlinkResearched;
	bool ColossusRangeUp;
	bool timing_attack;

	const Unit* advance_pylon;
	const Unit* probe_scout;
	const Unit* pylon_first;
	const Unit* probe_forward;

	bool work_probe_forward;
	bool find_enemy_location;
	std::vector<Point3D>::iterator iter_exp;
	Point3D enemy_expansion;

	uint16_t stage_number;
	const Unit* base;
	uint16_t branch;
	const size_t max_worker_count_ = 68;

	uint16_t num_adept = 0;
	uint16_t num_stalker = 0;

	bool try_initialbalance = false;
	bool Timeto_warpzealot = false;
uint16_t try_adept,try_stalker;
};
