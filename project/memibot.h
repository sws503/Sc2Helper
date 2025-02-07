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
#include <unordered_set>
#include <stack>
#include <fstream>
#include "util.h"
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

		int strategy = 0;

		// 테스트맵용
		if (game_info_.enemy_start_locations.empty()) {
			game_info_.enemy_start_locations.push_back(Point2D());
		}
		else {

			//strategy = ReadStats({});
		}


		GetEnemyRace();

		/*std::initializer_list<int> branch_lists;
		switch (enemyrace){
		case Protoss:
			branch_lists = { 6,7 };
			break;
		case Terran:
			branch_lists = { 5,6 };
			break;
		case Zerg:
			branch_lists = { 0,6,7 };
			break;
		case Random:
		default:
			branch_lists = { 0,5,6 };
			break;
		}
		std::vector<int> branches(branch_lists);
		strategy = ReadStats(branches);*/
		branch = 5;
		//tryadeptbranch6 = (branch == 6 && enemyrace == Zerg);

		/*
		// protoss : 1, 2 (기록 없으면 1)
		// terran : 3, 4 (기록 없으면 3)
		// zerg : 5, 6 (기록 없으면 5)
		tryadeptbranch6 = false;
		switch (strategy) {
        case 1:
            branch = 6;
            break;
        case 2:
            branch = 7;
            break;
        case 3:
            branch = 5;
            break;
        case 4:
            branch = 6;
            break;
        case 5:
            branch = 0;
            break;
        case 6:
            branch = 6;
			tryadeptbranch6 = true;
            break;
		case 7:
			branch = 7;
			break;
        default:
            branch = 0;
            break;
		}
		*/
        //branch=6;
		//branch 6 or 7은 이 전에 fix 되어야함

		last_destroyed_enemy_structure_pos = Point2D(0, 0);
		stage_number = 0;
		scout_candidates.clear();
		iter_exp = scout_candidates.end();
		Enemy_front_expansion = Point3D(0, 0, 0);
		recent_probe_scout_location = Point2D(0, 0);
		recent_probe_scout_loop = 0;
		last_dead_probe_pos.clear();
		attackers_observer_tag = NullTag;
		escort_observer_tag = NullTag;
		EscortProbeExpansionPoint = Point2D(0,0);
		next_expansion = Point2D(0,0);

		early_strategy = false;
		warpgate_researched = false;
		charge_researched = false;
		BlinkResearched = false;
		timing_attack = false;
		advance_pylon = nullptr;
		probe_scout = nullptr;
		pylon_first = nullptr;
		probe_forward = nullptr;
		pylon_cannon = nullptr;
		base = nullptr;
		find_enemy_location = false;
		work_probe_forward = true;

		shield3 = false;
		num_zealot = 0;
		num_adept = 0;
		num_stalker = 0;
		num_warpprism = 0;
		num_colossus = 0;
        num_voidray = 0;
		try_colossus = 0;
		try_immortal = 0;
		num_carrier = 0;
		num_expand = 3;

		last_map_renewal = 0;
		resources_to_nearest_base.clear();

		flags.reset();

		enemy_units_scouter_seen.clear();
		enemy_townhalls_scouter_seen.clear();
		adept_map.clear();
		observer_nexus_match.clear();

		enemyUnitsInRegion.clear();
		Attackers.clear();
		AttackersRecruiting.clear();

		emergency_killerworkers.clear();
		try_initialbalance = false;
		the_pylon = nullptr;

		//Temporary, we can replace this with observation->GetStartLocation() once implemented
		startLocation_ = Observation()->GetStartLocation();
		staging_location_ = startLocation_;

		// 본진 좌표가 (0,0)으로 나오는 것 수정.
		for (auto& e : expansions_) {
			if (Point2D(0, 0) == e) {
				e.x = startLocation_.x;
				e.y = startLocation_.y;
				break;
			}
		}

		for (const auto& e : expansions_) {
			std::cout << e.x << ", " << e.y << ", " << e.z << std::endl;
		}

		float minimum_distance = std::numeric_limits<float>::max();
		for (const auto& expansion : expansions_) {
			float current_distance = Query()->PathingDistance(Point2D(startLocation_.x+3,startLocation_.y+3), expansion);
			Point2D enemy = Point2D(game_info_.enemy_start_locations.front().x+3,game_info_.enemy_start_locations.front().y+3);
			if (current_distance < 5.0f) {
				continue;
			}
			if (Query()->PathingDistance(Point2D(startLocation_.x+3,startLocation_.y+3),enemy)<Query()->PathingDistance(expansion,enemy)) {
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

	virtual void OnGameEnd() final override{
		WriteStats();
	}

	virtual void OnStep() final override {
		const ObservationInterface* observation = Observation();
		//playerbot();
		//return;

		if (warpgate_researched) {
            ConvertGateWayToWarpGate();
		}

		if (observation->GetGameLoop() % 3 == 0) {
			ManageWorkers();
		}

		if (!early_strategy && observation->GetGameLoop()%5==0) {
			EarlyStrategy();
		}

		//WriteStats();
		//write(observation,enemyrace);



		scout_all();

#ifdef DEBUG
		// 주석 풀면 느려짐
		// PrintCursor();
#endif

		if (observation->GetGameLoop()%10==0) {
            ManageUpgrades();
            UnpoweredBuilding();
		}

		// Control 시작
		if (observation->GetGameLoop() % 5 == 0) {
			Defend();
			ManageTimingAttack();
			ManageRush();

		}
	}

	virtual void OnUnitIdle(const Unit* unit) final override {
		switch (unit->unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_PROBE: {
			if (probe_scout != nullptr && probe_scout->tag == unit->tag) {
				return;
			}
			if (probe_forward != nullptr && probe_forward->tag == unit->tag && !work_probe_forward) {
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
		case UNIT_TYPEID::PROTOSS_OBSERVER: {

			break;
		}
		default: {
			break;
		}
		}
		return;
	}

    virtual void OnUpgradeCompleted(UpgradeID upgrade) final override {
		if (branch == 5)
		{
			switch (upgrade.ToType()) {
			case UPGRADE_ID::PROTOSSGROUNDWEAPONSLEVEL1: {
				std::cout << "attack1";
				//timing_attack = true;
				return;
			}
			case UPGRADE_ID::PROTOSSGROUNDWEAPONSLEVEL2: {
				std::cout << "attack2";
				timing_attack = true;
				return;
			}
			case UPGRADE_ID::PROTOSSGROUNDWEAPONSLEVEL3: {
				std::cout << "attack3";
				timing_attack = true;
				return;
			}
			case UPGRADE_ID::PROTOSSGROUNDARMORSLEVEL1: {
				std::cout << "attack3";
				timing_attack = true;
				return;
			}
			case UPGRADE_ID::PROTOSSGROUNDARMORSLEVEL2: {
				std::cout << "attack3";
				timing_attack = true;
				return;
			}
			case UPGRADE_ID::PROTOSSGROUNDARMORSLEVEL3: {
				std::cout << "attack3";
				timing_attack = true;
				return;
			}
			case UPGRADE_ID::PROTOSSSHIELDSLEVEL1: {
			    num_expand=4;
                return;
			}
			case UPGRADE_ID::PROTOSSSHIELDSLEVEL2: {
                num_expand=5;
                return;
			}
			case UPGRADE_ID::PROTOSSSHIELDSLEVEL3: {
                num_expand=6;
                shield3 = true;
                return;
			}
			default:
				break;
			}
		}

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
            case UPGRADE_ID::WARPGATERESEARCH: {
                warpgate_researched = true;
                return;
            }
			case UPGRADE_ID::CHARGE: {
				charge_researched = true;
			}
            default:
                break;
        }
    }

	virtual void OnBuildingConstructionComplete(const Unit* u) final override {
		Print(UnitTypeToName(u->unit_type.ToType()));
		if (u->alliance == Unit::Alliance::Self) {
			/*switch (u->unit_type.ToType()) {
			default:
				break;
			}*/
		}
	}

	bool phasing_check()
	{
		size_t Phasing_count = CountUnitType(Observation(), UNIT_TYPEID::PROTOSS_WARPPRISMPHASING);

		if (Phasing_count)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	// only allies. also geyser probes, passengers etc.
	virtual void OnUnitCreated(const Unit* u) final override {
		switch (u->unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_ZEALOT:
			num_zealot++;
			break;
		case UNIT_TYPEID::PROTOSS_ADEPT:
			num_adept++;

			if (phasing_check())
			{
				Attackers.push_back(u);
			}

			break;
		case UNIT_TYPEID::PROTOSS_STALKER:
			num_stalker++;
			break;
		case UNIT_TYPEID::PROTOSS_COLOSSUS:
			num_colossus++;
			try_colossus++;
			break;
        case UNIT_TYPEID::PROTOSS_VOIDRAY:
            num_voidray++;
		case UNIT_TYPEID::PROTOSS_IMMORTAL:
			try_immortal++;
			break;
		case UNIT_TYPEID::PROTOSS_CARRIER:
			num_carrier++;
			break;
		case UNIT_TYPEID::PROTOSS_TEMPEST:
			num_carrier++;
			break;
		default:

			break;
		}
	}

	virtual void OnUnitDestroyed(const Unit* u) final override {
		Print(UnitTypeToName(u->unit_type.ToType()));
		if (u->alliance == Unit::Alliance::Self) {
			// Attackers에서 죽은 유닛 제거
			if (IsUnitInUnits(u, Attackers)) {
				int num_attackers = 0;
				int num_colossusssss = 0;
				std::unordered_set<const Unit*> TempAttackers;
				for (const auto& u : Attackers) {
					if (u->is_alive && !TempAttackers.count(u)) {
						TempAttackers.insert(u);
						num_attackers++;
						num_colossusssss += IsUnit(UNIT_TYPEID::PROTOSS_COLOSSUS)(*u);
					}
				}
				Attackers.clear();
				// colossus만 남아 있으면 attackers 해산
				if (num_attackers > num_colossusssss * 2) {
					for (const auto& u : TempAttackers) {
						Attackers.push_back(u);
					}
				}
				// attackersrecruiting 있으면 거기로 편입
				else if (!AttackersRecruiting.empty()){
					for (const auto& u : TempAttackers) {
						AttackersRecruiting.push_back(u);
					}
				}
			}
			else if (IsUnitInUnits(u, AttackersRecruiting)) {
				std::unordered_set<const Unit*> TempAttackers;
				for (const auto& u : AttackersRecruiting) {
					if (u->is_alive && !TempAttackers.count(u)) {
						TempAttackers.insert(u);
					}
				}
				AttackersRecruiting.clear();
				for (const auto& u : TempAttackers) {
					AttackersRecruiting.push_back(u);
				}
			}
			if (IsWorker()(*u)) {
				FleeWorkers(u);
			}
			switch (u->unit_type.ToType()) {
			case UNIT_TYPEID::PROTOSS_PROBE:
				last_dead_probe_pos.push_back(u->pos);
				break;
			default:
				break;
			}
		}
		if (u->alliance == Unit::Alliance::Enemy) {
			bool removed = false;
			for (auto& it = enemy_units_scouter_seen.begin(); it != enemy_units_scouter_seen.end();) {
				if ((*it)->tag == u->tag) {
					removed = true;
					it = enemy_units_scouter_seen.erase(it);
					break;
				}
				else {
					++it;
				}
			}
			if (removed && enemy_units_scouter_seen.empty()) {
				last_destroyed_enemy_structure_pos = u->pos;
			}
			for (auto& it = enemy_townhalls_scouter_seen.begin(); it != enemy_townhalls_scouter_seen.end();) {
				if ((*it)->tag == u->tag) {
					it = enemy_townhalls_scouter_seen.erase(it);
					break;
				}
				else {
					++it;
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

				targetPositionNew = sc2::Point2D(x_min, targetPosition.y);
			}
			else if (targetPosition.x > x_max)
			{
				targetPositionNew = sc2::Point2D(x_max, targetPosition.y);
			}
			else if (targetPosition.y < y_min)
			{
				targetPositionNew = sc2::Point2D(targetPosition.x, y_min);
			}
			else if (targetPosition.y > y_max)
			{

				targetPositionNew = sc2::Point2D(targetPosition.x, y_max);
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
	Units AttackersRecruiting;
	const Unit * the_pylon;
	Point2D* the_pylon_pos;

	bool SetOpponentID(const std::string& opponentid);

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

	int t_sleep = 1;

	void RealChat(std::string Message) // 릴리즈에서도 채팅 가능
	{
		if (t_sleep == 1) {
			Actions()->SendChat(Message);
			t_sleep = 0;
			SleepFor(100);
			t_sleep = 1;
		}
		else
		{
			std::cout << "채팅 기다리는 중" << std::endl;
		}
	}


	void DrawLine(Point3D p0, Point3D p1) {
#ifdef DEBUG
		Debug()->DebugLineOut(p0, p1);
		Debug()->SendDebug();
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

	// Todo: 궤멸충 담즙 멀리 피하기
	bool EvadeEffect(const Unit* unit)
	{
		const ObservationInterface* observation = Observation();
		bool moving = false;
		Vector2D mul_diff(0, 0);
		for (const auto & effect : observation->GetEffects())
		{
			if (isBadEffect(effect.effect_id))
			{
				const EffectData& ed = observation->GetEffectData().at(effect.effect_id);
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
						if (dist > 0)
						{
							Vector2D diff = unit->pos - pos; // 7.3 적 유닛과의 반대 방향으로 도망
							Normalize2D(diff);
							mul_diff += diff * (1.0f /*+ radius + unit->radius - dist*/);
							//fleeingPos = pos + normalizeVector(rangedUnit->getPos() - pos, radius + 2.0f);
						}
						else{}

						if (EffectID(effect.effect_id).ToType() == EFFECT_ID::LIBERATORMORPHED || EffectID(effect.effect_id).ToType() == EFFECT_ID::LIBERATORMORPHING)
						{
							Units bases = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));

							for (const auto & base : bases)
							{
								if (Distance2D(unit->pos, pos) < 10)
								{
									return false;
								}
							}
						}
						std::cout << "skill : " << ed.friendly_name << std::endl;
						moving = true;
						break;
					}
				}
			}
			if (moving) break;
		}
		if (!moving) {
			Units explodingunits = observation->GetUnits(Unit::Alliance::Enemy,
				IsUnits({UNIT_TYPEID::PROTOSS_DISRUPTORPHASED}));
			const Unit* nearestu = FindNearestUnit(unit->pos, explodingunits, 4.0f);
			if (nearestu != nullptr) {
				Point2D pos = nearestu->pos;
				Vector2D diff = unit->pos - pos; // 7.3 적 유닛과의 반대 방향으로 도망
				Normalize2D(diff);
				mul_diff += diff * (2.0f /*+ radius + unit->radius - dist*/);
				moving = true;
			}
		}
		if (moving) {
			Point2D fleeingPos(staging_location_);
			if (mul_diff != Point2D(0, 0)) {
				fleeingPos = unit->pos + mul_diff;
			}
			SmartMove(unit, fleeingPos);
			Chat("Enemy Skill Run~");
			DrawLine(unit->pos, Point3D(fleeingPos.x, fleeingPos.y, unit->pos.z));
			return true;
		}

		return moving;
	}

	bool DefendDuty(const Unit * unit);

	bool DefendDutyAttack(const Unit * unit);

	void Defend();

	void cancelnexus();

	void ManageWarpBlink(const Unit * unit);

	bool IsUnitInUnits(const Unit * unit, Units & units);

	void OrganizeSquad();

	void DoGuerrillaWarp(const Unit * unit);

	void Merge(const Unit * unit, Point2D mergelocation);

	void ManageTimingAttack();

	void ManageRush();

	void ManageOracleBeam(const Unit * unit, const Unit * target);

	void Roam_enemybase(const Unit * unit);

	void Roam_randombase(const Unit * unit);

	void AdeptPhaseShift(const Unit * unit, Units ShadeNearEnemies, Units NearbyEnemies, bool & ComeOn);

	void AdeptPhaseToLocation(const Unit * unit, Point2D Location, bool & Timer, bool & ComeOn);

	void ManageBlink(const Unit * unit, const Unit * enemyarmy);

	void StalkerBlinkEscape(const Unit * unit, const Unit * enemyarmy);

	void StalkerBlinkForward(const Unit * unit, const Unit * enemyarmy);

	void FleeKiting(const Unit * unit, const Unit * enemyarmy);

	void OracleBackKiting(const Unit * unit, const Unit * Workertarget, const Unit * Armytarget);

	void DistanceKiting(const Unit * unit, const Unit * enemyarmy, const Unit * army);

	void FrontKiting(const Unit * unit, const Unit * enemyarmy);

	bool CanHitMeGROUND(const Unit * unit);

	bool CanHitMe(const Unit * unit, float distance = 1.0f);

	void ComeOnKiting(const Unit * unit, const Unit * enemyarmy);

	void CarrierKiting(const Unit * unit, const Unit * enemyarmy);

	bool ChargeShield(const Unit * unit);

	void VoidRayKiting(const Unit * unit, const Unit * enemyarmy);

	void OracleKiting(const Unit * unit, const Unit * enemyarmy);

	void ColossusKiting(const Unit * unit, const Unit * enemyarmy);

	void SentryKiting(const Unit * unit, const Unit * enemyarmy);

	void Kiting(const Unit * unit, const Unit * enemyarmy);

	void EvadeKiting(const Unit * unit, const Unit * enemyarmy);

	void SmartMoveEfficient(const Unit * unit, Point2D KitingLocation, const Unit * enemyarmy);


	void EmergencyKiting(const Unit * unit, const Unit * enemyarmy);

	bool GetPosition(Units & units, Point2D & position);

	void KiteEnemy(const Unit * unit, Units enemy_army, Units enemy_units, Point2D KitingLocation, bool enemiesnear, const ObservationInterface * observation);


	float MinimumDistance2D(const Unit * unit, const Unit * enemyarmy);


	Point2D CalcKitingPosition(Point2D Mypos, Point2D EnemyPos);

	void PredictKiting(const Unit * unit, const Unit * enemyarmy);

	bool GetPosition(Units Enemyunits, Unit::Alliance alliace, Point2D & position);

	bool GetPosition(UNIT_TYPEID unit_type, Unit::Alliance alliace, Point2D & position);

	int getRushPriority(const Unit * u);

	int getAttackPriority(const Unit * u);

	int getOraclePriority(const Unit * u);


	bool IsBonusType(const Unit * rangedUnit, const Unit * target);

	const Unit * GetOracleTarget(const Unit * rangedUnit, Units & targets);

	bool LoadUnit(const Unit * unit, const Unit * passenger);

	bool LoadUnitWeaponCooldown(const Unit * unit, const Unit * passenger);

	const Unit * GetPassenger(const Unit * shuttle, Units & targets);

	const Unit * GetNearShuttle(const Unit * unit);

	void ManageWarpBlink(const Unit * unit, const Unit * shuttle);

	const Unit * GetRushTarget(const Unit * rangedUnit, Units & targets);

	const Unit * GetTarget(const Unit * rangedUnit, Units & targets);

	const Unit * GetOracleRushTarget(const Unit * rangedUnit, Units & targets);

	const Unit * GetZealotTarget(const Unit * rangedUnit, Units & targets);

	const Unit * GetNearTarget(const Unit * rangedUnit, Units & targets);

	const float getunitsDpsGROUND(Units targets) const;

	const float getDpsGROUND(const Unit * target) const;

	const float getAttackRangeAIR(const Unit * target) const;

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

		Point2D target_pos;

		// 가장 마지막으로 본 적의 위치를 target_pos 로 리턴
		// last_destroyed_enemy_structure_pos : 마지막으로 터진 건물
		if (FindEnemyPosition(target_pos)) { //적 기지를 알고있는 상황이면
			if (!enemy_units.empty()) // 적 유닛이 있는 상황이면 또는 이 유닛이 적 기지 근처에 없는상황이면
			{
				const Unit * closest_enemy = FindNearestUnit(unit->pos, enemy_units);
				SmartAttackUnit(unit, closest_enemy);
				return;
			}
			else if (!unit->orders.empty()) {
				return;
			}
			else if (Distance2D(unit->pos, target_pos) < 20) { //적 유닛이 없는 상황에서 적 기지가 근처에 있으면
				if (TryFindRandomPathableLocation(unit, target_pos)) { //유닛별로 맵 전체적으로 퍼지는 위치를 배정받고
					SmartAttackMove(unit, target_pos); //그 위치로 간다
					return;
				}
			}
			SmartAttackMove(unit, target_pos); //위 작업이 끝나면 적 기지를 다시한번 간다
		}
		else { //적 기지도 모르면 막 돌아다녀라
			if (!unit->orders.empty()) {
				return;
			}
			else if (TryFindRandomPathableLocation(unit, target_pos)) {
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
			else if (!unit->orders.empty() && unit->orders.front().ability_id == ABILITY_ID::MOVE) // 움직이고 있는 상황일 때도
			{

			}
			else //너가 아무것도 안하고 있었다면
			{
				SmartMove(unit, retreat_position); // 움직여라
			}
		}
		return moving;
	}

	bool RetreatPrism(const Unit* unit, Point2D retreat_position) {
		bool moving = false;

		float dist = Distance2D(unit->pos, retreat_position);

		if (dist >= 10) // 멀리있으면
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
		target_pos = (last_destroyed_enemy_structure_pos == Point2D(0,0))? game_info_.enemy_start_locations.front(): last_destroyed_enemy_structure_pos;
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

	const Unit* FindNearestUnit(const Point2D& start, const std::list<const Unit *> &units, float max_d = std::numeric_limits<float>::max()) const {
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

	const Unit* FindSecondNearestUnit(const Point2D& start, Filter f = {}) const {
		const Units units = Observation()->GetUnits(f);
		return FindSecondNearestUnit(start, units);
	}

	const Unit* FindSecondNearestUnit(const Point2D& start, Unit::Alliance a, Filter f = {}) const {
		const Units units = Observation()->GetUnits(a, f);
		return FindSecondNearestUnit(start, units);
	}

	const Unit* FindSecondNearestUnit(const Point2D& start, const Units& units) const {
		float nearest_distance = std::numeric_limits<float>::max();
		float nearest_distance2 = std::numeric_limits<float>::max();
		const Unit* nearest_unit = nullptr;
		const Unit* nearest_unit2 = nullptr;
		for (const auto& u : units) {
			float current_distance = DistanceSquared2D(u->pos, start);
			if (current_distance < nearest_distance2) {
				nearest_distance2 = current_distance;
				nearest_unit2 = u;
			}
			if (nearest_distance2 < nearest_distance) {
				float tmp_distance;
				const Unit* tmp_unit;
				tmp_distance = nearest_distance;
				nearest_distance = nearest_distance2;
				nearest_distance2 = tmp_distance;
				tmp_unit = nearest_unit;
				nearest_unit = nearest_unit2;
				nearest_unit2 = tmp_unit;
			}
		}

		//If we never found one return nullptr
		return nearest_unit2;
	}

	const Unit* FindSecondNearestUnit(const Point2D& start, const std::list<const Unit *>& units) const {
		float nearest_distance = std::numeric_limits<float>::max();
		float nearest_distance2 = std::numeric_limits<float>::max();
		const Unit* nearest_unit = nullptr;
		const Unit* nearest_unit2 = nullptr;
		for (const auto& u : units) {
			float current_distance = DistanceSquared2D(u->pos, start);
			if (current_distance < nearest_distance2) {
				nearest_distance2 = current_distance;
				nearest_unit2 = u;
			}
			if (nearest_distance2 < nearest_distance) {
				float tmp_distance;
				const Unit* tmp_unit;
				tmp_distance = nearest_distance;
				nearest_distance = nearest_distance2;
				nearest_distance2 = tmp_distance;
				tmp_unit = nearest_unit;
				nearest_unit = nearest_unit2;
				nearest_unit2 = tmp_unit;
			}
		}

		//If we never found one return nullptr
		return nearest_unit2;
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
		if (unit->build_progress < 1.0f) return false;
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

		if (observation->GetMinerals() < observation->GetUnitTypeData().at(unit_type).mineral_cost
			|| observation->GetVespene() < observation->GetUnitTypeData().at(unit_type).vespene_cost) {
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
		if (observation->GetMinerals() < observation->GetUnitTypeData().at(unit_type).mineral_cost
			|| observation->GetVespene() < observation->GetUnitTypeData().at(unit_type).vespene_cost) {
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

		if ((uint32_t)observation->GetMinerals() < observation->GetUpgradeData().at(upgrade_type).mineral_cost
			|| (uint32_t)observation->GetVespene() < observation->GetUpgradeData().at(upgrade_type).vespene_cost) {
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
		
        if ((uint32_t)observation->GetMinerals() < observation->GetUpgradeData().at(upgrade_type).mineral_cost
			|| (uint32_t)observation->GetVespene() < observation->GetUpgradeData().at(upgrade_type).vespene_cost) {
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
		RealChat("Bot is trying to upgrade " + observation->GetUpgradeData().at(upgrade_type).name);
		Actions()->UnitCommand(unit, ability_type_for_unit);
		return true;
	}

	bool TryWarpUnitPrism(AbilityID ability_type_for_unit) {
        const ObservationInterface* observation = Observation();
        std::vector<PowerSource> power_sources = observation->GetPowerSources();
        Units warpgates = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_WARPGATE));
		if (observation->GetFoodUsed() + 2 > observation->GetFoodCap()) {
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
		//if (!observation->IsPathable(build_location)) return false;

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

		if (observation->GetMinerals() < observation->GetUnitTypeData().at(building_type).mineral_cost
			|| observation->GetVespene() < observation->GetUnitTypeData().at(building_type).vespene_cost) {
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

		// keep expansion site clean.
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
			if (d < 0.01f && DistanceSquared2D(query_vector.at(i).start_, location) > 0.5f) {
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

	bool TryBuildStructureAtLocation(AbilityID ability_type_for_structure, UnitTypeID building_type, Point2D location) {
        const ObservationInterface* observation = Observation();
		Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PROBE));
		
		if (observation->GetMinerals() < observation->GetUnitTypeData().at(building_type).mineral_cost || observation->GetVespene() < observation->GetUnitTypeData().at(building_type).vespene_cost) {
            return false;
		}
		
		//if we have no workers Don't build
		if (workers.empty()) {
			return false;
		}
		

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
			if (d < 0.01f && DistanceSquared2D(query_vector.at(i).start_, location) > 0.5f) {
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

		if (observation->GetMinerals() < observation->GetUnitTypeData().at(building_type).mineral_cost
			|| observation->GetVespene() < observation->GetUnitTypeData().at(building_type).vespene_cost) {
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

		if (observation->GetMinerals() < observation->GetUnitTypeData().at(building_type).mineral_cost
			|| observation->GetVespene() < observation->GetUnitTypeData().at(building_type).vespene_cost) {
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
		if (observation->GetUnit(random_power_source.tag) == nullptr) {
			return false;
		}
		if (observation->GetUnit(random_power_source.tag)->unit_type == UNIT_TYPEID::PROTOSS_WARPPRISMPHASING) {
			return false;
		}
		// do not build near the_pylon
		if (branch == 7 && the_pylon_pos != nullptr && *the_pylon_pos == random_power_source.position) {
			if (observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PYLON)).size() >= 2) {
				return false;
			}
		}

		float radius = random_power_source.radius;
		float rx = GetRandomScalar();
		float ry = GetRandomScalar();
		Point2D build_location = Point2D(random_power_source.position.x + rx * radius, random_power_source.position.y + ry * radius);
		return TryBuildStructure(ability_type_for_structure, building_type, UNIT_TYPEID::PROTOSS_PROBE, build_location);
	}

	bool TryBuildStructureNearPylonInBase(AbilityID ability_type_for_structure, UnitTypeID building_type) {
		const ObservationInterface* observation = Observation();

		if (observation->GetMinerals() < observation->GetUnitTypeData().at(building_type).mineral_cost
			|| observation->GetVespene() < observation->GetUnitTypeData().at(building_type).vespene_cost) {
            return false;
		}

		//Need to check to make sure its a pylon instead of a warp prism
		std::vector<PowerSource> power_sources = observation->GetPowerSources();
		if (power_sources.empty()) {
			return false;
		}

		const PowerSource& random_power_source = GetRandomEntry(power_sources);
		if (Distance2D(random_power_source.position,base->pos)>30) {
            return false;
		}

		if (advance_pylon !=nullptr && Distance2D(random_power_source.position,advance_pylon->pos)<10) {
            return false;
		}
		if (observation->GetUnit(random_power_source.tag) == nullptr) {
			return false;
		}
		if (observation->GetUnit(random_power_source.tag)->unit_type == UNIT_TYPEID::PROTOSS_WARPPRISMPHASING) {
			return false;
		}
		// do not build near the_pylon
		if (branch == 7 && the_pylon_pos != nullptr && *the_pylon_pos == random_power_source.position) {
			if (observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PYLON)).size() >= 2) {
				return false;
			}
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

		if (observation->GetMinerals()<75) {
            return false;
		}
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

		//if (TryBuildPylon(startLocation_, 15.0f, i)) return true;
		Units bases = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));
		return TryBuildPylon(bases[GetRandomInteger(0, bases.size() - 1)]->pos, 7.0f, i);
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

	bool TryBuildSupply() {
		const ObservationInterface* observation = Observation();
		Units bases = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_COMMANDCENTER));

		if (bases.size() == 0) {
			return false;
		}

		const Unit* base = bases[GetRandomInteger(0, bases.size()-1)];

		if (observation->GetMinerals() < 100) {
			return false;
		}

		//check to see if there is already on building
		Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_SUPPLYDEPOT));

		int MaxBuildAtOnce;

		if (observation->GetFoodUsed() < 50) MaxBuildAtOnce = 1;
		else if (observation->GetFoodUsed() < 100) MaxBuildAtOnce = 2;
		else MaxBuildAtOnce = 3;


		int NumBuildInProgress = 0;
		for (const auto& unit : units) {
			if (unit->build_progress != 1) {
				NumBuildInProgress++;
				if (NumBuildInProgress >= MaxBuildAtOnce) {
					return false;
				}
			}
		}

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

		// Try and build a pylon. Find a random Probe and give it the order.
		float rx = GetRandomScalar();
		float ry = GetRandomScalar();
		Point2D build_location = Point2D(base->pos.x + rx * 15, base->pos.y + ry * 15);
		return TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, UNIT_TYPEID::TERRAN_SCV, build_location);


	}

	void playerbot() {
		const ObservationInterface* observation = Observation();

		TryBuildSupply();
		
	}

	void MakeBaseResourceMap();

	void MineIdleWorkers(const Unit* worker);

	int GetExpectedWorkers(UNIT_TYPEID vespene_building_type);

	void ManageWorkers();

	void FleeWorkers(const Unit * unit);

	void ControlWorkers();

	bool EvadeExplosiveUnits(const Unit * unit);

	void ManageUpgrades() {
		const ObservationInterface* observation = Observation();
		Units forges = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_FORGE));
		size_t base_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_NEXUS);
		size_t cybernetics_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
		Units cores = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_CYBERNETICSCORE));
		Units twilightcouncil = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL));
		auto upgrades = observation->GetUpgrades();

		
		if (branch == 5) {
			if (!warpgate_researched && cybernetics_count > 0 && cores.front()->orders.empty()) {
				TryBuildUpgrade(ABILITY_ID::RESEARCH_WARPGATE, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE, UPGRADE_ID::WARPGATERESEARCH);
			}
			if (BlinkResearched && !charge_researched && twilightcouncil.size() > 0 && twilightcouncil.front()->orders.empty()) {
				TryBuildUpgrade(ABILITY_ID::RESEARCH_CHARGE, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL, UPGRADE_ID::CHARGE);
			}


            TryBuildUpgrade(ABILITY_ID::RESEARCH_EXTENDEDTHERMALLANCE, UNIT_TYPEID::PROTOSS_ROBOTICSBAY, UPGRADE_ID::GRAVITICDRIVE);
            for (const auto& forge : forges) {
                if (!forge->orders.empty()) {
                    TryChronoboost(forge);
                }
		    }
            if (forges.size() ==0) {
                return;
            }
            TryBuildUpgrade(ABILITY_ID::RESEARCH_PROTOSSGROUNDWEAPONS, UNIT_TYPEID::PROTOSS_FORGE, UPGRADE_ID::PROTOSSGROUNDWEAPONSLEVEL1);
            TryBuildUpgrade(ABILITY_ID::RESEARCH_PROTOSSSHIELDS, UNIT_TYPEID::PROTOSS_FORGE, UPGRADE_ID::PROTOSSSHIELDSLEVEL1);
            if (base_count>3) {
                TryBuildUpgrade(ABILITY_ID::RESEARCH_GRAVITICDRIVE, UNIT_TYPEID::PROTOSS_ROBOTICSBAY, UPGRADE_ID::PROTOSSSHIELDSLEVEL1);
            }

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
		Units bases = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));
		for (const auto& b :bases) {
            if (b->build_progress != 1.0f) {
                return false;
            }
		}
		if (ManyEnemyRush) {
            return false;
		}

		float minimum_distance = std::numeric_limits<float>::max();
		Point3D closest_expansion;
		if (bases.size()==1 && base!=nullptr) {
            closest_expansion = front_expansion;
		}
		else {
            for (const auto& expansion : expansions_) {
                float current_distance = Query()->PathingDistance(Point2D(startLocation_.x+3,startLocation_.y+3), expansion);
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
		}
		next_expansion = closest_expansion;
		//only update staging location up till 3 bases.
		if (TryBuildStructure(build_ability, UNIT_TYPEID::PROTOSS_NEXUS,worker_type, closest_expansion, true) && observation->GetUnits(Unit::Self, IsTownHall()).size() < 4) {
			staging_location_ = Point3D(((staging_location_.x + closest_expansion.x) / 2), ((staging_location_.y + closest_expansion.y) / 2),
				((staging_location_.z + closest_expansion.z) / 2));
			EscortProbeExpansionPoint = closest_expansion;
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
			if (base->build_progress == 1.0f && base->assigned_harvesters >= base->ideal_harvesters) {
				if (TryBuildGas(base->pos)) {
					return true;
				}
			}
			if (base->build_progress == 1.0f && observation->GetMinerals()>1000 && observation->GetVespene()<200) {
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

	bool determine_scout_location(const Unit* u);

	void find_next_scout_location();

	void determine_enemy_expansion();

	void manageobserver();

	void roamobserver(const Unit * u);

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
		if (observation->GetMinerals() < observation->GetUnitTypeData().at(unit_type).mineral_cost
			|| observation->GetVespene() < observation->GetUnitTypeData().at(unit_type).vespene_cost) {
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
		//if (!observation->IsPathable(build_location)) return false;

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

    bool UnpoweredBuilding(){
        const ObservationInterface* observation = Observation();
        Units buildings = observation->GetUnits(Unit::Alliance::Self, IsUnpowered());
        for (const auto& b : buildings) {
            if (CountUnitTypeNearLocation(UNIT_TYPEID::PROTOSS_PYLON, b->pos, 5.5f)>0) continue;
            return TryBuildPylon(b->pos, 5.5f);
        }
        return false;
    }

    bool TryBuildArmyBranch5(){
        const ObservationInterface* observation = Observation();
        Units robotics = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY));
        Units observers = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_OBSERVER));
        Units roboticsbay = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_ROBOTICSBAY));
        Units warpprisms_phasing = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_WARPPRISMPHASING));

        size_t sentry_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_SENTRY);

        int robotics_empty=0;
        int robotics_observer=0;
        for (const auto& r : robotics) {
            if (r->build_progress<1.0f) {
                continue;
            }
            if (r->orders.empty()) {
                robotics_empty++;
            }
            else {
                if (r->orders.front().ability_id == ABILITY_ID::TRAIN_OBSERVER) {
                    robotics_observer++;
                }
                if (r->orders.front().ability_id == ABILITY_ID::TRAIN_WARPPRISM) {
                    num_warpprism=1;
                }
                if (shield3) {
                    TryChronoboost(r);
                }
            }
        }

        if (robotics_empty==0) {
            if (!warpprisms_phasing.empty()) {
                return TryWarpUnitPrism(ABILITY_ID::TRAINWARP_ZEALOT);
            }
            else if (sentry_count<2) {
                return TryWarpUnitPosition(ABILITY_ID::TRAINWARP_SENTRY, front_expansion);
            }
            else {
                return TryWarpUnitPosition(ABILITY_ID::TRAINWARP_STALKER, front_expansion);
            }
        }
        else if (observers.size()+robotics_observer<3) {
            return TryBuildUnit(ABILITY_ID::TRAIN_OBSERVER, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_OBSERVER);
        }
        else if (num_warpprism==0) {
            return TryBuildUnit(ABILITY_ID::TRAIN_WARPPRISM, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_WARPPRISM);
        }
        else {
            if (try_colossus>=try_immortal+1 || roboticsbay.empty()) {
                return TryBuildUnit(ABILITY_ID::TRAIN_IMMORTAL, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_IMMORTAL);
            }
            if (roboticsbay.front()->build_progress<1.0f) {
                return TryBuildUnit(ABILITY_ID::TRAIN_IMMORTAL, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_IMMORTAL);
            }
            return TryBuildUnit(ABILITY_ID::TRAIN_COLOSSUS, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_COLOSSUS);
        }
    }


    void TryBuildCannonNexus(int num = 1){
        const ObservationInterface* observation = Observation();
        Units bases = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));
        Units pylons = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PYLON));
        for (const auto& b :bases) {
            const Unit* mineral = FindNearestMineralPatch(b->pos, 15);
			if (mineral == nullptr) continue;
            if (CountUnitTypeNearLocation(UNIT_TYPEID::PROTOSS_PHOTONCANNON, mineral->pos, 6)>=num && CountUnitTypeNearLocation(UNIT_TYPEID::PROTOSS_PHOTONCANNON, b->pos, 10)>0) {
                continue;
            }
            if (CountUnitTypeNearLocation(UNIT_TYPEID::PROTOSS_PYLON, mineral->pos, 6)==0) {
                TryBuildPylon(mineral->pos,6,3);
                continue;
            }

			float rx = GetRandomScalar();
			float ry = GetRandomScalar();
			const Unit* pylon = FindNearestUnit(mineral->pos, pylons);
			Point2D build_location = Point2D(pylon->pos.x + rx * 7, pylon->pos.y + ry * 7);
			if (Distance2D(build_location, mineral->pos)>3) {
				continue;
			}
			TryBuildStructure(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE, build_location);

        }
    }

    bool TryBuildBatteryNexus(const Unit* base_){
        const ObservationInterface* observation = Observation();
        Units pylons = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PYLON));

        if (CountUnitTypeNearLocation(UNIT_TYPEID::PROTOSS_SHIELDBATTERY, base_->pos, 8)>0) {
            return true;
        }
        if (CountUnitTypeNearLocation(UNIT_TYPEID::PROTOSS_PYLON, base_->pos, 12)==0) {
            TryBuildPylon(base_->pos,6,3);
            return false;;
        }
        else {
            float rx = GetRandomScalar();
            float ry = GetRandomScalar();
            const Unit* pylon = FindNearestUnit(base_->pos, pylons);
            Point2D build_location = Point2D(pylon->pos.x + rx * 7, pylon->pos.y + ry * 7);
            if (Distance2D(build_location,base_->pos)>8) {
                return false;
            }
            return TryBuildStructure(ABILITY_ID::BUILD_SHIELDBATTERY, UNIT_TYPEID::PROTOSS_SHIELDBATTERY, UNIT_TYPEID::PROTOSS_PROBE, build_location);
        }
    }

	void scout_all();

	void scoutenemylocation();

	std::unordered_map<Tag, Tag> observer_nexus_match;
	Tag attackers_observer_tag;
	Tag escort_observer_tag;
	Point2D EscortProbeExpansionPoint;

	uint32_t last_map_renewal;
	std::unordered_map<Tag, Tag> resources_to_nearest_base;
	std::list<const Unit *> enemy_units_scouter_seen;
	std::list<const Unit *> enemy_townhalls_scouter_seen;
	Point2D recent_probe_scout_location;
	uint32_t recent_probe_scout_loop;
	std::list<Point2D> last_dead_probe_pos;
	std::vector<Point2D> scout_candidates;

	Point2D advance_pylon_location;
	Point2D last_destroyed_enemy_structure_pos;

	std::unordered_map<Tag, uint32_t> adept_map;
	Flags flags;

	std::string version;
	std::string botname;
	bool EnemyRush;
	bool ManyEnemyRush;
	bool PhotonRush;
	Point2D pylonlocation;
	std::unordered_set<const Unit*> emergency_killerworkers;

	bool early_strategy;
	bool warpgate_researched;
	bool charge_researched;
	bool BlinkResearched;
	bool ColossusRangeUp;
	bool timing_attack;
	bool Recruited;
	bool turret_exist = false;

	const Unit* advance_pylon;
	const Unit* probe_scout;
	const Unit* pylon_first;
	const Unit* probe_forward;
	const Unit* pylon_cannon;

	bool work_probe_forward;
	bool find_enemy_location;
	std::vector<Point2D>::iterator iter_exp;
	Point3D enemy_expansion;

	uint16_t stage_number;
	const Unit* base;
	uint16_t branch;
	const size_t max_worker_count_ = 68;

	uint16_t num_adept;
	int num_zealot;
	bool TimeToDrop = false;
	uint16_t num_stalker;
	uint16_t num_warpprism;
	uint16_t num_colossus;
	uint16_t num_voidray;
	uint16_t try_colossus;
	uint16_t try_immortal;
	uint16_t num_carrier;
	uint16_t num_expand;

	bool try_initialbalance;
	bool shield3;
	bool tryadeptbranch6;

	Point2D Pylon1, Pylon2, Pylon3, Pylon4, Gate1, Core1, Star1, Batt1, Batt2, Batt3, Batt4, Batt5, Center;
    Point2D next_expansion;

	Race enemyrace;
	std::string opponentID;
	//int ReadStats();
	int ReadStats(std::vector<int>& branches);
	bool WriteStats();
	Race GetEnemyRace();
	float PredictWinrate(int stalker, int immortal, int marine, int marauder, int siegetank, int medivac, int viking, int cyclone, int battlecruiser);

	bool trainstaklerbranch0 = false;
};
