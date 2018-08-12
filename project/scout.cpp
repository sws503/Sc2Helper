#include "memibot.h"

/* note:
pylon 0:25
gateway 0:40
assimilator
gateway 1:00
pylon
cybernetics core 1:40 v
assimilator 2:00 v
2 stalker
warp gate research
2 adept 2:30

determine branch 2:40~3:00

twilight council 2:40~3:00
2 adepts
gateway 3:10 v
pylon v
resonating glave 3:20~30 v
3 adepts

determine branch ~ 4:00

pylon_forward v
warp gate v
nexus expansion 4:10 ~ 20 v
3 adepts 4:48?

determine branch
*/

void MEMIBot::scout_all() {
	const ObservationInterface* observation = Observation();

	manageobserver();

	Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PROBE));
	Units observers_not_sieged = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_OBSERVER));
	//정찰 프로브 재지정
	if (workers.size() > 2 && (probe_scout != nullptr && !probe_scout->is_alive)) {
		for (const auto& p : workers) {
			if (probe_forward != nullptr && p->tag == probe_forward->tag) continue;
			if (IsCarryingMinerals(*p) || IsCarryingVespene(*p)) continue;

			// 본진 정찰 때 사망 -> 다른 정찰 기지로.
			if (!find_enemy_location) {
				flags.set("search_branch", 1);
				flags.set("search_result", 1);

				find_enemy_location = true;
				EnemyBaseLocation = game_info_.enemy_start_locations.front();
				determine_enemy_expansion();

				//todo: 재정찰
				/*
				if (DistanceSquared2D(probe_scout->pos, EnemyBaseLocation) < 400) {
					find_enemy_location = true;
					EnemyBaseLocation = game_info_.enemy_start_locations.front();
					determine_enemy_expansion();
				}
				else {
					std::random_shuffle(game_info_.enemy_start_locations.begin(),
						game_info_.enemy_start_locations.end());
				}
				*/
			}
			// 멀티 정찰 때 사망
			else {
				// 다음 확장으로 스킵
				find_next_scout_location();
			}

			probe_scout = p;

			// 본진 정찰 중단.
			flags.set("search_branch", 1);
			break;
		}
	}

	// 옵저버 3마리 이상이면 프로브 대신 옵저버가 정찰
	if (observers_not_sieged.size() >= 3) {
		if (probe_scout == nullptr || probe_scout->unit_type != UNIT_TYPEID::PROTOSS_OBSERVER) {
			for (const auto& observer : observers_not_sieged) {
				if (observer->tag != attacker_s_observer_tag) {
					probe_scout = observer;
				}
			}
		}
	}
	else {
		if (probe_scout == nullptr || probe_scout->unit_type != UNIT_TYPEID::PROTOSS_PROBE) {
			for (const auto& p : workers) {
				if (probe_forward != nullptr && p->tag == probe_forward->tag) continue;
				if (IsCarryingMinerals(*p) || IsCarryingVespene(*p)) continue;
				probe_scout = p;
			}
		}
	}

	if (probe_scout != nullptr && probe_scout->is_alive && DistanceSquared2D(recent_probe_scout_location, probe_scout->pos) > 4) {
		//Print("renew position");
		recent_probe_scout_location = probe_scout->pos;
		recent_probe_scout_loop = observation->GetGameLoop();
	}

	if (!find_enemy_location) {
		scoutenemylocation();
	}

	if (find_enemy_location && advance_pylon_location == Point2D((float)game_info_.width / 2, (float)game_info_.height / 2)) {
		advance_pylon_location = (startLocation_ * 1.8f + EnemyBaseLocation * 2.2f) / 4.0f;
	}

	// 정찰 : 분기 1, 2 정찰 시작
	if (flags.status("search_branch") == 0 && find_enemy_location) {
		SmartMove(probe_scout, EnemyBaseLocation);

		// trace probe_scout
		if (DistanceSquared2D(recent_probe_scout_location, probe_scout->pos) > 4 || recent_probe_scout_location == Point2D(0, 0)) {
			recent_probe_scout_location = probe_scout->pos;
			recent_probe_scout_loop = observation->GetGameLoop();
		}

		int numExpansion = 0;
		bool HasBarracksOrGateway = false;
		bool IsExtractor = false;
		for (const auto& e : enemy_units_scouter_seen) {
			Print(UnitTypeToName(e->unit_type.ToType()));
			numExpansion += IsTownHall()(*e);
			HasBarracksOrGateway |= IsUnits({
				UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_BARRACKSFLYING,
				UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_WARPGATE
				})(*e);
			IsExtractor |= IsUnit( UNIT_TYPEID::ZERG_EXTRACTOR )(*e);
		}
		// zerg has gas : branch 1
		if (IsExtractor) {
			flags.set("search_branch", 1);
			flags.set("search_result", 3);
		}

		// not blocked and has expansion : branch 0
		if (numExpansion >= 2) {
			flags.set("search_branch", 1);
			flags.set("search_result", 2);
		}

		// not blocked and has no expansion or barracks : branch 1
		if ( Distance2D(probe_scout->pos, EnemyBaseLocation) < 5 ){
			// not blocked and has no expansion or barracks :
			if (HasBarracksOrGateway) {
				flags.set("search_branch", 1);
				flags.set("search_result", 2);
			}
			else {
				flags.set("search_branch", 1);  // search end
				flags.set("search_result", 3);
			}
		}

		// if probe_scout is stuck or cannot go for 10 seconds
		if (observation->GetGameLoop() - recent_probe_scout_loop > 105 &&
			DistanceSquared2D(probe_scout->pos, startLocation_) > 200) {
			flags.set("search_branch", 1);	// search end
			flags.set("search_result", 1);
			Print("Base is blocked!");
		}
	}

	// todo : 브랜치에 따라 정찰 유무 결정.
	// 몰래멀티 주기적으로 검사
	if (flags.status("search_branch") != 0 && find_enemy_location == true &&
		observation->GetUnits(Unit::Alliance::Self, IsUnits({ UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_WARPGATE })).size() > 0
		&& (observation->GetFoodUsed() >= 15)) {
		scoutprobe();
	}

}

void MEMIBot::scoutenemylocation() {
	const ObservationInterface* observation = Observation();
	Units enemy_structures = observation->GetUnits(Unit::Alliance::Enemy, IsStructure(observation));
	Units enemy_townhalls = observation->GetUnits(Unit::Alliance::Enemy, IsTownHall());
	Units pylons = observation->GetUnits(IsUnit(UNIT_TYPEID::PROTOSS_PYLON));
    size_t forge_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_FORGE);
	size_t gateway_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_GATEWAY);

	if (forge_count + gateway_count>0 && probe_scout != nullptr && pylons.size()>0) {
		SmartMove(probe_scout, game_info_.enemy_start_locations.front());
		if (!enemy_townhalls.empty() || enemy_structures.size()>2 || game_info_.enemy_start_locations.size() == 1) {
			if (Distance2D(probe_scout->pos, game_info_.enemy_start_locations.front())<10 || game_info_.enemy_start_locations.size() == 1) {
				find_enemy_location = true;
				Print("find!");
				Actions()->UnitCommand(probe_scout, ABILITY_ID::STOP);
				EnemyBaseLocation = game_info_.enemy_start_locations.front();
				determine_enemy_expansion();
				return;
			}
		}
		else if (Distance2D(probe_scout->pos, game_info_.enemy_start_locations.front())<7) {
			std::vector<Point2D>::iterator iter_esl = game_info_.enemy_start_locations.begin();
			game_info_.enemy_start_locations.erase(iter_esl);
			return;
		}

		if (observation->GetGameLoop() - recent_probe_scout_loop > 105 &&
			DistanceSquared2D(probe_scout->pos, startLocation_) > 200) {
			flags.set("search_branch", 1);	// search end
			flags.set("search_result", 1);
			Print("Base is blocked!");
			find_enemy_location = true;
			Print("find!");
			Actions()->UnitCommand(probe_scout, ABILITY_ID::STOP);
			EnemyBaseLocation = game_info_.enemy_start_locations.front();
			determine_enemy_expansion();
			return;
		}
	}
}

// todo: 정찰 위치 주변에 건물이 있으면 거르기
void MEMIBot::scoutprobe() {
	const ObservationInterface* observation = Observation();
	if (probe_scout == nullptr) return;

	// 브랜치 6에서 거르기
	if (branch == 6) {
		if (stage_number < 620) {
			if (probe_scout != probe_forward) {
				SmartMove(probe_scout, startLocation_);
			}
			return;
		}
	}
	if (!determine_scout_location(probe_scout)) {
		if (probe_scout->unit_type == UNIT_TYPEID::PROTOSS_PROBE) {
			MineIdleWorkers(probe_scout);
			return;
		}
	}
	const Unit* mineralp = FindNearestMineralPatch(*iter_exp);
	const Unit* structurep = FindNearestUnit(*iter_exp, IsStructure(observation), 15);
	if (mineralp == nullptr) {
		return;
	}
	Point2D tag_pos = mineralp->pos;

	// 우리본진이랑 적본진은 거름
	if (DistanceSquared2D(EnemyBaseLocation, tag_pos)<200 ||
		DistanceSquared2D(enemy_expansion, tag_pos)<200 ||
		DistanceSquared2D(front_expansion, tag_pos)<200 ||
		DistanceSquared2D(startLocation_, tag_pos)<200) {
		find_next_scout_location();
		return;
	}
	SmartMove(probe_scout, tag_pos);
	// 도착하면 다음 확장으로
	if (DistanceSquared2D(probe_scout->pos, tag_pos)<4 || Query()->PathingDistance(probe_scout->pos, *iter_exp)<0.1f) {
		find_next_scout_location();
		return;
	}
}

bool MEMIBot::determine_scout_location(const Unit* u) {
	if (u == nullptr) return false;

	
	Filter f_structure = [this](const Unit& u) {
		return (u.alliance == Unit::Alliance::Self || u.alliance == Unit::Alliance::Enemy) && IsStructure(Observation())(u);
	};

	if (iter_exp == scout_candidates.end()) {
		size_t expansion_size;
		std::vector<QueryInterface::PathingQuery> query_vector;
		for (const auto& pos : expansions_) {
			
			QueryInterface::PathingQuery query;
			query.start_unit_tag_ = u->tag;
			query.start_ = u->pos;
			query.end_ = pos;
			query_vector.push_back(query);
		}
		std::vector<float> results = Query()->PathingDistance(query_vector);
		expansion_size = query_vector.size();
		scout_candidates.clear();
		scout_candidates.reserve(expansion_size);

		for (int i = 0; i < expansion_size; i++) {

			Point2D pos = query_vector.at(i).end_;
			float d = results.at(i);
			if (FindNearestUnit(pos, f_structure, 15) != nullptr) {
				continue;
			}
			if (DistanceSquared2D(EnemyBaseLocation, pos)<200 ||
				DistanceSquared2D(enemy_expansion, pos)<200 ||
				DistanceSquared2D(front_expansion, pos)<200 ||
				DistanceSquared2D(startLocation_, pos)<200) {
				continue;
			}
			if (d < 5.0f) {
				continue;
			}
			scout_candidates.push_back(pos);
		}

		std::random_shuffle(scout_candidates.begin(), scout_candidates.end());
		iter_exp = scout_candidates.begin();
	}
	return iter_exp != scout_candidates.end();
}

void MEMIBot::find_next_scout_location() {
	if (determine_scout_location(probe_scout)) {
		iter_exp++;
	}
}

void MEMIBot::determine_enemy_expansion() {
	if (!find_enemy_location) return;
	float minimum_distance = std::numeric_limits<float>::max();
	for (const auto& expansion : expansions_) {
		float current_distance = Distance2D(EnemyBaseLocation, expansion);
		if (current_distance < 3) {
			continue;
		}

		if (current_distance < minimum_distance) {
			enemy_expansion = expansion;
			minimum_distance = current_distance;
		}
	}
}

void MEMIBot::manageobserver() {
	const ObservationInterface* observation = Observation();
	// UNIT_TYPEID(1911); surveilance mode
	// ABILITY_ID(3739); observer mode
	// ABILITY_ID(3741); surveillance mode
	Units observers = observation->GetUnits(Unit::Alliance::Self,
		IsUnits({ UNIT_TYPEID::PROTOSS_OBSERVER, UNIT_TYPEID(1911) }));
	Units observers_not_sieged = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_OBSERVER));
	Units observers_sieged = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID(1911)));
	Units armies = observation->GetUnits(Unit::Alliance::Self, IsArmy(observation));
	Units enemyarmies = observation->GetUnits(Unit::Alliance::Enemy, IsArmy(observation));
	Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
	Units enemystructures = observation->GetUnits(Unit::Alliance::Enemy, IsStructure(observation));
	size_t observers_size = observers.size();
	size_t bases_size = bases.size();

	Filter filter_hidden = [](const Unit& u) {
		return u.display_type == Unit::CloakState::CloakedDetected || u.is_burrowed;
	};

	// attacker_s_probe_tag 지정
	if (attacker_s_observer_tag == NullTag) {
		for (const auto& o : observers_not_sieged) {
			if (probe_scout != nullptr && probe_scout->tag == o->tag) continue;
			attacker_s_observer_tag = o->tag;
			break;
		}
	}

	const Unit* attacker_s_observer = observation->GetUnit(attacker_s_observer_tag);
	float safedistance = 3.0f;

	// 판단
	bool do_siege_mode = false;
	bool dead_probe_cleared = false;
	bool observer_is_going = false;
	bool attackers_probe_is_doing = false;

	if (!last_dead_probe_pos.empty()) {
		Point2D pos = last_dead_probe_pos.front();

		//exclude scouter probe
		bool nearbase = false;
		bool nearenemystructure = false;
		for (const auto& b : bases) {
			nearbase |= (DistanceSquared2D(b->pos, pos) < 200);
			if (nearbase) break;
		}
		for (const auto& es : enemystructures) {
			nearenemystructure |= (DistanceSquared2D(es->pos, pos) < 200);
			if (nearenemystructure) break;
		}
		// 본진에 가깝다: 항상 보내기
		// 본진과의 거리가 30보다 작고 적 건물이 없다: 보내기

		if (nearbase || (Distance2D(startLocation_, pos) < 30 && nearenemystructure) ) {
			dead_probe_cleared = false;
		}
		else {
			dead_probe_cleared = true;
		}
		if (!dead_probe_cleared) {
			for (const auto& observer : observers_not_sieged) {
				float dist_sq = DistanceSquared2D(observer->pos, pos);
				// 죽은 곳으로 갔는데 없다.
				if (dist_sq < 1) {
					Units cloakeddetected_units = FindUnitsNear(observer, 15, Unit::Alliance::Enemy, filter_hidden);
					if (cloakeddetected_units.empty()) {
						dead_probe_cleared = true;
						break;
					}
				}
				// 옵저버가 죽은 곳으로 가고 있다.
				if (observer->orders.empty()) continue;
				observer_is_going |= DistanceSquared2D(pos, observer->orders.front().target_pos) < 9;
			}
		}
		if (dead_probe_cleared) {
			last_dead_probe_pos.pop_front();
		}
	}

	if (flags.status("observer_on_nexus") == 0) {
		if (observers_size > 1) {
			flags.set("observer_on_nexus", 2);
		}
	}

	// 감시모드 셋
	if (flags.status("observer_on_nexus") == 1) {
		if (CountUnitType(UNIT_TYPEID::PROTOSS_FORGE) > 0) {
			flags.set("observer_on_nexus", 2);
		}
		else {
			// 멀티가 있고 옵저버가 2개 이상이면
			if (observers_size >= bases_size && observers_size >= 2) {
				do_siege_mode = true;
			}
		}
	}

	// 옵저버 한마리 attackers와 함께 다니기
	if (flags.status("observer_on_nexus") == 2) {
		Point2D avg_pos(0, 0);
		if (attacker_s_observer != nullptr) {
			if (EvadeEffect(attacker_s_observer)) {}
			else if (CanHitMe(attacker_s_observer, safedistance))
			{
				const Unit * target = GetTarget(attacker_s_observer, enemyarmies);
				FleeKiting(attacker_s_observer, target);
			}
			else if (GetPosition(Attackers, avg_pos)) {
				const Unit* n_u = FindNearestUnit(avg_pos, Attackers);
				Point2D move_pos(avg_pos);
				if (n_u != nullptr) {
					move_pos = n_u->pos;
				}
				SmartMove(attacker_s_observer, move_pos);
				attackers_probe_is_doing = true;
			}
			else if (GetPosition(AttackersRecruiting, avg_pos)) {
				const Unit* n_u = FindNearestUnit(avg_pos, AttackersRecruiting);
				Point2D move_pos(avg_pos);
				if (n_u != nullptr) {
					move_pos = n_u->pos;
				}
				SmartMove(attacker_s_observer, move_pos);
				attackers_probe_is_doing = true;
			}
		}
	}

	// 감시모드
	// 옵저버 넥서스 위로 올려서 sentry mode로 하기
	if (do_siege_mode) {
		// 담아줍니다.
		if (observer_nexus_match.empty()) {
			for (int i = 0; i < bases_size; i++) {
				const Unit* observer = observers[i];
				const Unit* base = bases[i];
				observer_nexus_match.emplace(observer->tag, base->tag);
			}
		}
		for (const auto& o_n : observer_nexus_match) {
			const Unit* observer = observation->GetUnit(o_n.first);
			const Unit* base = observation->GetUnit(o_n.second);
			// 옵저버나 넥서스 터지면 리셋.
			if (observer == nullptr || base == nullptr) {
				observer_nexus_match.clear();
				break;
			}
			// 멀리 있으면 넥서스로 간다.
			if (DistanceSquared2D(observer->pos, base->pos) > 3) {
				// if observer is in surveilance mode
				if (observer->unit_type == UNIT_TYPEID(1911)) {
					// change to observer mode
					Actions()->UnitCommand(observer, ABILITY_ID(3739));
				}
				SmartMove(observer, base->pos);
			}
			// 가까이 있으면 감시모드
			else {
				if (observer->unit_type == UNIT_TYPEID::PROTOSS_OBSERVER) {
					// change to surveilance mode
					Actions()->UnitCommand(observer, ABILITY_ID(3741));
				}
			}
		}
	}
	// 감시모드 풀기
	else {
		if (!observer_nexus_match.empty()) {
			observer_nexus_match.clear();
		}
		for (const auto& observer : observers_sieged) {
			// change to observer mode
			Actions()->UnitCommand(observer, ABILITY_ID(3739));
		}

		// 본진에 프로브가 죽었고 옵저버가 가지 않는 경우 가장 가까운 놈 보내기
		if (!last_dead_probe_pos.empty() && !dead_probe_cleared && !observer_is_going) {
			Point2D pos = last_dead_probe_pos.front();
			Tag attack_obs_tag = attacker_s_observer_tag;
			Filter f = [attack_obs_tag, attackers_probe_is_doing](const Unit& u) {
				return IsUnit(UNIT_TYPEID::PROTOSS_OBSERVER)(u) && (!attackers_probe_is_doing || attack_obs_tag != u.tag);
			};
			const Unit* nearest_observer = FindNearestUnit(pos, Unit::Alliance::Self, f);
			if (nearest_observer) {
				if (EvadeEffect(nearest_observer)) {}
				else if (CanHitMe(nearest_observer, safedistance))
				{
					const Unit * target = GetTarget(nearest_observer, enemyarmies);
					FleeKiting(nearest_observer, target);
				}
				else {
					SmartMove(nearest_observer, pos);
				}
			}
		}
	}

	// 할 일 없으면 돌아다니기
	for (const auto& observer : observers_not_sieged) {
		if (attackers_probe_is_doing && observer->tag == attacker_s_observer_tag) continue;

		// check if observer is near bases
		bool nearbase = false;
		bool nearenemy = false;
		Point2D avg_pos(0, 0);
		for (const auto& base : bases) {
			if (DistanceSquared2D(base->pos, observer->pos) < 200) {
				nearbase = true;
				break;
			}
		}

		Units cloakeddetected_units = FindUnitsNear(observer, 15, Unit::Alliance::Enemy, filter_hidden);
		if (!cloakeddetected_units.empty()) {
			nearenemy = GetPosition(cloakeddetected_units, avg_pos);
		}
		// check if observer found some hidden enemy unit
		if (EvadeEffect(observer)) {}
		else if (CanHitMe(observer, safedistance))
		{
			const Unit * target = GetTarget(observer, enemyarmies);
			FleeKiting(observer, target);
		}
		else if (nearbase && nearenemy) {
			SmartMove(observer, avg_pos);
		}
		else {
			roamobserver(observer);
		}
	}

	return;
}

// 돌아다니기
void MEMIBot::roamobserver(const Unit* observer) {
	if (observer == nullptr) return;
	const ObservationInterface* observation = Observation();
	Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
	if (!observer->orders.empty()) return;
	if (bases.empty()) {
		SmartMove(observer, startLocation_);
		return;
	}
	const Unit * randombase;
	GetRandomUnit(randombase, observation, bases);
	Point2D mp = randombase->pos;

	float rx = GetRandomScalar();
	float ry = GetRandomScalar();
	int roam_radius = 7;
	if (Distance2D(mp, startLocation_) < 5)
	{
		roam_radius = 10;
	}
	Point2D RoamPosition = Point2D(mp.x + rx * roam_radius, mp.y + ry * roam_radius);
	SmartMove(observer, RoamPosition);
}

