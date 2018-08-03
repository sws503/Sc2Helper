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

	//건물 지을 프로브 재지정
	if (workers.size() > 2 && (probe_forward != nullptr && !probe_forward->is_alive)) {
		for (const auto& p : workers) {
			if (probe_scout != nullptr && p->tag == probe_scout->tag) continue;
			probe_forward = p;
			break;
		}
	}

	//브랜치 지정
	//디폴트 branch = 2
	if (flags.status("search_branch") == 1) {
		// 정찰 실패: 입구를 막았거나 프로브가 죽었음
		if (flags.status("search_result") == 1) {
			branch = 2;
		}
		// 적이 정석 빌드를 감: 멀티가 있거나 barracks, gateway가 있거나 extracter가 없음
		else if (flags.status("search_result") == 2) {
			branch = 0;
		}
		// 적이 심상치 않음: extracter가 있거나, 멀티도 없고 barracks, gateway도 없다.
		//					또는 정찰 가다가 본진 밖에 있는 건물을 봤다.
		else if (flags.status("search_result") == 3) {
			branch = 1;
		}
	}

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

	if (bases.size()==1) {
        if (observation->GetFoodWorkers()<23) {
            TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
        }
	}
	else{
        for (const auto& b : bases) {
            if (b->build_progress < 1.0) {
                if(observation->GetFoodWorkers() < max_worker_count_) {
                    TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
                }
            }
        }

        if (GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR) >= observation->GetFoodWorkers() && observation->GetFoodWorkers() < max_worker_count_) {
            TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
        }
	}


	if (branch<2 && stage_number>30) {
        TryBuildPylonIfNeeded(2);
	}
	if (branch<2 && stage_number>36) {
        if (bases.size()*2>assimilator_count) {
            TryBuildAssimilator();
        }
        if (forge_count<2) {
            TryBuildStructureNearPylon(ABILITY_ID::BUILD_FORGE, UNIT_TYPEID::PROTOSS_FORGE);
        }

        if (gateway_count<=bases.size()*2 && gateway_count<10) {
            TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_GATEWAY);
        }
        else if (observation->GetFoodUsed()>120 && GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR) <= observation->GetFoodWorkers() ) {
            for (const auto& b : bases) {
                if (b->build_progress < 1.0) {
                    return TryBuildArmyBranch0();
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

	if (branch == 5) {
        if (!BlinkResearched && stage_number>211) {
            TryChronoboost(observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL)).front());
        }
        if (stage_number>218) {
            TryBuildPylonIfNeeded(2);
        }
        if (stage_number>228) {
            if (bases.size()*2<assimilator_count+2) {
                TryBuildAssimilator();
            }
            if (TryBuildCannonNexus()<bases.size()){
                return false;
            }
            if (observation->GetFoodUsed()>150) {
                TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
            }
            if (warpprisms_phasing.empty()) {
                TryBuildArmyBranch5();
            }
            else {
                TryWarpUnitPrism(ABILITY_ID::TRAINWARP_ZEALOT);
            }

        }
	}

	switch (stage_number) {
	case 0:
		if (bases.size() > 1) {
			for (const auto& b : bases) {
				if (Distance2D(b->pos, startLocation_) < 3) {
					base = b;
				}
			}
		}
		//본진 넥서스 지정
		else if (bases.size() == 1){
			base = bases.front();
		}
		//정찰 프로브 지정
		if (probe_scout == nullptr || !probe_scout->is_alive) {
			for (const auto& p : workers) {
				if (probe_forward == p) continue;
				probe_scout = p;
				break;
			}
		}
		//건물 지을 프로브 지정
		if (probe_forward == nullptr || !probe_forward->is_alive) {
			for (const auto& p : workers) {
				if (probe_scout == p) continue;
				probe_forward = p;
				break;
			}
		}
        if (probe_scout != nullptr && probe_forward != nullptr) {
            if (branch == 5) {
                stage_number =200;
            }
            else {
                stage_number=1;
            }
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
		TrybuildFirstPylon();
		return false;
	case 2:
		if (gateway_count>0) {
			stage_number=3;
			return false;
		}
		if (observation->GetMinerals()>150) {
		    TryChronoboost(base);
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_GATEWAY);
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
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_GATEWAY);
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
		// 정찰 : 분기 1, 2 정찰 시작
		if (cybernetics_count>0) {
			stage_number=7;
			return false;
		}
		if (observation->GetMinerals()>150) {
			return TryBuildStructureNearPylon(ABILITY_ID::BUILD_CYBERNETICSCORE, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
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
        if (try_stalker>=2) {
            stage_number=10;
            return false;
        }
        return TryBuildUnitChrono(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_STALKER);
    case 10:
        if (!cores.front()->orders.empty()) {
            stage_number=11;
            return false;
        }
        if (TryBuildUpgrade(ABILITY_ID::RESEARCH_WARPGATE, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE, UPGRADE_ID::WARPGATERESEARCH)) {
            stage_number=11;
            return false;
        }
        return false;
    case 11:
		// 정찰 : 분기 2 결정
        if (branch == 2) {
            stage_number = 100;
            return false;
        }

        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_ADEPT)) {
            stage_number=12;
            return false;
        }
        return false;
    case 12:
        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_ADEPT)) {
            stage_number=13;
            return false;
        }
        return false;
    case 13:
		// 정찰: 분기 1 결정.
        if (branch == 0 || branch == 3) {
            stage_number = 16;
            return false;
        }
        if (branch == 1) {
            stage_number=14;
            return false;
        }
        return false;
    case 14:
        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_SHIELDBATTERY)>=2) {
            stage_number=15;
            return false;
        }
        if (observation->GetMinerals()>100){
            TryBuildStructureNearPylon(ABILITY_ID::BUILD_SHIELDBATTERY, UNIT_TYPEID::PROTOSS_SHIELDBATTERY);
        }
        return false;
    case 15:
        if (try_stalker>=4) {
            stage_number++;
            return false;
        }
        return TryBuildUnit(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_STALKER);
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
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_TWILIGHTCOUNCIL, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
        }
        return false;
    case 18:
        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_ADEPT)) {
            stage_number=19;
            return false;
        }
        return false;
    case 19:
        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_ADEPT)) {
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
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_GATEWAY);
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
		work_probe_forward = false;
        Actions()->UnitCommand(probe_forward, ABILITY_ID::MOVE, advance_pylon_location);
        if (TryBuildUpgrade(ABILITY_ID::RESEARCH_ADEPTRESONATINGGLAIVES, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL, UPGRADE_ID::ADEPTPIERCINGATTACK)) {
            stage_number=23;
            return false;
        }
        return false;
    case 23:
        if (observation->GetMinerals() < 100 || observation->GetVespene() < 50) {
            return false;
        }
        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_ADEPT)) {
            stage_number=24;
            return false;
        }
        return false;
    case 24:
        if (observation->GetMinerals() < 100 || observation->GetVespene() < 50) {
            return false;
        }
        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_ADEPT)) {
            stage_number=25;
            return false;
        }
        return false;
    case 25:
        if (observation->GetMinerals() < 100 || observation->GetVespene() < 50) {
            return false;
        }
        if (TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_ADEPT)) {
            stage_number=26;
            return false;
        }
        return false;
    case 26:
        if (branch==3) {
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
				work_probe_forward = true;
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
        if (try_adept>8) {
            stage_number=30;
            return false;
        }
        TryWarpAdept();
        return false;
    case 30:
        if (CountUnitType(observation,UNIT_TYPEID::PROTOSS_STALKER)>10) {
            stage_number=31;
            return false;
        }
        return TryWarpUnitPosition(ABILITY_ID::TRAINWARP_STALKER, front_expansion);
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
            TryBuildStructureNearPylon(ABILITY_ID::BUILD_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);
        }
        return false;
    case 33:
        if (stalkers.size()>=14) {
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
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_GATEWAY);
		}
		return false;
    case 35:
        if (robotics_bay_count > 0) {
            stage_number = 36;
            return false;
        }
        if (robotics_facility_count > 0 && robotics_bay_count < 1) {
            if (observation->GetMinerals() > 200 && observation->GetVespene() > 200) {
                TryBuildStructureNearPylon(ABILITY_ID::BUILD_ROBOTICSBAY, UNIT_TYPEID::PROTOSS_ROBOTICSBAY);
            }
        }
        return false;
    case 36:
        if (BlinkResearched || !observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL)).front()->orders.empty()) {
            stage_number=37;
            return false;
        }
        TryBuildUpgrade(ABILITY_ID::RESEARCH_BLINK, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL, UPGRADE_ID::BLINKTECH);
        return false;

    //branch 3
    case 50:
        if (templar_archive_count>0) {
            stage_number=51;
            return false;
        }
        TryBuildStructureNearPylon(ABILITY_ID::BUILD_TEMPLARARCHIVE, UNIT_TYPEID::PROTOSS_TEMPLARARCHIVE);
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
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_GATEWAY);
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
                return TryBuildUnit(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_STALKER);
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
        return TryBuildPylon(startLocation_,20.0);
    case 102:
        if (robotics_facility_count>=1) {
            stage_number=103;
            return false;
        }
        return TryBuildStructureNearPylon(ABILITY_ID::BUILD_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);

    case 103:
        if (robotics_facility_count<1) {
            stage_number=102;
            return false;
        }
		if (gateway_count>3) {
			stage_number=104;
			return false;
		}
		return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_GATEWAY);
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
            return TryBuildUnit(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_STALKER);
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
        return TryBuildUnitChrono(ABILITY_ID::TRAIN_WARPPRISM, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_WARPPRISM);
    case 106:
        if (TryBuildUnit(ABILITY_ID::TRAIN_OBSERVER, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_OBSERVER)) {
            stage_number=107;
            return false;
        }
        return false;
    case 107:
        if (pylons.size()>3) {
            stage_number=108;
            return false;
        }
        return TryBuildPylon(startLocation_,20.0);
	case 108:
	    if (CountUnitType(UNIT_TYPEID::PROTOSS_WARPPRISM)<1) {
            stage_number=109;
            return false;
	    }
	    return false;
	case 109:
	    if (warpprisms.empty()) {
            stage_number=110;
            return false;
	    }
	    else{

            //std::cout<<Query()->PathingDistance(warpprisms.front()->pos,game_info_.enemy_start_locations.front())<<std::endl;

            //Actions()->UnitCommand(warpprisms.front(), ABILITY_ID::MORPH_WARPPRISMPHASINGMODE);
            return false;
	    }
	case 110:
        return TryWarpUnitPrism(ABILITY_ID::TRAINWARP_STALKER);

	//branch 5
	case 200:
		if (pylons.size()>0) {
            if (pylons.front()->build_progress == 1.0) {
                stage_number=201;
                return false;
            }
		}
		return TrybuildFirstPylon();
	case 201:
		if (gateway_count>0) {
			stage_number=202;
			return false;
		}
		if (observation->GetMinerals()>150) {
		    TryChronoboost(base);
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_GATEWAY);
		}
		return false;
	case 202:
		if (assimilator_count>=1) {
			stage_number=203;
			return false;
		}
		if (observation->GetFoodWorkers()>=18) {
            if (TryBuildGas(base->pos)) {
                stage_number=203;
                return false;
            }
		}
		return false;
    case 203:
        if (bases.size()>=2) {
            stage_number=204;
            return false;
        }
        return TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
    case 204:
		if (cybernetics_count>0) {
			stage_number=205;
			return false;
		}
		return TryBuildStructureNearPylon(ABILITY_ID::BUILD_CYBERNETICSCORE, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
    case 205:
		if (assimilator_count>=2) {
			stage_number=206;
			return false;
		}
		return TryBuildGas(base->pos);
    case 206:
        if (pylons.size()>1) {
            stage_number=207;
            return false;
        }
        return TryBuildPylon(startLocation_,20.0);
	case 207:
	    if (try_stalker>0) {
            stage_number=208;
            return false;
	    }
        return TryBuildUnitChrono(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_STALKER);
    case 208:
        if (!cores.front()->orders.empty()) {
            stage_number=209;
            return false;
        }
        if (TryBuildUpgrade(ABILITY_ID::RESEARCH_WARPGATE, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE, UPGRADE_ID::WARPGATERESEARCH)) {
            stage_number=209;
            return false;
        }
        return false;
    case 209:
        if (twilight_council_count>0) {
            for (const auto& b : bases) {
                if (b!=base) {
                    TryChronoboost(b);
                }
            }
            stage_number=210;
            return false;
        }
        return TryBuildStructureNearPylon(ABILITY_ID::BUILD_TWILIGHTCOUNCIL, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
    case 210:
        if (try_stalker>1) {
            stage_number=211;
            return false;
        }
        return TryBuildUnit(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_STALKER);
    case 211:
        if (BlinkResearched || !observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL)).front()->orders.empty()) {
            stage_number=212;
            return false;
        }
        TryBuildUpgrade(ABILITY_ID::RESEARCH_BLINK, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL, UPGRADE_ID::BLINKTECH);
        return false;
    case 212:
        if (pylons.size()>2) {
            stage_number=213;
            return false;
        }
        return TryBuildPylon(startLocation_,20.0);
    case 213:
        if (try_stalker>2) {
            stage_number=214;
            return false;
        }
        return TryBuildUnit(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_STALKER);
    case 214:
        if (robotics_facility_count>=1) {
            stage_number=215;
            return false;
        }
        return TryBuildStructureNearPylon(ABILITY_ID::BUILD_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);
    case 215:
        if (gateway_count>2) {
			stage_number=216;
			return false;
		}
		return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_GATEWAY);
    case 216:
        if (try_stalker>3) {
            stage_number=217;
            return false;
        }
        return TryBuildUnit(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_STALKER);
    case 217:
        if (assimilator_count>=4) {
			stage_number=218;
			return false;
		}
		return TryBuildAssimilator();
    case 218:
        if (pylons.size()>3) {
            stage_number=219;
            return false;
        }
        return TryBuildPylon(startLocation_,20.0);
    case 219:
        if (TryBuildUnit(ABILITY_ID::TRAIN_OBSERVER, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_OBSERVER)) {
            stage_number=220;
            return false;
        }
        return false;
    case 220:
        if (bases.size()>=3) {
            stage_number=221;
            return false;
        }
        return TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
    case 221:
        if (TryBuildUnit(ABILITY_ID::TRAIN_OBSERVER, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_OBSERVER)) {
            stage_number=222;
            return false;
        }
        return false;
    case 222:
        if (try_stalker>6) {
            stage_number=223;
            return false;
        }
        return TryWarpUnitPosition(ABILITY_ID::TRAINWARP_STALKER, front_expansion);
    case 223:
        if (gateway_count>4) {
			stage_number=224;
			return false;
		}
		return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_GATEWAY);
    case 224:
        if (!observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL)).front()->orders.empty()) {
            stage_number=225;
            return false;
        }
        if (TryBuildUpgrade(ABILITY_ID::RESEARCH_CHARGE, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL, UPGRADE_ID::CHARGE)) {
            stage_number=225;
            return false;
        }
        return false;
    case 225:
        if (forge_count<2) {
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_FORGE, UNIT_TYPEID::PROTOSS_FORGE);
        }
        else {
            stage_number=226;
            return false;
        }
    case 226:
        if (try_stalker>7) {
            stage_number=227;
            return false;
        }
        return TryWarpUnitPosition(ABILITY_ID::TRAINWARP_STALKER, front_expansion);
    case 227:
        if (!observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY)).front()->orders.empty()) {
            if (observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY)).front()->orders.front().ability_id == ABILITY_ID::TRAIN_OBSERVER) {
                return false;
            }
            stage_number=228;
            return false;
        }
        if (TryBuildUnit(ABILITY_ID::TRAIN_WARPPRISM, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_WARPPRISM)) {
            stage_number=228;
            return false;
        }
        return false;
    case 228:
        if (gateway_count>6) {
			stage_number=229;
			return false;
		}
		return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_GATEWAY);

    default:
        return false;



	}



	return false;
}
