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
    Units warpprisms = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_WARPPRISM));
	Units warpprisms_phasing = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_WARPPRISMPHASING));
	Units stalkers = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_STALKER));

	/*
	while (workers.size() > 2 && (probe_forge == nullptr || !probe_forge->is_alive)) {
	//건물 지을 프로브 지정
		const Unit* probe_candidate;
		GetRandomUnit(probe_candidate, observation, UNIT_TYPEID::PROTOSS_PROBE);
		if (probe_scout == nullptr || probe_candidate->tag != probe_scout->tag) probe_forge = probe_candidate;
	}

	// Todo: 프로브 상납 방지
	while (workers.size() > 2 && (probe_scout == nullptr)) {
		//정찰 프로브 지정
		const Unit* probe_candidate;
		GetRandomUnit(probe_candidate, observation, UNIT_TYPEID::PROTOSS_PROBE);
		if (probe_forge == nullptr || probe_candidate->tag != probe_forge->tag)  probe_scout = probe_candidate;
	}
	*/

	size_t forge_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_FORGE);
	size_t cannon_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_PHOTONCANNON);
	size_t gateway_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_GATEWAY) + CountUnitType(observation, UNIT_TYPEID::PROTOSS_WARPGATE);
	size_t assimilator_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ASSIMILATOR);
	size_t cybernetics_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
	size_t stargate_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_STARGATE);
    size_t twilight_council_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
    size_t robotics_facility_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);
#ifdef DEBUG
	std::cout << stage_number << std::endl;
#endif
    const Point2D& enemy = game_info_.enemy_start_locations.front();
    //std::cout<<Query()->PathingDistance(bases.front(),game_info_.enemy_start_locations.front())<<std::endl;
    if (base != nullptr) std::cout<<Query()->PathingDistance(front_expansion,enemy)<<std::endl;

    if (find_enemy_location) {
        advance_pylon_location = Point2D((startLocation_.x*1.8 + game_info_.enemy_start_locations.front().x*2.2)/4, (startLocation_.y*1.8 + game_info_.enemy_start_locations.front().y*2.2)/4);
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
					std::vector<Point2D>::iterator iter_esl = game_info_.enemy_start_locations.begin();
					game_info_.enemy_start_locations.erase(iter_esl);
				}
			}
		}
	}


	if (stage_number<28) {
        if (observation->GetFoodWorkers()<23) {
            TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
        }
	}
	else{
        for (const auto& b : bases) {
            if (b->build_progress < 1.0) {
                if(observation->GetFoodWorkers() < max_worker_count_) {
                    TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
                }
            }
        }

        if (GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR) >= observation->GetFoodWorkers() && observation->GetFoodWorkers() < max_worker_count_) {
            TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
        }
	}
	if (branch<2 && stage_number>29) {
        TryBuildPylonIfNeeded(2);
        if ((bases.size()-1)*2>assimilator_count) {
            TryBuildAssimilator();
        }
        if (forge_count<1) {
            TryBuildStructureNearPylon(ABILITY_ID::BUILD_FORGE, UNIT_TYPEID::PROTOSS_PROBE);
        }
        if (!BlinkResearched && CountUnitType(observation, UNIT_TYPEID::PROTOSS_STALKER)>5) {
            TryBuildUnit(ABILITY_ID::RESEARCH_BLINK, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
        }
        if (gateway_count<bases.size()*3 && gateway_count<10) {
            TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_PROBE);
        }
        else if (GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR) <= observation->GetFoodWorkers() ) {
            for (const auto& b : bases) {
                if (b->build_progress < 1.0) {
                    return TryWarpStalker();
                }
            }
            TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
        }
        else{
            TryWarpStalker();
        }
	}

	if (branch == 2 && stage_number > 109) {
        TryBuildPylonIfNeeded(2);
	}


	Point2D probe_scout_dest;


	switch (stage_number) {
	case 0:
		if (bases.size() > 1) {
			for (const auto& b : bases) {
				if (Distance2D(b->pos, startLocation_) < 3) {
					base = b;
				}
			}
		}
		else if (bases.size() == 1){
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
            return TrybuildFirstPylon();
		}
		return false;
	case 2:
		if (gateway_count>0) {
			stage_number++;
			return false;
		}
		if (observation->GetMinerals()>150) {
		    TryChronoboost(base);
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_PROBE);
		}
		return false;
	case 3:
		if (assimilator_count>=1) {
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
        if (pylons.size()>1) {
            stage_number++;
            return false;
        }
        if (observation->GetMinerals()>100) {
            return TryBuildPylon(startLocation_,20.0);
        }
        return false;
    case 6:
		if (cybernetics_count>0) {
			stage_number++;
			return false;
		}
		if (observation->GetMinerals()>150) {
			return TryBuildStructureNearPylon(ABILITY_ID::BUILD_CYBERNETICSCORE, UNIT_TYPEID::PROTOSS_PROBE);
		}
		return false;
    case 7:
		if (assimilator_count>=2) {
			stage_number++;
			return false;
		}
		if (observation->GetMinerals()>75) {
			return TryBuildGas(base->pos);
		}
		return false;
    case 8:
        if (cores.front()->build_progress == 1.0) {
            stage_number++;
            return false;
        }
        return false;
    case 9:
        for (const auto& gate : gateways) {
            if (gate->build_progress < 1.0) {
                continue;
            }
            if (observation->GetMinerals() < 125 || observation->GetVespene() < 50) {
                return false;
            }
            if (gate->orders.empty()){
                return TryBuildUnitChrono(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY);
            }
        }
        stage_number++;
        return false;
    case 10:
        if (TryBuildUnit(ABILITY_ID::RESEARCH_WARPGATE, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE)) {
            stage_number++;
            return false;
        }
    case 11:
        if (branch == 2) {
            stage_number = 100;
            return false;
        }

        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY)) {
            stage_number++;
            return false;
        }
        return false;
    case 12:
        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY)) {
            stage_number++;
            return false;
        }
        return false;
    case 13:
        if (branch == 0) {
            stage_number = 16;
            return false;
        }
        if (branch == 1) {
            stage_number++;
            return false;
        }
    case 14:
        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_SHIELDBATTERY)>=2) {
            stage_number++;
            return false;
        }
        if (observation->GetMinerals()>100){
            TryBuildStructureNearPylon(ABILITY_ID::BUILD_SHIELDBATTERY, UNIT_TYPEID::PROTOSS_PROBE);
        }
        return false;
    case 15:
        for (const auto& gate : gateways) {
            if (gate->build_progress < 1.0) {
                continue;
            }
            if (observation->GetMinerals() < 125 || observation->GetVespene() < 50) {
                return false;
            }
            if (gate->orders.empty()){
                return TryBuildUnitChrono(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY);
            }
        }
        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_STALKER)<2) {
               return false;
        }
        stage_number++;
        return false;
    case 16:
        if (pylons.size()>2) {
            stage_number++;
            return false;
        }
        if (observation->GetMinerals()>100) {
            return TryBuildPylon(startLocation_,20.0);
        }
        return false;
	case 17:
        if (twilight_council_count>0) {
            stage_number++;
            return false;
        }
        if(observation->GetMinerals()>=150&&observation->GetVespene()>=100){
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_TWILIGHTCOUNCIL, UNIT_TYPEID::PROTOSS_PROBE);
        }
    case 18:
        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY)) {
            stage_number++;
            return false;
        }
        return false;
    case 19:
        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY)) {
            stage_number++;
            return false;
        }
        return false;
    case 20:
		if (gateway_count>2) {
			stage_number++;
			return false;
		}
		if (observation->GetMinerals()>150) {
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_PROBE);
		}
		return false;
    case 21:
        if (pylons.size()>3) {
            stage_number++;
            return false;
        }
        if (observation->GetMinerals()>100) {
            return TryBuildPylon(startLocation_,20.0);
        }
        return false;
    case 22:
        Actions()->UnitCommand(probe_forward, ABILITY_ID::MOVE, advance_pylon_location);
        if (TryBuildUnit(ABILITY_ID::RESEARCH_ADEPTRESONATINGGLAIVES, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL)) {
            stage_number++;
            return false;
        }
    case 23:
        if (observation->GetMinerals() < 100 || observation->GetVespene() < 50) {
            return false;
        }
        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY)) {
            stage_number++;
            return false;
        }
        return false;
    case 24:
        if (observation->GetMinerals() < 100 || observation->GetVespene() < 50) {
            return false;
        }
        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY)) {
            stage_number++;
            return false;
        }
        return false;
    case 25:
        if (observation->GetMinerals() < 100 || observation->GetVespene() < 50) {
            return false;
        }
        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY)) {
            stage_number++;
            return false;
        }
        return false;
    case 26:
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
    case 27:
        if (bases.size()>=2) {
            stage_number++;
            return false;
        }
        if (observation->GetMinerals()>400) {
            TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
        }
        return false;
    case 28:
        if (bases.size()<2) {
            stage_number--;
            return false;
        }
        if (CountUnitTypeNearLocation(UNIT_TYPEID::PROTOSS_PYLON, front_expansion, 8.0)>=1) {
            stage_number++;
            return false;
        }
        if (observation->GetMinerals() > 100) {
            TryBuildPylon(front_expansion, 6.0);
        }
        return false;
    case 29:
        if (CountUnitType(observation,UNIT_TYPEID::PROTOSS_ADEPT)>9) {
            stage_number++;
            return false;
        }
        TryWarpAdept();

    //branch 2
    case 100:
        for (const auto& gate : gateways) {
            if (gate->build_progress < 1.0) {
                continue;
            }
            if (observation->GetMinerals() < 125 || observation->GetVespene() < 50) {
                return false;
            }
            if (gate->orders.empty()){
                return TryBuildUnitChrono(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY);
            }
        }
        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_STALKER)<2) {
            return false;
        }
        stage_number++;
        return false;
    case 101:
        if (pylons.size()>2) {
            stage_number++;
            return false;
        }
        if (observation->GetMinerals()>100) {
            return TryBuildPylon(startLocation_,20.0);
        }
        return false;
    case 102:
        if (robotics_facility_count>=1) {
            stage_number++;
            return false;
        }
        if (observation->GetMinerals() > 200 && observation->GetVespene() > 100) {
            TryBuildStructureNearPylon(ABILITY_ID::BUILD_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_PROBE);
        }
    case 103:
        if (robotics_facility_count<1) {
            stage_number--;
            return false;
        }
		if (gateway_count>3) {
			stage_number++;
			return false;
		}
		if (observation->GetMinerals()>150) {
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_PROBE);
		}
		return false;
    case 104:
        TryChronoboost(cores.front());
        for (const auto& gate : gateways) {
            if (gate->build_progress < 1.0) {
                continue;
            }
            if (!gate->orders.empty()) {
                continue;
            }
            if (observation->GetMinerals() < 125 || observation->GetVespene() < 50) {
                return false;
            }
            return TryBuildUnitChrono(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY);
        }
        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_STALKER)<4) {
            return false;
        }
        stage_number++;
        return false;
    case 105:
        if (CountUnitType(UNIT_TYPEID::PROTOSS_WARPPRISM)>0) {
            stage_number++;
            return false;
        }
        return TryBuildUnitChrono(ABILITY_ID::TRAIN_WARPPRISM, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);
    case 106:
        if (TryBuildUnit(ABILITY_ID::TRAIN_OBSERVER, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY)) {
            stage_number++;
            return false;
        }
        return false;
    case 107:
        if (pylons.size()>3) {
            stage_number++;
            return false;
        }
        if (observation->GetMinerals()>100) {
            return TryBuildPylon(startLocation_,20.0);
        }
        return false;
	case 108:
	    if (CountUnitType(UNIT_TYPEID::PROTOSS_WARPPRISM)<1) {
            stage_number=105;
            return false;
	    }
	    if (warpprisms.front()->cargo_space_taken==warpprisms.front()->cargo_space_max) {
            Actions()->UnitCommand(warpprisms.front(), ABILITY_ID::MOVE, game_info_.enemy_start_locations.front());
            stage_number++;
            return false;
	    }
        Actions()->UnitCommand(warpprisms.front(),ABILITY_ID::LOAD,stalkers.front());
        return false;
	case 109:
	    if (warpprisms.empty()) {
            stage_number++;
            return false;
	    }
	    else{
            std::cout<<Query()->PathingDistance(warpprisms.front()->pos,game_info_.enemy_start_locations.front())<<std::endl;

            //Actions()->UnitCommand(warpprisms.front(), ABILITY_ID::MORPH_WARPPRISMPHASINGMODE);
            return false;
	    }
	case 110:
        TryWarpUnitPrism(ABILITY_ID::TRAINWARP_STALKER);
	}

	return false;
}
