#include "memibot.h"

bool MEMIBot::EarlyStrategy() {
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


	std::cout << stage_number << std::endl;
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

	switch (stage_number) {
    case 3:
        if(observation->GetFoodUsed()<18) {
            TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
        }
	case 4:
		break;
	case 5:
		break;
    case 6 :
        break;
    case 7 :
        break;
	default:
		if (observation->GetFoodWorkers()<25 && observation->GetFoodCap() - observation->GetFoodUsed() >= 1) {
			TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
			//TryBuildUnitChrono(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
		}
		else {
			if (GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR) > observation->GetFoodWorkers() && observation->GetFoodWorkers() < max_worker_count_) {
				TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
			}
		}
		if (bases.size() > 3) {
			TryBuildProbe();
		}

		if (stage_number>18) {
			TryBuildPylonIfNeeded(CountUnitType(observation, UNIT_TYPEID::PROTOSS_STARGATE));
		}

		if (stage_number>18) {
			if (observation->GetMinerals() > 350 && observation->GetVespene() > 250 && observation->GetFoodCap() - observation->GetFoodUsed() >= 6)
				TryBuildUnit(ABILITY_ID::TRAIN_CARRIER, UNIT_TYPEID::PROTOSS_STARGATE);
		}

		if (stage_number>25) {
			//TryBuildExpansionNexus();
			TryBuildAssimilator();
			if (stargate_count<bases.size()) {
				TryBuildStructureNearPylon(ABILITY_ID::BUILD_STARGATE, UNIT_TYPEID::PROTOSS_PROBE);
			}
			int CurrentStargate = CountUnitType(observation, UNIT_TYPEID::PROTOSS_STARGATE);

		}
		// 파일런 다시 짓기
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
		if (observation->GetMinerals()>50) {
			Actions()->UnitCommand(probe_scout, ABILITY_ID::MOVE, probe_scout_dest);
			stage_number++;
		}
		return false;
	case 1:
		if (pylons.size()>0) {
			stage_number++;
			return false;
		}
		if (Distance2D(probe_scout->pos, front_expansion)<3) {
			probe_scout_dest = Point2D((float)game_info_.width / 2, (float)game_info_.height / 2);
			Actions()->UnitCommand(probe_scout, ABILITY_ID::MOVE, probe_scout_dest);
		}
		if (Distance2D(probe_scout_dest,front_expansion)>5) {
            if (Distance2D(probe_scout->pos,front_expansion) > 20) {
                Actions()->UnitCommand(probe_scout, ABILITY_ID::MOVE, front_expansion);
            }
            else if (Distance2D(probe_scout->pos, front_expansion) > 7 && Distance2D(probe_scout->pos, startLocation_) > 20) {
				if (!probe_scout->orders.empty() && probe_scout->orders.front().ability_id != ABILITY_ID::BUILD_PYLON) {
					Actions()->UnitCommand(probe_scout, ABILITY_ID::STOP);
				}
				TryBuildPylon(probe_scout->pos);
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
		if (pylon_first != nullptr && observation->GetMinerals()>100 && observation->GetMinerals()<150) {
			Actions()->UnitCommand(probe_forge, ABILITY_ID::MOVE, pylon_first->pos);
		}
		if (probe_forge->orders.empty()) {
			if (TryBuildForge(probe_forge, pylon_first)) {
                return false;
			}
		}
		return false;
	case 3:
		if (bases.size()>1) {
			stage_number++;
			return false;
		}
		if (observation->GetFoodUsed()<17) {
			TryChronoboost(base);
        }

		if (observation->GetMinerals() >= 400) {
			Actions()->UnitCommand(probe_forge, ABILITY_ID::BUILD_NEXUS, front_expansion);
		}
		if (observation->GetMinerals()<400 && observation->GetMinerals()>300) {
			Actions()->UnitCommand(probe_forge, ABILITY_ID::MOVE, front_expansion);
		}
		return false;
	case 4:
		if (cannon_count>1) {
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
		if (assimilator_count>0) {
			stage_number++;
			return false;
		}
		if (observation->GetMinerals()>75) {
			TryBuildGas(base->pos);
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
		if (cannon_count>3) {
			stage_number++;
			return false;
		}
		if (observation->GetMinerals()>150) {
			TryBuildStructureNearPylonWithUnit(probe_forge, ABILITY_ID::BUILD_PHOTONCANNON, pylon_first);
		}
		return false;
    case 9:
		if (cybernetics_count>0) {
			stage_number++;
			return false;
		}
		if (observation->GetMinerals()>150) {
			TryBuildStructureNearPylon(ABILITY_ID::BUILD_CYBERNETICSCORE, UNIT_TYPEID::PROTOSS_PROBE);
		}
		return false;
    case 10:
		if (assimilator_count>1) {
			stage_number++;
			return false;
		}
		for (const auto& b : bases) {
            if (b!=base){
				TryChronoboost(b);
            }
		}

		if (observation->GetMinerals()>75) {
			TryBuildGas(base->pos);
		}
		return false;
	case 11:
		if (stargate_count>0) {
			stage_number++;
			return false;
		}
		if (observation->GetMinerals()>150 && observation->GetVespene()>150) {
			TryBuildStructureNearPylon(ABILITY_ID::BUILD_STARGATE, UNIT_TYPEID::PROTOSS_PROBE);
		}
		return false;
    case 12:
		if (cannon_count>5) {
			stage_number++;
			return false;
		}
		if (observation->GetMinerals()>150) {
			TryBuildStructureNearPylonWithUnit(probe_forge, ABILITY_ID::BUILD_PHOTONCANNON, pylon_first);
		}
		return false;
    case 13 :
        if (assimilator_count>3) {
            stage_number++;
            return false;
        }
        for (const auto& b : bases) {
            if (b != base) {
                TryBuildGas(b->pos);
            }
        }
        return false;
     case 14:
		 if (observation->GetMinerals() > 150 && observation->GetVespene() > 150 && observation->GetFoodCap() - observation->GetFoodUsed() >= 3) // 안되길래 바꿔놨음
		 {
			 if (TryBuildUnit(ABILITY_ID::TRAIN_ORACLE, UNIT_TYPEID::PROTOSS_STARGATE)) {
				 OracleTrained = true;
				 stage_number++;
			 }
		 }
		return false;
	case 15:
		if (pylons.size()>2) {
			stage_number++;
			return false;
		}
		if (observation->GetMinerals()>100) {
			TryBuildPylon(Point2D((FindNearestMineralPatch(base->pos)->pos.x + base->pos.x) / 2, (FindNearestMineralPatch(base->pos)->pos.y + base->pos.y) / 2));
		}
		return false;
    case 16:
		if (pylons.size()>3) {
			stage_number++;
			return false;
		}
		if (observation->GetMinerals()>100) {
            for (const auto& b : bases) {
                if (b != base) {
                    //TryBuildPylon(Point2D((FindNearestMineralPatch(b->pos)->pos.x + b->pos.x) / 2, (FindNearestMineralPatch(b->pos)->pos.y + b->pos.y) / 2));
                    TryBuildPylon(FindNearestMineralPatch(b->pos)->pos);
                }
            }
		}
		return false;
	case 17:
		if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_FLEETBEACON)>0) {
			stage_number++;
			return false;
		}

		if (observation->GetMinerals()>300 && observation->GetVespene()>200) {
			TryBuildStructureNearPylon(ABILITY_ID::BUILD_FLEETBEACON, UNIT_TYPEID::PROTOSS_PROBE);
		}
		return false;
    case 18:
		if (observation->GetMinerals() > 250 && observation->GetVespene() > 150 && observation->GetFoodCap() - observation->GetFoodUsed() >= 4) // 안되길래 바꿔놨음
		{
			if (TryBuildUnit(ABILITY_ID::TRAIN_VOIDRAY, UNIT_TYPEID::PROTOSS_STARGATE)) {
				stage_number++;
			}
		}
		return false;
	case 19:
		if (stargate_count>1) {
			stage_number++;
			return false;
		}
		if (observation->GetMinerals()>150 && observation->GetVespene()>150) {
			TryBuildStructureNearPylon(ABILITY_ID::BUILD_STARGATE, UNIT_TYPEID::PROTOSS_PROBE);
		}
		return false;
    case 20:
		if (TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRWEAPONS, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE)) {
			stage_number++;
		}
		return false;
    case 21:
        if (CountUnitTypeNearLocation(observation, UNIT_TYPEID::PROTOSS_PHOTONCANNON, base->pos)>0) {
            stage_number++;
            return false;
        }
		for (const auto& pylon : pylons) {
			if (Distance2D(pylon->pos, base->pos)<9) {
				TryBuildStructureNearPylon(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE, pylon);
			}
		}
		return false;
    case 22:
		for (const auto& pylon : pylons) {
			if (Distance2D(pylon->pos, front_expansion)<10) {
				if (TryBuildStructureNearPylon(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE, pylon)) {
					stage_number++;
					return false;
				}
			}
		}
		return false;
	case 23:
		if (observation->GetMinerals()>150) {
			if (TryBuildStructureNearPylon(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE, pylon_first)) {
               stage_number++;
               return false;
			}
		}
		return false;
    case 24:
		if (observation->GetMinerals()>150) {
			if (TryBuildStructureNearPylon(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE, pylon_first)) {
               stage_number++;
               return false;
			}
		}
		return false;
    case 25:
        stage_number++;
        return false;
		/*if (TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRWEAPONS, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE)) {
			stage_number++;
		}
		return false;*/
    case 26:
        if (cannon_count>10) {
            stage_number++;
            return false;
        }
        if (observation->GetMinerals()>700){
            if(TryBuildStructureNearPylon(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE, pylon_first)) {
                return false;
            }
            for (const auto& pylon : pylons) {
                if (Distance2D(pylon->pos,front_expansion)<Distance2D(pylon->pos,base->pos)) {
                    if(TryBuildStructureNearPylon(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE, pylon)){
                        return false;
                    }
                }
            }
        }
        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_CARRIER)>1) {
            TryBuildUnit(ABILITY_ID::RESEARCH_INTERCEPTORGRAVITONCATAPULT, UNIT_TYPEID::PROTOSS_FLEETBEACON);
        }
        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_CARRIER)>3) {
            TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSSHIELDS, UNIT_TYPEID::PROTOSS_FORGE);
        }
        TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRWEAPONS, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
        TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRARMOR, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
    case 27:
        if (bases.size()>2) {
            for (const auto& b : bases) {
                if(b!=base && Distance2D(b->pos,front_expansion)>10) {
                    if(CountUnitTypeNearLocation(observation,UNIT_TYPEID::PROTOSS_PHOTONCANNON,b->pos)>8){
                        stage_number++;
                    }
                }
            }
        }
        if (bases.size()<3) {
            TryExpand(ABILITY_ID::BUILD_NEXUS,UNIT_TYPEID::PROTOSS_PROBE);
            return false;
        }
        if (bases.size()>2) {
            for (const auto& pylon : pylons) {
                if(Distance2D(front_expansion,pylon->pos)>40 && Distance2D(base->pos,pylon->pos)>40) {
                    if (observation->GetMinerals()>800) {
                        if (TryBuildStructureNearPylon(ABILITY_ID::BUILD_PHOTONCANNON,UNIT_TYPEID::PROTOSS_PROBE,pylon)) {
                            return false;
                        }
                    }
                }
            }
            for (const auto& b : bases) {
                if(Distance2D(b->pos,base->pos)<10 || Distance2D(b->pos,front_expansion)<10) {
                    continue;
                }

                if (CountUnitTypeNearLocation(observation,UNIT_TYPEID::PROTOSS_PYLON,b->pos)<3) {
                    TryBuildPylon(b->pos,7);
                }
            }
        }
    case 28:
        if (observation->GetMinerals()>900) {
            for (const auto& b : bases) {
                if(Distance2D(b->pos,base->pos)<10 || Distance2D(b->pos,front_expansion)<10) {
                    continue;
                }
                for (const auto& pylon : pylons) {
                    if (Distance2D(pylon->pos,b->pos)<20 || Distance2D(pylon->pos,front_expansion)) {
                        TryBuildStructureNearPylon(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE, pylon);
                        return false;
                    }
                }
            }
           for (const auto& pylon : pylons) {
                if (Distance2D(pylon->pos,base->pos)>40) {
                    TryBuildStructureNearPylon(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE, pylon);
                    return false;
                }
            }
        }


    /*

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
		return false;*/
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
