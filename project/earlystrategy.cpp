#include "memibot.h"

// Todo: trybuildunit() 호출하기 전에 자원, 인구수가 있는지 체크하기
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
    Units templars = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_HIGHTEMPLAR));
    Units archons = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_ARCHON));

	/*
	while (workers.size() > 2 && (probe_forward == nullptr || !probe_forward->is_alive)) {
	//건물 지을 프로브 지정
		const Unit* probe_candidate;
		GetRandomUnit(probe_candidate, observation, UNIT_TYPEID::PROTOSS_PROBE);
		if (probe_scout == nullptr || probe_candidate->tag != probe_scout->tag) probe_forward = probe_candidate;
	}

	// Todo: 프로브 상납 방지
	while (workers.size() > 2 && (probe_scout == nullptr)) {
		//정찰 프로브 지정
		const Unit* probe_candidate;
		GetRandomUnit(probe_candidate, observation, UNIT_TYPEID::PROTOSS_PROBE);
		if (probe_forward == nullptr || probe_candidate->tag != probe_forward->tag)  probe_scout = probe_candidate;
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
    size_t robotics_bay_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ROBOTICSBAY);
    size_t templar_archive_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_TEMPLARARCHIVE);

#ifdef DEBUG
	std::cout << stage_number << std::endl;
#endif


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
	if (branch<2 && stage_number>35) {
        TryBuildPylonIfNeeded(2);
        if (bases.size()*2>assimilator_count) {
            TryBuildAssimilator();
        }
        if (forge_count<2) {
            TryBuildStructureNearPylon(ABILITY_ID::BUILD_FORGE, UNIT_TYPEID::PROTOSS_PROBE);
        }
        if (!BlinkResearched && CountUnitType(observation, UNIT_TYPEID::PROTOSS_STALKER)>5) {
			if (TryBuildUnit(ABILITY_ID::RESEARCH_BLINK, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL))
				BlinkResearched = true;
			
        }
        if (gateway_count<=bases.size()*2) {
            TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_PROBE);
        }
        else if (GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR) <= observation->GetFoodWorkers() ) {
            for (const auto& b : bases) {
                if (b->build_progress < 1.0) {
                    TryBuildArmyBranch0();
                }
            }
            TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
        }
        else{
            TryBuildArmyBranch0();
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
            stage_number=1;
            return false;
        }
		return false;
	case 1:
		if (pylons.size()>0) {
            if (pylons.front()->build_progress == 1.0) {
                stage_number=2;
                return false;
            }
		}
		if (observation->GetMinerals()>100) {
            return TrybuildFirstPylon();
		}
		return false;
	case 2:
		if (gateway_count>0) {
			stage_number=3;
			return false;
		}
		if (observation->GetMinerals()>150) {
		    TryChronoboost(base);
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_PROBE);
		}
		return false;
	case 3:
		if (assimilator_count>=1) {
			stage_number=4;
			return false;
		}
		if (observation->GetMinerals()>75) {
			return TryBuildGas(base->pos);
		}
		return false;
	case 4:
		if (gateway_count>1) {
			stage_number=5;
			return false;
		}
		if (observation->GetMinerals()>150) {
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_PROBE);
		}
		return false;
    case 5:
        if (pylons.size()>1) {
            stage_number=6;
            return false;
        }
        if (observation->GetMinerals()>100) {
            return TryBuildPylon(startLocation_,20.0);
        }
        return false;
    case 6:
		if (cybernetics_count>0) {
			stage_number=7;
			return false;
		}
		if (observation->GetMinerals()>150) {
			return TryBuildStructureNearPylon(ABILITY_ID::BUILD_CYBERNETICSCORE, UNIT_TYPEID::PROTOSS_PROBE);
		}
		return false;
    case 7:
		if (assimilator_count>=2) {
			stage_number=8;
			return false;
		}
		if (observation->GetMinerals()>75) {
			return TryBuildGas(base->pos);
		}
		return false;
    case 8:
        if (cores.front()->build_progress == 1.0) {
            stage_number=9;
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
        stage_number=10;
        return false;
    case 10:
        if (TryBuildUnit(ABILITY_ID::RESEARCH_WARPGATE, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE)) {
            stage_number=11;
            return false;
        }
    case 11:
        if (branch == 2) {
            stage_number = 100;
            return false;
        }

        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY)) {
            stage_number=12;
            return false;
        }
        return false;
    case 12:
        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY)) {
            stage_number=13;
            return false;
        }
        return false;
    case 13:
        if (branch == 0 || branch == 3) {
            stage_number = 16;
            return false;
        }
        if (branch == 1) {
            stage_number=14;
            return false;
        }
    case 14:
        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_SHIELDBATTERY)>=2) {
            stage_number=15;
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
                //return TryBuildUnit(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_STALKER);
                return TryBuildUnitChrono(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY);
            }
        }
        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_STALKER)<2) {
               return false;
        }
        stage_number=16;
        return false;
    case 16:
        if (pylons.size()>2) {
            stage_number=17;
            return false;
        }
        if (observation->GetMinerals()>100) {
            return TryBuildPylon(startLocation_,20.0);
        }
        return false;
	case 17:
        if (twilight_council_count>0) {
            stage_number=18;
            return false;
        }
        if(observation->GetMinerals()>=150&&observation->GetVespene()>=100){
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_TWILIGHTCOUNCIL, UNIT_TYPEID::PROTOSS_PROBE);
        }
    case 18:
        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY)) {
            stage_number=19;
            return false;
        }
        return false;
    case 19:
        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY)) {
            stage_number=20;
            return false;
        }
        return false;
    case 20:
		if (gateway_count>2) {
			stage_number=21;
			return false;
		}
		if (observation->GetMinerals()>150) {
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_PROBE);
		}
		return false;
    case 21:
        if (pylons.size()>3) {
            stage_number=22;
            return false;
        }
        if (observation->GetMinerals()>100) {
            return TryBuildPylon(startLocation_,20.0);
        }
        return false;
    case 22:
        Actions()->UnitCommand(probe_forward, ABILITY_ID::MOVE, advance_pylon_location);
        if (TryBuildUnit(ABILITY_ID::RESEARCH_ADEPTRESONATINGGLAIVES, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL)) {
            stage_number=23;
            return false;
        }
    case 23:
        if (observation->GetMinerals() < 100 || observation->GetVespene() < 50) {
            return false;
        }
        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY)) {
            stage_number=24;
            return false;
        }
        return false;
    case 24:
        if (observation->GetMinerals() < 100 || observation->GetVespene() < 50) {
            return false;
        }
        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY)) {
            stage_number=25;
            return false;
        }
        return false;
    case 25:
        if (observation->GetMinerals() < 100 || observation->GetVespene() < 50) {
            return false;
        }
        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY)) {
            stage_number=26;
            return false;
        }
        return false;
    case 26:
        if (branch == 3) {
            stage_number=50;
            return false;
        }

        if (advance_pylon != nullptr) {
            stage_number=27;
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
            stage_number=28;
            return false;
        }
        if (observation->GetMinerals()>400) {
            TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
        }
        return false;
    case 28:
        if (bases.size()<2) {
            stage_number=27;
            return false;
        }
        if (CountUnitTypeNearLocation(UNIT_TYPEID::PROTOSS_PYLON, front_expansion, 8.0)>=1) {
            stage_number=29;
            return false;
        }
        if (observation->GetMinerals() > 100) {
            TryBuildPylon(front_expansion, 6.0);
        }
        return false;
    case 29:
        if (CountUnitType(observation,UNIT_TYPEID::PROTOSS_ADEPT)>9) {
            stage_number=30;
            return false;
        }
        TryWarpAdept();
    case 30:
        if (CountUnitType(observation,UNIT_TYPEID::PROTOSS_STALKER)>7) {
            stage_number=31;
            return false;
        }
        TryWarpUnitPosition(ABILITY_ID::TRAINWARP_STALKER, front_expansion);
    case 31:
        if (bases.size()>=3) {
            stage_number=32;
            return false;
        }
        if (observation->GetMinerals()>400) {
            TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
        }
        return false;
    case 32:
        if (robotics_facility_count>=1) {
            stage_number=33;
            return false;
        }
        if (observation->GetMinerals() > 200 && observation->GetVespene() > 100) {
            TryBuildStructureNearPylon(ABILITY_ID::BUILD_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_PROBE);
        }
        return false;
    case 33:
        if (stalkers.size()>=5) {
            stage_number=34;
            return false;
        }
        TryWarpUnitPosition(ABILITY_ID::TRAINWARP_STALKER, front_expansion);
        return false;
    case 34:
        if (gateway_count>=7) {
			stage_number=35;
			return false;
		}
		if (observation->GetMinerals()>150) {
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_PROBE);
		}
		return false;
    case 35:
        if (robotics_bay_count > 0) {
            stage_number = 36;
            return false;
        }
        if (robotics_facility_count > 0 && robotics_bay_count < 1) {
            if (observation->GetMinerals() > 200 && observation->GetVespene() > 200) {
                TryBuildStructureNearPylon(ABILITY_ID::BUILD_ROBOTICSBAY, UNIT_TYPEID::PROTOSS_PROBE);
            }
        }
        return false;




    //branch 3
    case 50:
        if (templar_archive_count>0) {
            stage_number=51;
            return false;
        }
        TryBuildStructureNearPylon(ABILITY_ID::BUILD_TEMPLARARCHIVE, UNIT_TYPEID::PROTOSS_PROBE);
        return false;
    case 51:
        if (templar_archive_count==0) {
            stage_number=50;
            return false;
        }
        if (advance_pylon != nullptr) {
            stage_number=52;
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
    case 52:
        if (gateway_count>3) {
			stage_number=53;
			return false;
		}
		if (observation->GetMinerals()>150) {
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_PROBE);
		}
		return false;
    case 53:
        if (gateway_count<4) {
            stage_number=52;
            return false;
        }
        TryBuildPylonIfNeeded(2);
        /*if (observation->GetMinerals()<100 && observation->GetVespene()>200){
            TryWarpTemplar();
        }
        else {
            TryWarpAdept();
        }*/
        TryWarpTemplar();
        if (templars.size() > 1) {
            Units templar_merge;
            for (int i = 0; i < 2; i++) {
                templar_merge.push_back(templars.at(i));
            }
            //Actions()->UnitCommand(templar_merge, 1767);
            Actions()->UnitCommand(templars.at(0), 1767, templars.at(1));

        }
        return false;

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
                return TryBuildUnit(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY);
                //return TryBuildUnitChrono(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY);
            }
        }
        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_STALKER)<2) {
            return false;
        }
        stage_number=101;
        return false;
    case 101:
        if (pylons.size()>2) {
            stage_number=102;
            return false;
        }
        if (observation->GetMinerals()>100) {
            return TryBuildPylon(startLocation_,20.0);
        }
        return false;
    case 102:
        if (robotics_facility_count>=1) {
            stage_number=103;
            return false;
        }
        if (observation->GetMinerals() > 200 && observation->GetVespene() > 100) {
            TryBuildStructureNearPylon(ABILITY_ID::BUILD_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_PROBE);
        }
        return false;
    case 103:
        if (robotics_facility_count<1) {
            stage_number=102;
            return false;
        }
		if (gateway_count>3) {
			stage_number=104;
			return false;
		}
		if (observation->GetMinerals()>150) {
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_PROBE);
		}
		return false;
    case 104:
        //TryChronoboost(cores.front());
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
            return TryBuildUnit(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY);
        }
        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_STALKER)<4) {
            return false;
        }
        stage_number=105;
        return false;
    case 105:
        if (CountUnitType(UNIT_TYPEID::PROTOSS_WARPPRISM)>0) {
            stage_number=106;
            return false;
        }
        return TryBuildUnitChrono(ABILITY_ID::TRAIN_WARPPRISM, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);
    case 106:
        if (TryBuildUnit(ABILITY_ID::TRAIN_OBSERVER, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY)) {
            stage_number=107;
            return false;
        }
        return false;
    case 107:
        if (pylons.size()>3) {
            stage_number=108;
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
            stage_number=109;
            return false;
	    }
        Actions()->UnitCommand(warpprisms.front(),ABILITY_ID::LOAD,stalkers.front());
        return false;
	case 109:
	    if (warpprisms.empty()) {
            stage_number=110;
            return false;
	    }
	    else{
            if (Query()->PathingDistance(warpprisms.front()->pos,Point2D(game_info_.enemy_start_locations.front().x+3,game_info_.enemy_start_locations.front().y))<20) {
                Actions()->UnitCommand(warpprisms.front(), ABILITY_ID::UNLOADALLAT_WARPPRISM, warpprisms.front()->pos);
                if (warpprisms.front()->cargo_space_taken==0) {
                    Actions()->UnitCommand(warpprisms.front(), ABILITY_ID::MORPH_WARPPRISMPHASINGMODE);
                }
            }
            //std::cout<<Query()->PathingDistance(warpprisms.front()->pos,game_info_.enemy_start_locations.front())<<std::endl;

            //Actions()->UnitCommand(warpprisms.front(), ABILITY_ID::MORPH_WARPPRISMPHASINGMODE);
            return false;
	    }
	case 110:
        TryWarpUnitPrism(ABILITY_ID::TRAINWARP_STALKER);
	}

	return false;
}
