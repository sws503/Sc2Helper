#include "memibot.h"

bool MEMIBot::EarlyStrategy() {
	const ObservationInterface* observation = Observation();
	Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PROBE));
	Units bases = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));
	Units pylons = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PYLON));
	Units gateways = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_GATEWAY));
	Units cores = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_CYBERNETICSCORE));
	Units enemy_structures = observation->GetUnits(Unit::Alliance::Enemy, IsStructure(observation));
	Units enemy_townhalls = observation->GetUnits(Unit::Alliance::Enemy, IsTownHall());


	size_t forge_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_FORGE);
	size_t cannon_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_PHOTONCANNON);
	size_t gateway_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_GATEWAY) + CountUnitType(observation, UNIT_TYPEID::PROTOSS_WARPGATE);
	size_t assimilator_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ASSIMILATOR);
	size_t cybernetics_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
	size_t stargate_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_STARGATE);
    size_t twilight_council_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
#ifdef DEBUG
	std::cout << stage_number << std::endl;
#endif

    if (find_enemy_location) {
        advance_pylon_location = Point2D((startLocation_.x*1.5 + game_info_.enemy_start_locations.front().x*2.5)/4, (startLocation_.y*1.5 + game_info_.enemy_start_locations.front().y*2.5)/4);
    }

    if (stage_number>2) {
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

	if (stage_number>14) {
        TryWarpAdept();
        TryBuildPylonIfNeeded(2);

	}

	if (observation->GetFoodWorkers()<23) {
        TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
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
		if (probe_forward == nullptr || !probe_forward->is_alive) {
			GetRandomUnit(probe_forward, observation, UNIT_TYPEID::PROTOSS_PROBE);
			if (probe_scout == probe_forward) probe_forward = nullptr;
		}//건물 지을 프로브 지정
        if (probe_scout != nullptr && probe_forward != nullptr) {
            stage_number++;
            return false;
        }
		return false;
	case 1:
		if (pylons.size()>0) {
            if (pylons.front()->build_progress == 1.0) {
                stage_number++;
                return false;
            }
		}
		if (observation->GetMinerals()>100) {
            return TryBuildPylon(startLocation_,20.0);
		}
		return false;
	case 2:
		if (gateway_count>0) {
			stage_number++;
			return false;
		}
		if (observation->GetMinerals()>150) {
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_PROBE);
		}
		return false;
	case 3:
		if (assimilator_count>1) {
			stage_number++;
			return false;
		}
		if (observation->GetMinerals()>75) {
			return TryBuildGas(base->pos);
		}
		return false;
	case 4:
		if (gateway_count>1) {
			stage_number++;
			return false;
		}
		if (observation->GetMinerals()>150) {
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_PROBE);
		}
		return false;
    case 5:
		if (cybernetics_count>0) {
			stage_number++;
			return false;
		}
		if (observation->GetMinerals()>150) {
			return TryBuildStructureNearPylon(ABILITY_ID::BUILD_CYBERNETICSCORE, UNIT_TYPEID::PROTOSS_PROBE);
		}
		return false;
    case 6:
        if (pylons.size()>1) {
            stage_number++;
            return false;
        }
        if (observation->GetFoodUsed()>20) {
            return TryBuildPylon(startLocation_,20.0);
        }
        return false;
    case 7:
        if (cores.front()->build_progress == 1.0) {
            stage_number++;
            return false;
        }
        return false;
    case 8:
        if (TryBuildUnit(ABILITY_ID::RESEARCH_WARPGATE, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE)) {
            stage_number++;
            return false;
        }
    case 9:
        for (const auto& gate : gateways) {
            if (gate->orders.empty()){
                return TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY);
            }
        }
        stage_number++;
        return false;
    case 10:
        if (twilight_council_count>0) {
            stage_number++;
            return false;
        }
        if(observation->GetMinerals()>=150&&observation->GetVespene()>=100){
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_TWILIGHTCOUNCIL, UNIT_TYPEID::PROTOSS_PROBE);
        }
        return false;
	case 11:
	    Actions()->UnitCommand(probe_forward, ABILITY_ID::MOVE, advance_pylon_location);
		if (gateway_count>3) {
			stage_number++;
			return false;
		}
		if (observation->GetMinerals()>150) {
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_PROBE);
		}
		return false;
	case 12:
        /*if (TryBuildUnit(ABILITY_ID::RESEARCH_ADEPTRESONATINGGLAIVES, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL)) {
            stage_number++;
            return false;
        }*/
        stage_number++;
    case 13:
        for (const auto& gate : gateways) {
            if (gate->build_progress < 1.0) {
                continue;
            }
            if (gate->orders.empty()){
                return TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY);
            }
        }
        stage_number++;
        return false;
    case 14:
        if (advance_pylon != nullptr) {
            stage_number++;
            return false;
        }
        for (const auto& p : pylons) {
            if (Distance2D(p->pos,advance_pylon_location)<20) {
                advance_pylon = p;
            }
        }
        if (observation->GetMinerals()>100) {
            TryBuildPylon(advance_pylon_location, 10.0);
        }
        return false;
    case 15:
        break;



	}


	return false;
}
