#include "memibot.h"

// Todo: trybuildunit() 호출하기 전에 자원, 인구수가 있는지 체크하기
bool MEMIBot::EarlyStrategy() {
	const ObservationInterface* observation = Observation();
	Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PROBE));
	Units bases = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));
	Units pylons = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PYLON));
	Units gateways = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_GATEWAY));
	Units warpgates = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_WARPGATE));
	Units stargates = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_STARGATE));
	Units cores = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_CYBERNETICSCORE));
	Units enemy_structures = observation->GetUnits(Unit::Alliance::Enemy, IsStructure(observation));
	Units enemy_townhalls = observation->GetUnits(Unit::Alliance::Enemy, IsTownHall());
    Units warpprisms = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_WARPPRISM));
	Units warpprisms_phasing = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_WARPPRISMPHASING));
	Units stalkers = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_STALKER));
    Units templars = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_HIGHTEMPLAR));
    Units archons = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_ARCHON));
    Units robotics = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY));

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
	/*if (branch !=5 && flags.status("search_branch") == 1) {
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
	}*/

	size_t forge_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_FORGE);
	size_t cannon_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_PHOTONCANNON);
	size_t battery_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_SHIELDBATTERY);
	size_t gateway_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_GATEWAY) + CountUnitType(observation, UNIT_TYPEID::PROTOSS_WARPGATE);
	size_t assimilator_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ASSIMILATOR);
	size_t cybernetics_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
	size_t stargate_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_STARGATE);
    size_t twilight_council_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
    size_t robotics_facility_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);
	size_t robotics_bay_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ROBOTICSBAY);

    size_t templar_archive_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_TEMPLARARCHIVE);

#ifdef DEBUG
	std::cout << "branch : " << branch << ", stage : "<< stage_number << std::endl;
#endif

    /*if (branch==5 && stage_number<208) {
        if (observation->GetFoodUsed()<21) {
            TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
        }
    }*/
	if (bases.size()==1) {
        if (observation->GetFoodWorkers()<23) {
            TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
        }
	}
	else{
        for (const auto& b : bases) {
            if (b->build_progress < 1.0f) {
                if(observation->GetFoodWorkers() < max_worker_count_) {
                    TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
                }
            }
        }

        if (GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR) >= observation->GetFoodWorkers() && observation->GetFoodWorkers() < max_worker_count_) {
            TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
        }
	}

	if (stage_number>18) {
        TryBuildPylonIfNeeded(2);
        TryBuildUnit(ABILITY_ID::TRAIN_VOIDRAY, UNIT_TYPEID::PROTOSS_STARGATE, UNIT_TYPEID::PROTOSS_VOIDRAY);
        if (observation->GetMinerals()>400 && !stargates.front()->orders.empty()) {
            TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
        }
        if (bases.size()>stargates.size()) {
            TryBuildStructureNearPylon(ABILITY_ID::BUILD_STARGATE, UNIT_TYPEID::PROTOSS_STARGATE);
        }
        if (bases.size()*2>assimilator_count) {
            TryBuildAssimilator();
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
        if (probe_scout != nullptr && probe_forward != nullptr && observation->GetFoodUsed()>13) {
            stage_number=1;
            return false;
        }
		return false;
	case 1:
		if (pylons.size()>0) {
            if (pylons.front()->build_progress == 1.0f) {
                stage_number=2;
                return false;
            }
		}
		return TryBuildStructureAtLocation(ABILITY_ID::BUILD_PYLON, UNIT_TYPEID::PROTOSS_PYLON, Pylon1);
	case 2:
		if (gateway_count>0) {
			stage_number=3;
			return false;
		}
		if (observation->GetMinerals()>150) {
            work_probe_forward = false;
		    TryChronoboost(base);
		    return TryBuildStructureAtLocation(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_GATEWAY, Gate1);
        }
		return false;
	case 3:
	    Actions()->UnitCommand(probe_forward, ABILITY_ID::MOVE, Pylon2);
		if (assimilator_count>=2) {
			stage_number=4;
			return false;
		}
		if (observation->GetMinerals()>75) {
			return TryBuildGas(base->pos);
		}
		return false;
    case 4:
        if (pylons.size()>1) {
            stage_number=5;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_PYLON, UNIT_TYPEID::PROTOSS_PYLON, Pylon2);
	case 5:
		// 정찰 : 분기 1, 2 정찰 시작
		if (cybernetics_count>0) {
			stage_number=6;
			return false;
		}
		return TryBuildStructureAtLocation(ABILITY_ID::BUILD_CYBERNETICSCORE, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE, Core1);
    case 6:
        if (pylons.size()>2) {
            stage_number=7;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_PYLON, UNIT_TYPEID::PROTOSS_PYLON, Pylon3);
    case 7:
        if (stargate_count > 0) {
			stage_number=8;
			return false;
		}
		return TryBuildStructureAtLocation(ABILITY_ID::BUILD_STARGATE, UNIT_TYPEID::PROTOSS_STARGATE, Star1);
    case 8:
        if (!gateways.front()->orders.empty()) {
            stage_number=9;
            return false;
        }
        return TryBuildUnit(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_STALKER);
    case 9:
        if (!cores.front()->orders.empty()) {
            stage_number=10;
            return false;
        }
        return TryBuildUpgrade(ABILITY_ID::RESEARCH_PROTOSSAIRWEAPONS, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE, UPGRADE_ID::PROTOSSAIRWEAPONSLEVEL1);
    case 10:
        if (battery_count>0) {
            stage_number=11;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_SHIELDBATTERY, UNIT_TYPEID::PROTOSS_SHIELDBATTERY, Batt1);
    case 11:
        if (battery_count>1) {
            stage_number=12;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_SHIELDBATTERY, UNIT_TYPEID::PROTOSS_SHIELDBATTERY, Batt2);
    case 12:
        if (!gateways.front()->orders.empty()) {
            if (gateways.front()->orders.front().progress>0.5f){
                return false;
            }
            stage_number=13;
            return false;
        }
        return TryBuildUnit(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_STALKER);
    case 13:
        if (!stargates.front()->orders.empty()) {
            stage_number=14;
            return false;
        }
        return TryBuildUnit(ABILITY_ID::TRAIN_ORACLE, UNIT_TYPEID::PROTOSS_STARGATE, UNIT_TYPEID::PROTOSS_ORACLE);
    case 14:
        if (battery_count>2) {
            stage_number=15;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_SHIELDBATTERY, UNIT_TYPEID::PROTOSS_SHIELDBATTERY, Batt3);
    case 15:
        if (battery_count>3) {
            stage_number=16;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_SHIELDBATTERY, UNIT_TYPEID::PROTOSS_SHIELDBATTERY, Batt4);
    case 16:
        if (battery_count>4) {
            stage_number=17;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_SHIELDBATTERY, UNIT_TYPEID::PROTOSS_SHIELDBATTERY, Batt5);
    case 17:
        if (!stargates.front()->orders.empty()) {
            if (stargates.front()->orders.front().progress>0.4f) {
                return false;
            }
            stage_number=18;
            return false;
        }
        return TryBuildUnit(ABILITY_ID::TRAIN_VOIDRAY, UNIT_TYPEID::PROTOSS_STARGATE, UNIT_TYPEID::PROTOSS_VOIDRAY);
    case 18:
        if (pylons.size()>3) {
            stage_number=19;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_PYLON, UNIT_TYPEID::PROTOSS_PYLON, Pylon4);


    default:
        return false;





	}



	return false;
}
