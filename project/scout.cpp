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

	// Todo: 프로브 상납 방지
	while (workers.size() > 2 && (probe_scout != nullptr && !probe_scout->is_alive)) {
		//정찰 프로브 지정
		const Unit* probe_candidate;
		GetRandomUnit(probe_candidate, observation, UNIT_TYPEID::PROTOSS_PROBE);
		if (probe_forward == nullptr || probe_candidate->tag != probe_forward->tag)  probe_scout = probe_candidate;
		flags.set("search_branch", 1);
	}

	if (!find_enemy_location) {
		scoutenemylocation();
	}

	if (find_enemy_location && advance_pylon_location == Point2D((float)game_info_.width / 2, (float)game_info_.height / 2)) {
		advance_pylon_location = Point2D((startLocation_.x*1.8 + game_info_.enemy_start_locations.front().x*2.2) / 4, (startLocation_.y*1.8 + game_info_.enemy_start_locations.front().y*2.2) / 4);
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

	std::cout << flags.status("search_branch") << std::endl;
	if (probe_scout != nullptr && probe_scout->is_alive && DistanceSquared2D(recent_probe_scout_location, probe_scout->pos) > 4) {
		recent_probe_scout_location = probe_scout->pos;
		recent_probe_scout_loop = observation->GetGameLoop();
		Print("renew position");
	}

	// 정찰 : 분기 1, 2 정찰 시작
	if (flags.status("search_branch") == 0 && find_enemy_location) {
		if (probe_scout != nullptr && (probe_scout->orders.empty() || EnemyBaseLocation == probe_scout->orders.front().target_pos))
			Actions()->UnitCommand(probe_scout, ABILITY_ID::MOVE, EnemyBaseLocation);
		
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
			numExpansion += IsTownHall()(*e);
			HasBarracksOrGateway |= IsUnits({ 
				UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_BARRACKSFLYING, 
				UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_WARPGATE
				})(*e);
			IsExtractor |= IsUnit( UNIT_TYPEID::ZERG_EXTRACTOR )(*e);
		}
		if (IsExtractor) {
			flags.set("search_branch", 1);	// search end
			branch = 1;
		}

		if ( Distance2D(probe_scout->pos, EnemyLocation) < 7 && ((numExpansion < 2) || !HasBarracksOrGateway) ) {
			flags.set("search_branch", 1);  // search end
			branch = 1;
		}

		// if probe_scout is stuck or cannot go for 10 seconds
		if (observation->GetGameLoop() - recent_probe_scout_loop > 217) {
			if (Query()->PathingDistance(probe_scout, enemy_expansion + Point2D(4, 4)) < 0.1) {
				flags.set("search_status", 1);	// search end
				branch = 2;
				Print("Base is blocked!");
			}
		}
	}
	else if (observation->GetUnits(Unit::Alliance::Self, IsUnits({ UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_WARPGATE })).size() > 0 
		&& (observation->GetFoodUsed() >= 15) && find_enemy_location == true) {
		scoutprobe();
	}

	if (probe_scout != nullptr) {
		//Actions()->UnitCommand(probe_scout, ABILITY_ID::SMART, position);
	}
}



void MEMIBot::scoutenemylocation() {
	const ObservationInterface* observation = Observation();
	Units enemy_structures = observation->GetUnits(Unit::Alliance::Enemy, IsStructure(observation));
	Units enemy_townhalls = observation->GetUnits(Unit::Alliance::Enemy, IsTownHall());
	Units pylons = observation->GetUnits(IsUnit(UNIT_TYPEID::PROTOSS_PYLON));

	if (stage_number>2 && probe_scout != nullptr && pylons.size()>0) {
		if (probe_scout->orders.empty() || (game_info_.enemy_start_locations.front()) != probe_scout->orders.front().target_pos)
			Actions()->UnitCommand(probe_scout, ABILITY_ID::MOVE, game_info_.enemy_start_locations.front());
		if (!enemy_townhalls.empty() || enemy_structures.size()>2 || game_info_.enemy_start_locations.size() == 1) {
			if (Distance2D(probe_scout->pos, game_info_.enemy_start_locations.front())<10 || game_info_.enemy_start_locations.size() == 1) {
				find_enemy_location = true;
				Print("find!");
				Actions()->UnitCommand(probe_scout, ABILITY_ID::STOP);
				EnemyBaseLocation = game_info_.enemy_start_locations.front();
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
		}
		else {
			if (Distance2D(probe_scout->pos, game_info_.enemy_start_locations.front())<7) {
				std::vector<Point2D>::iterator iter_esl = game_info_.enemy_start_locations.begin();
				game_info_.enemy_start_locations.erase(iter_esl);
			}
		}
	}
}

void MEMIBot::scoutprobe() {
	const ObservationInterface* observation = Observation();

	if (iter_exp == expansions_.end()) iter_exp = expansions_.begin();

	const Unit* mineralp = FindNearestMineralPatch(*iter_exp);
	if (mineralp == nullptr) {
		return;
	}
	Point2D tag_pos = mineralp->pos;

	if (DistanceSquared2D(EnemyBaseLocation, tag_pos)<200 ||
		DistanceSquared2D(enemy_expansion, tag_pos)<200 ||
		DistanceSquared2D(front_expansion, tag_pos)<200 ||
		DistanceSquared2D(startLocation_, tag_pos)<200) {
		iter_exp++;
		return;
	}
	if (probe_scout->orders.empty() || tag_pos != probe_scout->orders.front().target_pos)
		Actions()->UnitCommand(probe_scout, ABILITY_ID::MOVE, tag_pos);
	if (DistanceSquared2D(probe_scout->pos, tag_pos)<4) {
		iter_exp++;
	}

}

