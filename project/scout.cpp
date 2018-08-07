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

	Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PROBE));

	//정찰 프로브 재지정
	if (workers.size() > 2 && (probe_scout != nullptr && !probe_scout->is_alive)) {
		for (const auto& p : workers) {
			if (probe_forward != nullptr && p->tag == probe_forward->tag) continue;
			if (IsCarryingMinerals(*p) || IsCarryingVespene(*p)) continue;

			// 본진 정찰 때 사망 -> 다른 정찰 기지로.
			if (!find_enemy_location) {
				flags.set("search_branch", 1);
				flags.set("search_result", 1);

				if (DistanceSquared2D(probe_scout->pos, EnemyBaseLocation) < 400) {
					find_enemy_location = true;
					EnemyBaseLocation = game_info_.enemy_start_locations.front();
					determine_enemy_expansion();
				}
				else {
					std::random_shuffle(game_info_.enemy_start_locations.begin(),
						game_info_.enemy_start_locations.end());
				}
			}
			// 멀티 정찰 때 사망
			else {
				// 다음 확장으로 스킵
				if (iter_exp == expansions_.end()) {
					std::random_shuffle(expansions_.begin(), expansions_.end());
					iter_exp = expansions_.begin();
				}
				iter_exp++;
			}

			probe_scout = p;

			// 본진 정찰 중단.
			flags.set("search_branch", 1);
			break;
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
		advance_pylon_location = (startLocation_ * 1.8 + EnemyBaseLocation * 2.2) / 4;
	}

	/*
	if (game_info_.enemy_start_locations.size() == 1 && Enemy_front_expansion == Point3D(0, 0, 0))
	{
		float minimum_distance = std::numeric_limits<float>::max();
		for (const auto& expansion : expansions_) {
			float Enemy_distance = Distance2D(game_info_.enemy_start_locations.front(), expansion);
			if (Enemy_distance < 5.0f) {
				continue;
			}

			if (Enemy_distance < minimum_distance) {
				if (Query()->Placement(ABILITY_ID::BUILD_NEXUS, expansion)) {
					Enemy_front_expansion = expansion;
					minimum_distance = Enemy_distance;
				}
			}
		}
		std::cout << "Enemy front expansion :" << Enemy_front_expansion.x << "  " << Enemy_front_expansion.y << "  " << Enemy_front_expansion.z << std::endl;
	}
	*/

	// 정찰 : 분기 1, 2 정찰 시작
	if (flags.status("search_branch") == 0 && find_enemy_location) {
		SmartMove(probe_scout, EnemyBaseLocation);
		
		// trace probe_scout
		if (DistanceSquared2D(recent_probe_scout_location, probe_scout->pos) > 4 || recent_probe_scout_location == Point2D(0, 0)) {
			recent_probe_scout_location = probe_scout->pos;
			recent_probe_scout_loop = observation->GetGameLoop();
		}

		/*
		Units enemy_units_scouter = FindUnitsNear(probe_scout, 8, Unit::Alliance::Enemy, IsStructure(observation));
		std::list<const Unit*> temp;
		bool nalbil = false;
		for (const auto& e : enemy_units_scouter) {
			// buildings that are not in base
			if (Distance2D(e->pos, EnemyBaseLocation) > 50) {
				// front build
				if (IsUnits({ UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_BARRACKSFLYING,
					UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_WARPGATE})(*e)) {
					nalbil = true;
					break;
				}
				// skip??
				else {
					continue;
				}
			}
			bool exist = false;
			for (auto& l : enemy_units_scouter_seen) {
				if (exist = (l->tag == e->tag)) break;
			}
			if (exist) continue;
			temp.push_back(e);
		}
		for (const auto& e : temp) {
			enemy_units_scouter_seen.push_back(e);
		} */

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
			flags.set("search_branch", 1);  // search end
			flags.set("search_result", 3);
		}

		// if probe_scout is stuck or cannot go for 10 seconds
		if (observation->GetGameLoop() - recent_probe_scout_loop > 105 &&
			DistanceSquared2D(probe_scout->pos, startLocation_) > 200) {
			flags.set("search_branch", 1);	// search end
			flags.set("search_result", 1);
			Print("Base is blocked!");
		}
	}

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

void MEMIBot::scoutprobe() {
	const ObservationInterface* observation = Observation();

	if (iter_exp == expansions_.end()) {
		std::random_shuffle(expansions_.begin(), expansions_.end());
		iter_exp = expansions_.begin();
	}

	const Unit* mineralp = FindNearestMineralPatch(*iter_exp);
	if (mineralp == nullptr) {
		return;
	}
	Point2D tag_pos = mineralp->pos;

	// 우리본진이랑 적본진은 거름
	if (DistanceSquared2D(EnemyBaseLocation, tag_pos)<200 ||
		DistanceSquared2D(enemy_expansion, tag_pos)<200 ||
		DistanceSquared2D(front_expansion, tag_pos)<200 ||
		DistanceSquared2D(startLocation_, tag_pos)<200) {
		iter_exp++;
		return;
	}
	SmartMove(probe_scout, tag_pos);
	// 도착하면 다음 확장으로
	if (DistanceSquared2D(probe_scout->pos, tag_pos)<4) {
		iter_exp++;
		return;
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
