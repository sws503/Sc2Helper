#include "memibot.h"

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
	Units adepts = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_ADEPT));
    Units stalkers = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_STALKER));
    Units templars = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_HIGHTEMPLAR));
    Units archons = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_ARCHON));
    Units robotics = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY));
    Units twilights = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL));
    Units forges = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_FORGE));
	//건물 지을 프로브 재지정
	if (workers.size() > 2 && (probe_forward != nullptr && !probe_forward->is_alive)) {
		for (const auto& p : workers) {
			if (probe_scout != nullptr && p->tag == probe_scout->tag) continue;
			probe_forward = p;
			break;
		}
	}
	//브랜치 지정
	/*if (branch ==0 && flags.status("search_branch") == 1) {
		// 정찰 실패: 입구를 막았거나 프로브가 죽었음
		if (flags.status("search_result") == 1) {
			//branch = 2;
		}
		// 적이 정석 빌드를 감: 멀티가 있거나 barracks, gateway가 있거나 extracter가 없음
		else if (flags.status("search_result") == 2) {
			//branch = 0;
		}
		// 적이 심상치 않음: extracter가 있거나, 멀티도 없고 barracks, gateway도 없다.
		//					또는 정찰 가다가 본진 밖에 있는 건물을 봤다.
		else if (flags.status("search_result") == 3) {
			branch = 1;
		}
	}*/


	size_t observer_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_OBSERVER);
	size_t forge_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_FORGE);
	size_t cannon_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_PHOTONCANNON);
	size_t gateway_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_GATEWAY) + CountUnitType(observation, UNIT_TYPEID::PROTOSS_WARPGATE);
	size_t battery_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_SHIELDBATTERY);
	size_t assimilator_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ASSIMILATOR);
	size_t cybernetics_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
	size_t stargate_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_STARGATE);
    size_t twilight_council_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
    size_t robotics_facility_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);
	size_t robotics_bay_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ROBOTICSBAY);
	size_t fleetbeacon_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_FLEETBEACON);
    size_t templar_archive_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_TEMPLARARCHIVE);
    size_t sentry_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_SENTRY);


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

        if (branch==6 && observation->GetFoodWorkers()<24) {
            TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
        }
	}
	else{
        for (const auto& b : bases) {
            if (b->build_progress < 1.0f) {
                next_expansion = Point2D(0,0);
                if(GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR)+10 >= observation->GetFoodWorkers() && observation->GetFoodWorkers() < max_worker_count_) {
                    TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
                }
            }
        }

        if (GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR) >= observation->GetFoodWorkers() && observation->GetFoodWorkers() < max_worker_count_) {
            TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
        }
	}
    if (branch == 0) {
        if (stage_number>12 && stage_number<25) {
            TryBuildPylonIfNeeded();
        }
        if (stage_number>24) {
            if (GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR) <= observation->GetFoodWorkers() || bases.size()<3) {
                bool expand = true;
                if (ManyEnemyRush) {
                    expand = false;
                }
                if (try_colossus<bases.size()*6-14) {
                    expand = false;
                }
                for (const auto& b :bases) {
                    if (b->build_progress < 1.0f) {
                        expand = false;
                    }
                }
                if (expand || bases.size()<3) {
                    return TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
                }
            }
            TryBuildPylonIfNeeded(3);
            if (bases.size()*2>assimilator_count) {
                TryBuildAssimilator();
            }
            if (gateway_count<bases.size()*2) {
                TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_GATEWAY);
            }
            if (robotics_facility_count<3) {
				if (bases.size() < 4) {
					if (robotics_facility_count < 2) {
						TryBuildStructureNearPylon(ABILITY_ID::BUILD_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);
					}
				}
                TryBuildStructureNearPylon(ABILITY_ID::BUILD_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);
            }
            TryBuildArmyBranch0();
        }
    }
	else if (branch == 5) {
        if (observer_count<3 && stage_number>220 && stage_number<233) {
            TryBuildUnit(ABILITY_ID::TRAIN_OBSERVER, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_OBSERVER);
        }
        if (!BlinkResearched && stage_number>211 && !twilights.front()->orders.empty()) {
            if (twilights.front()->orders.front().progress<0.6f) {
                TryChronoboost(observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL)).front());
            }
        }
        if (stage_number>218) {
            TryBuildPylonIfNeeded(2);
        }
        if (stage_number>232) {
            if (GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR) <= observation->GetFoodWorkers() || bases.size()<3) {
                bool expand = true;
                if (ManyEnemyRush) {
                    expand = false;
                }
                if (bases.size()>=num_expand) {
                    expand = false;
                }
                if (num_expand==6) {
                    expand = true;
                }
                for (const auto& b :bases) {
                    if (b->build_progress < 1.0f) {
                        expand = false;
                    }
                }
                if (expand) {
                    return TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
                }
            }

            TryBuildCannonNexus();
            if (bases.size()*2>assimilator_count+1) {
                TryBuildAssimilator();
            }



            if (gateway_count<bases.size()*2-1) {
                TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_GATEWAY);
            }

            if (warpprisms_phasing.empty()) {
                TryBuildArmyBranch5();
            }
            else {
                TryWarpUnitPrism(ABILITY_ID::TRAINWARP_ZEALOT);
            }

        }
	}
	else if (branch==6) {
		if (cores.empty() && stage_number>604) {
			return TryBuildStructureAtLocation(ABILITY_ID::BUILD_CYBERNETICSCORE, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE, Core1);
		}

        if (stage_number>613) {
            TryBuildPylonIfNeeded();
            if (gateway_count == 0) {
                stage_number = 602;
                return false;
            }
            if (tryadeptbranch6 && gateways.front()->orders.empty() && num_adept<4) {
                TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_ADEPT);
            }
        }
        if (stage_number>616) {
            for (const auto& stargate : stargates) {
                TryChronoboost(stargate);
            }
        }
        if (stage_number>620) {
            TryBuildUnit(ABILITY_ID::TRAIN_VOIDRAY, UNIT_TYPEID::PROTOSS_STARGATE, UNIT_TYPEID::PROTOSS_VOIDRAY);

            TryBuildPylonIfNeeded(2);

            if (forge_count==0) {
                TryBuildStructureNearPylonInBase(ABILITY_ID::BUILD_FORGE, UNIT_TYPEID::PROTOSS_FORGE);
            }
            else if (CountUnitTypeNearLocation(UNIT_TYPEID::PROTOSS_PHOTONCANNON, Pylon4,8)==0) {
                float rx = GetRandomScalar();
                float ry = GetRandomScalar();
                Point2D build_location = Point2D(Pylon4.x + rx * 7, Pylon4.y + ry * 7);
                TryBuildStructure(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE, build_location);
            }
            TryBuildCannonNexus();
            TooMuchMineralBranch6();
        }
	}
	else if (branch==7) {
        /*if (stage_number>718 && stage_number<726) {
            TryBuildPylonIfNeeded();
        }*/
	    if (stage_number>725) {
            TryBuildPylonIfNeeded(stargate_count);
            for (const auto& stargate : stargates) {
                TryChronoboost(stargate);
            }
	    }
	    if (stage_number>728) {
            TryBuildCannonNexus(2);
	    }
        if (stage_number>732) {
            if (cores.empty()) {
                TryBuildStructureAtLocation(ABILITY_ID::BUILD_CYBERNETICSCORE, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE, Core1);
            }
            if (stargates.size()<4) {
                TryBuildStructureNearPylonInBase(ABILITY_ID::BUILD_STARGATE, UNIT_TYPEID::PROTOSS_STARGATE);
            }
            for (const auto& b : bases){
                TryBuildBatteryNexus(b);
            }
            TryBuildUnit(ABILITY_ID::TRAIN_CARRIER, UNIT_TYPEID::PROTOSS_STARGATE, UNIT_TYPEID::PROTOSS_CARRIER);
            if (bases.size()<3 || (num_carrier>5 && GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR) <= observation->GetFoodWorkers())) {
                bool expand = 1;
                if (ManyEnemyRush) {
                    expand = false;
                }
                for (const auto& b :bases) {
                    if (b->build_progress < 1.0f) {
                        expand = false;
                    }
                }
                if (expand) {
                    return TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
                }
            }
            TryBuildCannonPylonBranch7();

            if (bases.size()*2>assimilator_count) {
                TryBuildAssimilator();
            }
            if (bases.size()>3) {
                if (twilight_council_count==0) {
                    TryBuildStructureNearPylonInBase(ABILITY_ID::BUILD_TWILIGHTCOUNCIL, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
                }
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
		else if (bases.size() == 1){
			base = bases.front();
		}
		if (probe_scout == nullptr || !probe_scout->is_alive) {
			for (const auto& p : workers) {
				if (probe_forward == p) continue;
				probe_scout = p;
				break;
			}
		}
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
            else if (branch == 6) {
				work_probe_forward = false;
				Actions()->UnitCommand(probe_forward, ABILITY_ID::MOVE, Pylon2);
                stage_number = 601;
            }
            else if (branch == 7) {
                stage_number = 701;
            }
            else {
                stage_number=1;
            }
            return false;
        }
		return false;
	case 1:
		if (pylons.size()>0) {
            stage_number=2;
            return false;
		}
		TrybuildFirstPylon();
		return false;
	case 2:
		if (gateway_count>0) {
			stage_number=3;
			return false;
		}
		return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_GATEWAY);
	case 3:
	    TryChronoboost(base);
		if (assimilator_count>=1) {
			stage_number=4;
			return false;
		}
		if (observation->GetFoodWorkers()>=18) {
            if (TryBuildGas(base->pos)) {
                stage_number=4;
                return false;
            }
		}
		return false;
	case 4:
		if (bases.size()>=2) {
            stage_number=5;
            return false;
        }
        return TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
    case 6:
        if (pylons.size()>1) {
            stage_number=7;
            return false;
        }
        return TryBuildPylon(startLocation_,15.0f);
    case 5:
		if (cybernetics_count>0) {
			stage_number=6;
			return false;
		}
		return TryBuildStructureNearPylon(ABILITY_ID::BUILD_CYBERNETICSCORE, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
    case 7:
		if (assimilator_count>=2) {
			stage_number=8;
			return false;
		}
		return TryBuildGas(base->pos);
    case 8:
		if (gateway_count>2) {
			stage_number=9;
			return false;
		}
		return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_GATEWAY);
    case 9:
        for (const auto& gate : gateways) {
            if (gate->build_progress < 1.0f) continue;
            if (gate->orders.empty()) {
                return TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_ADEPT);
            }
        }
        stage_number=10;
        return false;
    case 10:
        if (cybernetics_count == 0) {
			return TryBuildStructureNearPylon(ABILITY_ID::BUILD_CYBERNETICSCORE, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
		}
        if (!cores.front()->orders.empty()) {
            stage_number=11;
            return false;
        }
        return TryBuildUpgrade(ABILITY_ID::RESEARCH_WARPGATE, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE, UPGRADE_ID::WARPGATERESEARCH);
    case 11:
        for (const auto& gate : gateways) {
            if (gate->orders.empty()) {
                return TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_ADEPT);
            }
        }
        for (const auto& b : bases) {
            if (b!=base) {
                TryChronoboost(b);
            }
        }
        if (num_adept==0) {
            return false;
        }
        stage_number=12;
        return false;
    case 12:
        if (pylons.size()>2) {
            stage_number=13;
            return false;
        }
        return TryBuildPylon(startLocation_,15.0);
    case 13:
        for (const auto& gate : gateways) {
            if (gate->orders.empty()) {
                return TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_ADEPT);
            }
        }
        if (num_adept<4) {
            return false;
        }
        stage_number=14;
        return false;
    case 14:
        if (assimilator_count>3) {
            stage_number=15;
            return false;
        }
        return TryBuildGas(front_expansion);
    case 15:
        if (robotics_facility_count>=1) {
            stage_number=16;
            return false;
        }
        return TryBuildStructureNearPylon(ABILITY_ID::BUILD_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);
    case 16:
        if (forge_count<1) {
            return TryBuildStructureNearPylonInBase(ABILITY_ID::BUILD_FORGE, UNIT_TYPEID::PROTOSS_FORGE);
        }
        else {
            stage_number=17;
            return false;
        }
    case 17:
        if (twilight_council_count>0) {
            stage_number=18;
            return false;
        }
        return TryBuildStructureNearPylon(ABILITY_ID::BUILD_TWILIGHTCOUNCIL, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
    case 18:
        if (sentry_count>2) {
            stage_number=19;
            return false;
        }
        return TryWarpUnitPosition(ABILITY_ID::TRAINWARP_SENTRY, front_expansion);
    case 19:
        if (robotics_bay_count>=1) {
            stage_number=20;
            return false;
        }
        return TryBuildStructureNearPylonInBase(ABILITY_ID::BUILD_ROBOTICSBAY, UNIT_TYPEID::PROTOSS_ROBOTICSBAY);
    case 20:
        if (forges.empty()) {
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_FORGE, UNIT_TYPEID::PROTOSS_FORGE);
        }
        if (!forges.front()->orders.empty()) {
            stage_number=21;
            return false;
        }
        return TryBuildUpgrade(ABILITY_ID::RESEARCH_PROTOSSGROUNDWEAPONS, UNIT_TYPEID::PROTOSS_FORGE, UPGRADE_ID::PROTOSSGROUNDWEAPONSLEVEL1);
    case 21:
        if (robotics_facility_count>1) {
            stage_number=22;
            return false;
        }
        return TryBuildStructureNearPylon(ABILITY_ID::BUILD_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);
    case 22:
        if (twilights.empty()) {
            return TryBuildStructureNearPylon(ABILITY_ID::BUILD_TWILIGHTCOUNCIL, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
        }
        if (!twilights.front()->orders.empty()) {
            stage_number=23;
            return false;
        }
        return TryBuildUpgrade(ABILITY_ID::RESEARCH_ADEPTRESONATINGGLAIVES,UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL,UPGRADE_ID::ADEPTPIERCINGATTACK);
    case 23:
        if (bases.size()>=3) {
            stage_number=24;
            return false;
        }
        return TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
    case 24:
        if (gateway_count>5) {
			stage_number=25;
			return false;
		}
		return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_GATEWAY);


	//branch 5
	case 200:
		if (pylons.size()>0) {
            stage_number=201;
            return false;
		}
		return TrybuildFirstPylon();
	case 201:
		if (gateway_count>0) {
			stage_number=202;
			return false;
		}
		return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_GATEWAY);
	case 202:
	    TryChronoboost(base);
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
        if (pylons.size()>1) {
            stage_number=206;
            return false;
        }
        return TryBuildPylon(startLocation_,15.0);
    case 206:
		if (assimilator_count>=2) {
			stage_number=207;
			return false;
		}
		return TryBuildGas(base->pos);

	case 207:
		if (gateway_count == 0) {
			stage_number = 201;
			return false;
		}
	    if (!gateways.front()->orders.empty()) {
            TryChronoboost(gateways.front());
            stage_number=208;
            return false;
	    }
        return TryBuildUnit(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_STALKER);
   case 208:
	    if (cybernetics_count == 0) {
	 	    stage_number = 204;
		    return false;
	    }
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
        return TryBuildStructureNearPylonInBase(ABILITY_ID::BUILD_TWILIGHTCOUNCIL, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
    case 210:
		if (gateway_count == 0) {
			stage_number = 201;
			return false;
		}
        if (!gateways.front()->orders.empty()) {
            if (gateways.front()->orders.front().progress<0.5f) {
                stage_number=211;
                return false;
            }
	    }
        return TryBuildUnit(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_STALKER);
    case 211:
		if (twilight_council_count==0) {
            stage_number=209;
            return false;
		}
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
        return TryBuildPylon(startLocation_,15.0);
    case 213:
		if (gateway_count == 0) {
			stage_number = 201;
			return false;
		}
        if (!gateways.front()->orders.empty()) {
            if (gateways.front()->orders.front().progress<0.5f) {
                stage_number=214;
                return false;
            }
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
        for (const auto& gate : gateways) {
            if (gate->build_progress != 1.0f) continue;
            if (gate->orders.empty()) continue;
            if (gate->orders.front().progress<0.5f) {
                stage_number=217;
                return false;
            }
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
        return TryBuildPylon(startLocation_,15.0);
    case 219:
        if (robotics.empty()) {
			stage_number = 214;
			return false;
		}
        if (!robotics.front()->orders.empty()) {
            stage_number=220;
            return false;
	    }
        return TryBuildUnit(ABILITY_ID::TRAIN_IMMORTAL, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_IMMORTAL);
    case 220:
		if (robotics.size() > 0) { TryChronoboost(robotics.front()); }
        if (bases.size()>=3) {
            stage_number=222;
            return false;
        }
        return TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
    /*case 221:
        if (robotics.empty()) {
			stage_number = 214;
			return false;
		}
        if (!robotics.front()->orders.empty()) {
            if (robotics.front()->orders.front().ability_id == ABILITY_ID::TRAIN_OBSERVER) {
                stage_number=220;
                return false;
            }
	    }
        return TryBuildUnit(ABILITY_ID::TRAIN_OBSERVER, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_OBSERVER);*/
    case 222:
        if (stalkers.size()>=7) {
            stage_number=225;
            return false;
        }
        return TryWarpUnitPosition(ABILITY_ID::TRAINWARP_STALKER, front_expansion);
    case 225:
        if (gateway_count>3) {
			stage_number=226;
			return false;
		}
		return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_GATEWAY);
    case 226:
		if (twilight_council_count == 0) {
			stage_number = 209;
			return false;
		}
        if (!observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL)).front()->orders.empty()) {
            stage_number=227;
            return false;
        }
        return TryBuildUpgrade(ABILITY_ID::RESEARCH_CHARGE, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL, UPGRADE_ID::CHARGE);
    case 227:
        if (forge_count<2) {
            return TryBuildStructureNearPylonInBase(ABILITY_ID::BUILD_FORGE, UNIT_TYPEID::PROTOSS_FORGE);
        }
        else {
            stage_number=228;
            return false;
        }
    case 228:
        for (const auto& s : stalkers) {
            if (s->build_progress<1.0f) {
                stage_number=230;
                return false;
            }
        }
        return TryWarpUnitPosition(ABILITY_ID::TRAINWARP_STALKER, front_expansion);
    /*case 229:
		if (robotics_facility_count == 0) {
			stage_number = 214;
			return false;
		}
        if (!observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY)).front()->orders.empty()) {
            if (observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY)).front()->orders.front().ability_id != ABILITY_ID::TRAIN_OBSERVER) {
                return false;
            }
            stage_number=230;
            return false;
        }
        if (TryBuildUnit(ABILITY_ID::TRAIN_OBSERVER, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_OBSERVER)) {
            stage_number=230;
            return false;
        }
        return false;*/
    case 230:
        if (gateway_count>4) {
			stage_number=231;
			return false;
		}
		return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_GATEWAY);
    case 231:
        if (robotics_bay_count>=1) {
            stage_number=232;
            return false;
        }
        return TryBuildStructureNearPylonInBase(ABILITY_ID::BUILD_ROBOTICSBAY, UNIT_TYPEID::PROTOSS_ROBOTICSBAY);
    case 232:
        if (robotics_facility_count>=2) {
            stage_number=233;
            return false;
        }
        return TryBuildStructureNearPylon(ABILITY_ID::BUILD_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);



    case 601:
		if (pylons.size()>0) {
            stage_number=602;
            return false;
		}
		return TryBuildStructureAtLocation(ABILITY_ID::BUILD_PYLON, UNIT_TYPEID::PROTOSS_PYLON, Pylon1);
	case 602:
		if (gateway_count>0) {
			stage_number=603;
			return false;
		}
		if (observation->GetMinerals()>150) {
            work_probe_forward = false;
		    return TryBuildStructureAtLocation(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_GATEWAY, Gate1);
        }
		return false;
	case 603:
	    TryChronoboost(base);
		if (assimilator_count>=2) {
			stage_number=604;
			return false;
		}
		if (observation->GetMinerals()>75) {
			return TryBuildGas(base->pos);
		}
		return false;
	case 604:
	    Actions()->UnitCommand(gateways,ABILITY_ID::RALLY_UNITS, startLocation_);
		if (cybernetics_count>0) {
			stage_number=605;
			return false;
		}
		return TryBuildStructureAtLocation(ABILITY_ID::BUILD_CYBERNETICSCORE, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE, Core1);
    case 605:
        if (pylons.size()>1) {
            stage_number=606;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_PYLON, UNIT_TYPEID::PROTOSS_PYLON, Pylon2);
    case 606:
        if (pylons.size()>2) {
            stage_number=607;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_PYLON, UNIT_TYPEID::PROTOSS_PYLON, Pylon3);
    case 608:
        if (stargate_count > 0) {
			stage_number=610;
			return false;
		}
		return TryBuildStructureAtLocation(ABILITY_ID::BUILD_STARGATE, UNIT_TYPEID::PROTOSS_STARGATE, Star1);
    case 607:
        if (tryadeptbranch6) {
            if (gateway_count == 0) {
                stage_number = 602;
                return false;
            }
            if (!gateways.front()->orders.empty()) {
                stage_number=608;
                return false;
            }
            return TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_ADEPT);
        }
        else {
            if (gateway_count == 0) {
                stage_number = 602;
                return false;
            }
            if (!gateways.front()->orders.empty()) {
                stage_number=608;
                return false;
            }
            return TryBuildUnit(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_STALKER);
        }
    case 610:
        if (battery_count>0) {
            stage_number=611;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_SHIELDBATTERY, UNIT_TYPEID::PROTOSS_SHIELDBATTERY, Batt1);
    case 611:
        if (battery_count>1) {
            stage_number=612;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_SHIELDBATTERY, UNIT_TYPEID::PROTOSS_SHIELDBATTERY, Batt2);
    case 612:
        if (tryadeptbranch6) {
            if (gateway_count == 0) {
                stage_number = 602;
                return false;
            }
            if (!gateways.front()->orders.empty() && num_adept>0) {
                stage_number=613;
                return false;
            }
            return TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_ADEPT);
        }
        else {
            if (gateway_count == 0) {
                stage_number = 602;
                return false;
            }
            if (!gateways.front()->orders.empty() && num_stalker>0) {
                stage_number=613;
                return false;
            }
            return TryBuildUnit(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_STALKER);
        }
    case 613:
		if (stargate_count == 0) {
			stage_number = 607;
			return false;
		}
        if (!stargates.front()->orders.empty()) {
            stage_number=614;
            return false;
        }
        return TryBuildUnit(ABILITY_ID::TRAIN_ORACLE, UNIT_TYPEID::PROTOSS_STARGATE, UNIT_TYPEID::PROTOSS_ORACLE);
    case 614:
        TryChronoboost(stargates.front());
        if (battery_count>2) {
            stage_number=615;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_SHIELDBATTERY, UNIT_TYPEID::PROTOSS_SHIELDBATTERY, Batt3);
    case 615:
        if (battery_count>3) {
            stage_number=616;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_SHIELDBATTERY, UNIT_TYPEID::PROTOSS_SHIELDBATTERY, Batt4);
   case 616:
		if (stargate_count == 0) {
			stage_number = 607;
			return false;
		}
        if (!stargates.front()->orders.empty()) {
            if (stargates.front()->orders.front().ability_id != ABILITY_ID::TRAIN_VOIDRAY) {
                return false;
            }
            stage_number=617;
            return false;
        }
        return TryBuildUnit(ABILITY_ID::TRAIN_VOIDRAY, UNIT_TYPEID::PROTOSS_STARGATE, UNIT_TYPEID::PROTOSS_VOIDRAY);
    case 617:
        if (battery_count>4) {
            stage_number=618;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_SHIELDBATTERY, UNIT_TYPEID::PROTOSS_SHIELDBATTERY, Batt5);
    case 618:
        if (pylons.size()>3) {
            stage_number=619;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_PYLON, UNIT_TYPEID::PROTOSS_PYLON, Pylon4);
    case 619:
		if (stargate_count == 0) {
			stage_number = 607;
			return false;
		}
        if (stargates.front()->orders.empty()) {
            return TryBuildUnit(ABILITY_ID::TRAIN_VOIDRAY, UNIT_TYPEID::PROTOSS_STARGATE, UNIT_TYPEID::PROTOSS_VOIDRAY);
        }
        if (num_voidray>1) {
            stage_number=620;
        }
        return false;
    case 620:
        if (stargate_count > 1) {
			stage_number=621;
			return false;
		}
        for (const auto& p : pylons) {
            if (Distance2D(p->pos,Pylon2)<15) {
                return TryBuildStructureNearPylon(ABILITY_ID::BUILD_STARGATE, UNIT_TYPEID::PROTOSS_STARGATE, p);
            }
        }
		return false;


    case 701:
        work_probe_forward = false;
        if (pylons.size()>0) {
            stage_number=702;
            return false;
		}
		return TryBuildStructureAtLocation(ABILITY_ID::BUILD_PYLON, UNIT_TYPEID::PROTOSS_PYLON, Pylon1);
    case 702:
        if (gateway_count>0) {
			stage_number=703;
			return false;
		}
		return TryBuildStructureAtLocation(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_GATEWAY, Gate1);
    case 703:
        TryChronoboost(base);
        if (assimilator_count>0) {
			stage_number=704;
			return false;
		}
		else if (TryBuildGas(base->pos)) {
            stage_number=704;
            return false;
		}
		return false;
    case 704:
        if (assimilator_count<1) {
            stage_number=703;
            return false;
        }
        if (bases.size()>=2) {
            stage_number=705;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_NEXUS, front_expansion);
    case 705:
        Actions()->UnitCommand(gateways,ABILITY_ID::RALLY_UNITS, Star1);
        if (cybernetics_count>0) {
			stage_number=706;
			return false;
		}
		return TryBuildStructureAtLocation(ABILITY_ID::BUILD_CYBERNETICSCORE, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE,Core1);
    case 706:
        //Actions()->UnitCommand(probe_forward, ABILITY_ID::MOVE, Pylon2);
        if (assimilator_count>=2) {
			stage_number=707;
			return false;
		}
		if (observation->GetMinerals()>75) {
			return TryBuildGas(base->pos);
		}
		return false;
    case 707:
        if (pylons.size()>1) {
            stage_number=708;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_PYLON, UNIT_TYPEID::PROTOSS_PYLON, Pylon2);
    case 708:
		if (gateway_count == 0) {
			stage_number = 702;
			return false;
		}
        if (!gateways.front()->orders.empty()) {
            stage_number=709;
            return false;
        }
        return TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_ADEPT);
    case 709:
        if (stargate_count > 0) {
			stage_number=710;
			return false;
		}
		return TryBuildStructureAtLocation(ABILITY_ID::BUILD_STARGATE, UNIT_TYPEID::PROTOSS_STARGATE, Star1);
    case 710:
		if (cybernetics_count == 0) {
			stage_number = 705;
			return false;
		}
        if (!cores.front()->orders.empty()) {
            stage_number=711;
            return false;
        }
        return TryBuildUpgrade(ABILITY_ID::RESEARCH_PROTOSSAIRWEAPONS, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE, UPGRADE_ID::PROTOSSAIRWEAPONSLEVEL1);
    case 711:
		if (gateway_count == 0) {
			stage_number = 702;
			return false;
		}
        if (!gateways.front()->orders.empty()) {
            if (gateways.front()->orders.front().progress>0.3f){
                return false;
            }
            stage_number=712;
            return false;
        }
        return TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_ADEPT);
    case 712:
        for (const auto& b : bases) {
            if (b==base) continue;
            TryChronoboost(b);
        }
        if (pylons.size()>2) {
            stage_number=713;
            return false;
        }
        return TryBuildPylon(staging_location_,7.0);
    case 713:
		if (stargate_count == 0) {
			stage_number = 709;
			return false;
		}
        if (!stargates.front()->orders.empty()) {
            stage_number=714;
            return false;
        }
        return TryBuildUnit(ABILITY_ID::TRAIN_ORACLE, UNIT_TYPEID::PROTOSS_STARGATE, UNIT_TYPEID::PROTOSS_ORACLE);
    case 714:
        TryChronoboost(IsUnit(UNIT_TYPEID::PROTOSS_STARGATE));
        if (battery_count>0) {
            stage_number=715;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_SHIELDBATTERY, UNIT_TYPEID::PROTOSS_SHIELDBATTERY, Batt1);
    case 715:
        if (battery_count>1) {
            stage_number=716;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_SHIELDBATTERY, UNIT_TYPEID::PROTOSS_SHIELDBATTERY, Batt2);
    case 716:
        if (pylons.size()>3) {
            stage_number=717;
            return false;
        }
        return TryBuildStructureAtLocation(ABILITY_ID::BUILD_PYLON, UNIT_TYPEID::PROTOSS_PYLON, Pylon3);
    case 717:
		if (stargate_count == 0) {
			stage_number = 709;
			return false;
		}
        if (!stargates.front()->orders.empty()) {
            if (stargates.front()->orders.front().ability_id!=ABILITY_ID::TRAIN_VOIDRAY) {
                return false;
            }
            stage_number=718;
            return false;
        }
        return TryBuildUnit(ABILITY_ID::TRAIN_VOIDRAY, UNIT_TYPEID::PROTOSS_STARGATE, UNIT_TYPEID::PROTOSS_VOIDRAY);
     case 718:
        if (assimilator_count>=4) {
			stage_number=719;
			return false;
		}
		for (const auto& b : bases) {
            TryBuildGas(b->pos);
		}
		return false;
     case 720:
        if (stargate_count == 0) {
            stage_number = 709;
            return false;
        }
        for (const auto& stargate : stargates) {
            if (stargate->build_progress<1.0f) {
                continue;
            }
            if (!stargate->orders.empty() && num_voidray>0) {
                stage_number=721;
                return false;
            }
        }
        return TryBuildUnit(ABILITY_ID::TRAIN_VOIDRAY, UNIT_TYPEID::PROTOSS_STARGATE, UNIT_TYPEID::PROTOSS_VOIDRAY);
     case 719:
        if (stargate_count > 1) {
			stage_number=720;
			return false;
		}
		return TryBuildStructureNearPylonInBase(ABILITY_ID::BUILD_STARGATE, UNIT_TYPEID::PROTOSS_STARGATE);
     case 721:
        if (pylons.size()>5) {
            stage_number=722;
            return false;
        }
        TryBuildPylon(staging_location_,15.0,3);
        TryBuildPylon(front_expansion,8.0,3);
        return TryBuildPylon(startLocation_,15.0,3);
     case 722:
        if (battery_count>2) {
            stage_number=723;
            return false;
        }
        for (const auto& p : pylons) {
            if (p->build_progress<1.0f) continue;
            if (Distance2D(p->pos, front_expansion)<12) {
                return TryBuildStructureNearPylon(ABILITY_ID::BUILD_SHIELDBATTERY, UNIT_TYPEID::PROTOSS_SHIELDBATTERY, p);
            }
        }
        return false;
     case 723:
        if (CountUnitType(UNIT_TYPEID::PROTOSS_FLEETBEACON) > 0) {
			stage_number=724;
			return false;
		}
		return TryBuildStructureNearPylonInBase(ABILITY_ID::BUILD_FLEETBEACON, UNIT_TYPEID::PROTOSS_FLEETBEACON);
    case 724:
        if (pylons.size()>6) {
            stage_number=725;
            return false;
        }
        return TryBuildPylon(staging_location_,10.0);
     case 725:
        for (const auto& stargate : stargates) {
            if (stargate->orders.empty()) {
                return TryBuildUnit(ABILITY_ID::TRAIN_VOIDRAY, UNIT_TYPEID::PROTOSS_STARGATE, UNIT_TYPEID::PROTOSS_VOIDRAY);
            }
        }
        if (num_voidray<2) {
            return false;
        }
        stage_number=726;
        return false;
     case 726:
        if (bases.size()>=3) {
            stage_number=727;
            return false;
        }
        return TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
     case 727:
        if (forge_count>0) {
            stage_number=728;
            return false;
        }
        return TryBuildStructureNearPylonInBase(ABILITY_ID::BUILD_FORGE, UNIT_TYPEID::PROTOSS_FORGE);
     case 728:
        for (const auto& stargate : stargates) {
            if (stargate->orders.empty()) {
                return TryBuildUnit(ABILITY_ID::TRAIN_CARRIER, UNIT_TYPEID::PROTOSS_STARGATE, UNIT_TYPEID::PROTOSS_CARRIER);
            }
            if (stargate->orders.front().ability_id != ABILITY_ID::TRAIN_CARRIER) {
                return false;
            }
        }
        stage_number=729;
        return false;
     case 729:
		if (cybernetics_count == 0) {
			stage_number = 705;
			return false;
		}
        if (cores.front()->orders.empty()) {
            return TryBuildUpgrade(ABILITY_ID::RESEARCH_PROTOSSAIRWEAPONS, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE, UPGRADE_ID::PROTOSSAIRWEAPONSLEVEL1);
        }
        if (cores.front()->orders.front().progress<0.5f) {
            stage_number=730;
        }
        return false;
     case 730:
        if (stargate_count > 2) {
			stage_number=731;
			return false;
		}
		return TryBuildStructureNearPylon(ABILITY_ID::BUILD_STARGATE, UNIT_TYPEID::PROTOSS_STARGATE);
     case 731:
		if (fleetbeacon_count == 0) {
			stage_number = 723;
			return false;
		}
        if (!observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_FLEETBEACON)).front()->orders.empty()) {
            stage_number=732;
            return false;
        }
        return TryBuildUpgrade(ABILITY_ID::RESEARCH_INTERCEPTORGRAVITONCATAPULT, UNIT_TYPEID::PROTOSS_FLEETBEACON, UPGRADE_ID::CARRIERLAUNCHSPEEDUPGRADE);
     case 732:
        for (const auto& b : bases) {
            if (b->orders.empty()) continue;
            if (b->orders.front().ability_id == ABILITY_ID::TRAIN_MOTHERSHIP) {
                stage_number=733;
                return false;
            }
        }
        return TryBuildUnit(ABILITY_ID::TRAIN_MOTHERSHIP, UNIT_TYPEID::PROTOSS_NEXUS, UNIT_TYPEID::PROTOSS_MOTHERSHIP);



    default:
        return false;



	}



	return false;
}
