#include "memibot.h"
#include <string.h>

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

	size_t observer_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_OBSERVER);
	size_t immortal_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_IMMORTAL);
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
	size_t carrier_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_CARRIER);
	size_t tempest_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_TEMPEST);


#ifdef DEBUG
	std::cout << "branch : " << branch << ", stage : "<< stage_number << std::endl;
#endif

	if (bases.size()<3) {
        if (observation->GetFoodWorkers()<50) {
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

	if (stage_number > 206 && stage_number <= 232) {
		TryBuildUnit(ABILITY_ID::TRAIN_STALKER, UNIT_TYPEID::PROTOSS_GATEWAY, UNIT_TYPEID::PROTOSS_STALKER);
		TryWarpUnitPrism(ABILITY_ID::TRAINWARP_STALKER);
	}

	if (stage_number>218) {
		if (immortal_count<1) TryBuildUnit(ABILITY_ID::TRAIN_IMMORTAL, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_IMMORTAL);
		else if(observer_count<3) TryBuildUnit(ABILITY_ID::TRAIN_OBSERVER, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_OBSERVER);
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
				RealChat("Bot is expanding Nexus");
                return TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
            }
        }

        TryBuildCannonNexus();
        if (bases.size()*2>assimilator_count+1) {
            TryBuildAssimilator();
        }



        if (gateway_count<bases.size()*2-1) {
			RealChat("Bot's Gateway : " + std::to_string(gateway_count));
            TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_GATEWAY);
        }

        if (warpprisms_phasing.empty()) {
            TryBuildArmyBranch5();
        }
        else {
            TryWarpUnitPrism(ABILITY_ID::TRAINWARP_ZEALOT);
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
		else {
			return false;
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
            stage_number =200;
			//RealChat("Bot의 전략은 Protoss입니다.\nBot의 전략은 점멸추적자 전략입니다.\n이 전략의 특징은 점멸을 활용해 초반 유닛인 추적자를 효율적으로 활용하는 무상성 전략입니다.");
        }
		return false;
	//branch 5
	case 200:
		RealChat("Bot's Race : Protoss");
		RealChat("Bot's Strategy : Blink Stalkers");
		RealChat("A powerful strategy for using STALKERs with BLINK skill efficiently");
		if (pylons.size()>0) {
            stage_number=201;
            return false;
		}
		return TrybuildFirstPylon();
	case 201:
		if (gateway_count>0) {
			stage_number=202;
			RealChat("Bot's Gateway : " + std::to_string(gateway_count));
			return false;
		}
		return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_GATEWAY);
	case 202:
		//TryChronoboost(base);
		if (assimilator_count>=1) {
			stage_number=203;
			return false;
		}
		if (observation->GetFoodWorkers()>=18 && TryBuildGas(base->pos)) {
            stage_number=203;
            return false;
		}
		return false;
    case 203:
        if (bases.size()>=2) {
			RealChat("Bot is expanding Nexus in front expansion");
            stage_number=204;
            return false;
        }
		RealChat("Bot is expanding Nexus in front expansion");
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
			stage_number=209;
			return false;
		}
		return TryBuildGas(base->pos);
    case 209:
        if (twilight_council_count>0) {
            for (const auto& b : bases) {
                if (b!=base) {
                    TryChronoboost(b);
                }
            }
            stage_number=211;
            return false;
        }
        return TryBuildStructureNearPylonInBase(ABILITY_ID::BUILD_TWILIGHTCOUNCIL, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
    case 211:
		RealChat("Bot is trying to upgrade BLINK");
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
		stage_number = 214;
		return false;
	case 214:
        if (robotics_facility_count>=1) {
			RealChat("Bot's Robotics : " + std::to_string(robotics_facility_count));
            stage_number=215;
            return false;
        }
        return TryBuildStructureNearPylon(ABILITY_ID::BUILD_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);
    case 215:
        if (gateway_count>2) {
			stage_number=217;
			RealChat("Bot's Gateway : " + std::to_string(gateway_count));
			return false;
		}
		return TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY,UNIT_TYPEID::PROTOSS_GATEWAY);
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
        stage_number=220;
		return false;
    case 220:
        if (bases.size()>=3) {
			RealChat("Bot is expanding third Nexus");
            stage_number=225;
            return false;
        }
		RealChat("Bot is expanding third Nexus");
        return TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
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
		stage_number = 227;
		return false;
	case 227:
        if (forge_count<2) {
            return TryBuildStructureNearPylonInBase(ABILITY_ID::BUILD_FORGE, UNIT_TYPEID::PROTOSS_FORGE);
        }
        stage_number=230;
        return false;
    case 230:
        if (gateway_count>4) {
			RealChat("Bot's Gateway : " + std::to_string(gateway_count));
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
			RealChat("Bot's Robotics : " + std::to_string(robotics_facility_count));
            stage_number=233;
            return false;
        }
        return TryBuildStructureNearPylon(ABILITY_ID::BUILD_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);

    default:
        return false;

	}



	return false;
}
