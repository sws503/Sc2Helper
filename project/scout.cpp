#include "memibot.h"

void MEMIBot::scout_all() {
	const ObservationInterface* observation = Observation();

	if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_GATEWAY)>0 && iter_exp < expansions_.end() && find_enemy_location == true) {
		scoutprobe();
	}

	scoutenemylocation();

	Point2D position;
	enemy_expansion;

	if (probe_scout != nullptr) {
		//Actions()->UnitCommand(probe_scout, ABILITY_ID::SMART, position);
	}
}

void MEMIBot::scoutenemylocation() {
	const ObservationInterface* observation = Observation();
	Units enemy_structures = observation->GetUnits(Unit::Alliance::Enemy, IsStructure(observation));
	Units enemy_townhalls = observation->GetUnits(Unit::Alliance::Enemy, IsTownHall());
	Units pylons = observation->GetUnits(IsUnit(UNIT_TYPEID::PROTOSS_PYLON));
    size_t forge_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_FORGE);
	size_t gateway_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_GATEWAY);

	if (find_enemy_location) {
		advance_pylon_location = Point2D((startLocation_.x*1.8 + game_info_.enemy_start_locations.front().x*2.2) / 4, (startLocation_.y*1.8 + game_info_.enemy_start_locations.front().y*2.2) / 4);
	}

	if (forge_count + gateway_count>0) {
		if (probe_scout != nullptr && find_enemy_location == false && pylons.size()>0) {
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
					std::vector<Point2D>::iterator iter_esl = game_info_.enemy_start_locations.begin();
					game_info_.enemy_start_locations.erase(iter_esl);
				}
			}
		}
	}
}

void MEMIBot::scoutprobe() {
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

