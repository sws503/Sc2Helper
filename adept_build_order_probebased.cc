#include <sc2api/sc2_api.h>
#include "sc2lib/sc2_lib.h"
#include <sc2utils/sc2_manage_process.h>
#include "balance_unit.h"

#include "bot_examples.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <random>
#include <iterator>
#include <typeinfo>
#include <map>

using namespace sc2;

class Bot : public Agent {
public:
    virtual void OnGameStart() final {
        game_info_ = Observation()->GetGameInfo();
        std::cout << "Game started!" << std::endl;
        expansions_ = search::CalculateExpansionLocations(Observation(), Query());
        iter_exp = expansions_.begin();

        //Temporary, we can replace this with observation->GetStartLocation() once implemented
        startLocation_ = Observation()->GetStartLocation();
        staging_location_ = startLocation_;

        //bool find_enemy_location=false;
        if(game_info_.enemy_start_locations.size()==1) find_enemy_location=true;
        //2인용맵이면 정찰 필요X
        //std::vector<Point2D> enemy_start_locations(game_info_.enemy_start_locations);
        //std::cout<<game_info_.enemy_start_locations.size()<<std::endl;
        //iter=game_info_.enemy_start_locations.erase(iter);
        //std::cout<<game_info_.enemy_start_locations.size()<<std::endl;
        //enemy_start_locations(game_info_.enemy_start_locations);
    }

    virtual void OnStep() final {

        const ObservationInterface* observation = Observation();
        Units units = observation->GetUnits(Unit::Self, IsArmy(observation));
        ConvertGateWayToWarpGate();

        ManageWorkers(UNIT_TYPEID::PROTOSS_PROBE, ABILITY_ID::HARVEST_GATHER, UNIT_TYPEID::PROTOSS_ASSIMILATOR);

        if(!adept_scout.first || !adept_scout.second){
            scoutadept();
        }

        if(iter_exp < expansions_.end() && find_enemy_location == true){
            scoutprobe();
        }

        if(early_strategy==false){
            EarlyStrategy();
            return;
        }

        //Throttle some behavior that can wait to avoid duplicate orders.

        ManageAdept();

        if (TryWarpAdept())
            return;

        if (TryBuildPylon())
            return;

    }

    virtual void OnUnitIdle(const Unit* unit) override {
        switch (unit->unit_type.ToType()) {
            case UNIT_TYPEID::PROTOSS_PROBE: {
                MineIdleWorkers(unit, ABILITY_ID::HARVEST_GATHER, UNIT_TYPEID::PROTOSS_ASSIMILATOR);
                break;
            }
            case UNIT_TYPEID::PROTOSS_CYBERNETICSCORE: {
                const ObservationInterface* observation = Observation();
                Units nexus = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));

                if (!warpgate_reasearched_) {
                    Actions()->UnitCommand(unit, ABILITY_ID::RESEARCH_WARPGATE);
                    if (!nexus.empty()) {
                        Actions()->UnitCommand(nexus.front(), ABILITY_ID::EFFECT_TIMEWARP, unit);
                    }
                }
            }
            default: {
                break;
            }
        }
        return;
    }

    void OnUpgradeCompleted(UpgradeID upgrade) {
        switch (upgrade.ToType()) {
            case UPGRADE_ID::WARPGATERESEARCH: {
                warpgate_reasearched_ = true;
            }
            case UPGRADE_ID::BLINKTECH: {
                blink_reasearched_ = true;
            }
            case UPGRADE_ID::ADEPTPIERCINGATTACK: {
                ADEPTRESONATINGGLAIVES = true;
            }
            default:
                break;
        }
    }

    int max_worker_count_ = 70;

    //When to start building attacking units
    int target_worker_count_ = 15;
    GameInfo game_info_;
    std::vector<Point3D> expansions_;
    Point3D startLocation_;
    Point3D staging_location_;



private:
    struct IsAttackable {
        bool operator()(const Unit& unit) {
            switch (unit.unit_type.ToType()) {
                case UNIT_TYPEID::ZERG_OVERLORD: return false;
                case UNIT_TYPEID::ZERG_OVERSEER: return false;
                case UNIT_TYPEID::PROTOSS_OBSERVER: return false;
                default: return true;
            }
        }
    };

    struct IsFlying {
        bool operator()(const Unit& unit) {
            return unit.is_flying;
        }
    };

    struct IsWorker {
         bool operator()(const Unit& unit) {
            switch (unit.unit_type.ToType()) {
                case UNIT_TYPEID::PROTOSS_PROBE: return true;
                case UNIT_TYPEID::ZERG_DRONE: return true;
                case UNIT_TYPEID::TERRAN_SCV: return true;
                default: return false;
            }
        }
    };

    struct IsArmy {
        IsArmy(const ObservationInterface* obs) : observation_(obs) {}

        bool operator()(const Unit& unit) {
            auto attributes = observation_->GetUnitTypeData().at(unit.unit_type).attributes;
            for (const auto& attribute : attributes) {
                if (attribute == Attribute::Structure) {
                    return false;
                }
            }
            switch (unit.unit_type.ToType()) {
                case UNIT_TYPEID::ZERG_OVERLORD: return false;
                case UNIT_TYPEID::PROTOSS_PROBE: return false;
                case UNIT_TYPEID::ZERG_DRONE: return false;
                case UNIT_TYPEID::TERRAN_SCV: return false;
                case UNIT_TYPEID::ZERG_QUEEN: return false;
                case UNIT_TYPEID::ZERG_LARVA: return false;
                case UNIT_TYPEID::ZERG_EGG: return false;
                case UNIT_TYPEID::TERRAN_MULE: return false;
                case UNIT_TYPEID::TERRAN_NUKE: return false;
                default: return true;
            }
        }

        const ObservationInterface* observation_;
    };


    struct IsStructure {
        IsStructure(const ObservationInterface* obs) : observation_(obs) {};

        bool operator()(const Unit& unit) {
            auto& attributes = observation_->GetUnitTypeData().at(unit.unit_type).attributes;
            bool is_structure = false;
            for (const auto& attribute : attributes) {
                if (attribute == Attribute::Structure) {
                    is_structure = true;
                }
            }
            return is_structure;
        }

        const ObservationInterface* observation_;
    };

    struct IsTownHall {
        bool operator()(const Unit& unit) {

            switch (unit.unit_type.ToType()) {
                case UNIT_TYPEID::ZERG_HATCHERY: return true;
                case UNIT_TYPEID::ZERG_LAIR: return true;
                case UNIT_TYPEID::ZERG_HIVE : return true;
                case UNIT_TYPEID::TERRAN_COMMANDCENTER: return true;
                case UNIT_TYPEID::TERRAN_ORBITALCOMMAND: return true;
                case UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING: return true;
                case UNIT_TYPEID::TERRAN_PLANETARYFORTRESS: return true;
                case UNIT_TYPEID::PROTOSS_NEXUS: return true;
                default: return false;
            }
        }
    };

    struct IsVespeneGeyser {
        bool operator()(const Unit& unit) {
            switch (unit.unit_type.ToType()) {
                case UNIT_TYPEID::NEUTRAL_VESPENEGEYSER: return true;
                case UNIT_TYPEID::NEUTRAL_SPACEPLATFORMGEYSER: return true;
                case UNIT_TYPEID::NEUTRAL_PROTOSSVESPENEGEYSER: return true;
                case UNIT_TYPEID::NEUTRAL_PURIFIERVESPENEGEYSER:    return true;
                case UNIT_TYPEID::NEUTRAL_RICHVESPENEGEYSER:    return true;
                case UNIT_TYPEID::NEUTRAL_SHAKURASVESPENEGEYSER:    return true;
                default: return false;
            }
        }
    };

    size_t CountUnitType(const ObservationInterface* observation, UnitTypeID unit_type) {
        return observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
    }

    size_t CountUnitTypeBuilding(const ObservationInterface* observation, UNIT_TYPEID production_building, ABILITY_ID ability) {
        int building_count = 0;
        Units buildings = observation->GetUnits(Unit::Self, IsUnit(production_building));

        for (const auto& building : buildings) {
            if (building->orders.empty()) {
                continue;
            }

            for (const auto order : building->orders) {
                if (order.ability_id == ability) {
                    building_count++;
                }
            }
        }

        return building_count;
    }

    size_t CountUnitTypeTotal(const ObservationInterface* observation, UNIT_TYPEID unit_type, UNIT_TYPEID production, ABILITY_ID ability) {
        return CountUnitType(observation, unit_type) + CountUnitTypeBuilding(observation, production, ability);
    }

    size_t CountUnitTypeTotal(const ObservationInterface* observation, std::vector<UNIT_TYPEID> unit_type, UNIT_TYPEID production, ABILITY_ID ability) {
        size_t count = 0;
        for (const auto& type : unit_type) {
            count += CountUnitType(observation, type);
        }
        return count + CountUnitTypeBuilding(observation, production, ability);
    }

    bool FindEnemyStructure(const ObservationInterface* observation, const Unit*& enemy_unit) {
        Units my_units = observation->GetUnits(Unit::Alliance::Enemy);
        for (const auto unit : my_units) {
            if (unit->unit_type == UNIT_TYPEID::TERRAN_COMMANDCENTER ||
                unit->unit_type == UNIT_TYPEID::TERRAN_SUPPLYDEPOT ||
                unit->unit_type == UNIT_TYPEID::TERRAN_BARRACKS) {
                enemy_unit = unit;
                return true;
            }
        }

        return false;
    }

    bool GetRandomUnit(const Unit*& unit_out, const ObservationInterface* observation, UnitTypeID unit_type) {
        Units my_units = observation->GetUnits(Unit::Alliance::Self);
        std::random_shuffle(my_units.begin(), my_units.end()); // Doesn't work, or doesn't work well.
        for (const auto unit : my_units) {
            if (unit->unit_type == unit_type) {
                unit_out = unit;
                return true;
            }
        }
        return false;
    }



    const Unit* FindNearestMineralPatch(const Point2D& start) {
        Units units = Observation()->GetUnits(Unit::Alliance::Neutral);
        float distance = std::numeric_limits<float>::max();
        const Unit* target = nullptr;
        for (const auto& u : units) {
            if (u->unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD) {
                float d = DistanceSquared2D(u->pos, start);
                if (d < distance) {
                    distance = d;
                    target = u;
                }
            }
        }
        //If we never found one return false;
        if (distance == std::numeric_limits<float>::max()) {
            return target;
        }
        return target;
    }

    bool FindEnemyPosition(Point2D& target_pos) {
        if (game_info_.enemy_start_locations.empty()) {
            return false;
        }
        target_pos = game_info_.enemy_start_locations.front();
        return true;
    }

    bool TryFindRandomPathableLocation(const Unit* unit, Point2D& target_pos) {
        // First, find a random point inside the playable area of the map.
        float playable_w = game_info_.playable_max.x - game_info_.playable_min.x;
        float playable_h = game_info_.playable_max.y - game_info_.playable_min.y;

        // The case where game_info_ does not provide a valid answer
        if (playable_w == 0 || playable_h == 0) {
            playable_w = 236;
            playable_h = 228;
        }

        target_pos.x = playable_w * GetRandomFraction() + game_info_.playable_min.x;
        target_pos.y = playable_h * GetRandomFraction() + game_info_.playable_min.y;

        // Now send a pathing query from the unit to that point. Can also query from point to point,
        // but using a unit tag wherever possible will be more accurate.
        // Note: This query must communicate with the game to get a result which affects performance.
        // Ideally batch up the queries (using PathingDistanceBatched) and do many at once.
        float distance = Query()->PathingDistance(unit, target_pos);

        return distance > 0.1f;
    }

    int GetExpectedWorkers(UNIT_TYPEID vespene_building_type) {
        const ObservationInterface* observation = Observation();
        Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
        Units geysers = observation->GetUnits(Unit::Alliance::Self, IsUnit(vespene_building_type));
        int expected_workers = 0;
        for (const auto& base : bases) {
            if (base->build_progress != 1) {
                continue;
            }
            expected_workers += base->ideal_harvesters;
        }

        for (const auto& geyser : geysers) {
            if (geyser->vespene_contents > 0) {
                if (geyser->build_progress != 1) {
                    continue;
                }
                expected_workers += geyser->ideal_harvesters;
            }
        }

        return expected_workers;
    }

    bool TryBuildUnit(AbilityID ability_type_for_unit, UnitTypeID unit_type) {
        const ObservationInterface* observation = Observation();

        //If we are at supply cap, don't build anymore units, unless its an overlord.
        if (observation->GetFoodUsed() >= observation->GetFoodCap() && ability_type_for_unit != ABILITY_ID::TRAIN_OVERLORD) {
            return false;
        }
        const Unit* unit = nullptr;
        if (!GetRandomUnit(unit, observation, unit_type)) {
            return false;
        }
        if (!unit->orders.empty()) {
            return false;
        }

        if (unit->build_progress != 1) {
            return false;
        }

        Actions()->UnitCommand(unit, ability_type_for_unit);
        return true;
    }
    //미리 대기열 등
    bool TryBuildUnit2(AbilityID ability_type_for_unit, UnitTypeID unit_type) {
        const ObservationInterface* observation = Observation();

        //If we are at supply cap, don't build anymore units, unless its an overlord.
        if (observation->GetFoodUsed() >= observation->GetFoodCap() && ability_type_for_unit != ABILITY_ID::TRAIN_OVERLORD) {
            return false;
        }
        const Unit* unit = nullptr;
        if (!GetRandomUnit(unit, observation, unit_type)) {
            return false;
        }
        if (unit->build_progress != 1) {
            return false;
        }

        Actions()->UnitCommand(unit, ability_type_for_unit);
        return true;
    }

    void MineIdleWorkers(const Unit* worker, AbilityID worker_gather_command, UnitTypeID vespene_building_type) {
        if(worker==advance_probe) return;
        const ObservationInterface* observation = Observation();
        Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
        Units geysers = observation->GetUnits(Unit::Alliance::Self, IsUnit(vespene_building_type));

        const Unit* valid_mineral_patch = nullptr;

        if (bases.empty()) {
            return;
        }

        for (const auto& geyser : geysers) {
            if (geyser->assigned_harvesters < geyser->ideal_harvesters) {
                Actions()->UnitCommand(worker, worker_gather_command, geyser);
                return;
            }
        }
        //Search for a base that is missing workers.
        for (const auto& base : bases) {
            //If we have already mined out here skip the base.
            if (base->ideal_harvesters == 0 || base->build_progress != 1) {
                continue;
            }
            if (base->assigned_harvesters < base->ideal_harvesters) {
                valid_mineral_patch = FindNearestMineralPatch(base->pos);
                Actions()->UnitCommand(worker, worker_gather_command, valid_mineral_patch);
                return;
            }
        }

        if (!worker->orders.empty()) {
            return;
        }

        //If all workers are spots are filled just go to any base.
        const Unit* random_base = GetRandomEntry(bases);
        valid_mineral_patch = FindNearestMineralPatch(random_base->pos);
        Actions()->UnitCommand(worker, worker_gather_command, valid_mineral_patch);
    }

    void ManageWorkers(UNIT_TYPEID worker_type, AbilityID worker_gather_command, UNIT_TYPEID vespene_building_type) {
        const ObservationInterface* observation = Observation();
        Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
        Units geysers = observation->GetUnits(Unit::Alliance::Self, IsUnit(vespene_building_type));

        if (bases.empty()) {
            return;
        }

        for (const auto& base : bases) {
            //If we have already mined out or still building here skip the base.
            if (base->ideal_harvesters == 0 || base->build_progress != 1) {
                continue;
            }
            //if base is
            if (base->assigned_harvesters > base->ideal_harvesters) {
                Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(worker_type));

                for (const auto& worker : workers) {
                    if (!worker->orders.empty()) {
                        if (worker->orders.front().target_unit_tag == base->tag) {
                            //This should allow them to be picked up by mineidleworkers()
                            MineIdleWorkers(worker, worker_gather_command,vespene_building_type);
                            return;
                        }
                    }
                }
            }
        }
        Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(worker_type));

        for (const auto& geyser : geysers) {
            if (geyser->ideal_harvesters == 0 || geyser->build_progress != 1) {
                continue;
            }
            if (geyser->assigned_harvesters > geyser->ideal_harvesters) {
                for (const auto& worker : workers) {
                    if (!worker->orders.empty()) {
                        if (worker->orders.front().target_unit_tag == geyser->tag) {
                            //This should allow them to be picked up by mineidleworkers()
                            MineIdleWorkers(worker, worker_gather_command, vespene_building_type);
                            return;
                        }
                    }
                }
            }
            else if (geyser->assigned_harvesters < geyser->ideal_harvesters) {
                for (const auto& worker : workers) {
                    if (!worker->orders.empty()) {
                        //This should move a worker that isn't mining gas to gas
                        const Unit* target = observation->GetUnit(worker->orders.front().target_unit_tag);
                        if (target == nullptr) {
                            continue;
                        }
                        if (target->unit_type != vespene_building_type) {
                            //This should allow them to be picked up by mineidleworkers()
                            MineIdleWorkers(worker, worker_gather_command, vespene_building_type);
                            return;
                        }
                    }
                }
            }
        }
    }

    void RetreatWithUnits(UnitTypeID unit_type, Point2D retreat_position) {
        const ObservationInterface* observation = Observation();
        Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
        for (const auto& unit : units) {
            RetreatWithUnit(unit, retreat_position);
        }
    }

    void RetreatWithUnit(const Unit* unit, Point2D retreat_position) {
        float dist = Distance2D(unit->pos, retreat_position);

        if (dist < 10) {
            if (unit->orders.empty()) {
                return;
            }
            Actions()->UnitCommand(unit, ABILITY_ID::STOP);
            return;
        }

        if (unit->orders.empty() && dist > 14) {
            Actions()->UnitCommand(unit, ABILITY_ID::MOVE, retreat_position);
        }
        else if (!unit->orders.empty() && dist > 14) {
            if (unit->orders.front().ability_id != ABILITY_ID::MOVE) {
                Actions()->UnitCommand(unit, ABILITY_ID::MOVE, retreat_position);
            }
        }
    }

    bool TryBuildProbe() {
        const ObservationInterface* observation = Observation();
        Units bases = observation->GetUnits(Unit::Alliance::Self,IsTownHall());

        if(observation->GetFoodWorkers() >= max_worker_count_) {
            return false;
        }
        if(observation->GetFoodUsed() >= observation->GetFoodCap()) {
            return false;
        }
        if(observation->GetFoodWorkers() > GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR)){
            return false;
        }

        for (const auto& base : bases) {
            if(base->assigned_harvesters < base->ideal_harvesters && base->build_progress == 1) {
                if(observation->GetMinerals()>=50){
                    return TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
                }
            }
        }
        return false;
    }

    bool TryBuildStructure(AbilityID ability_type_for_structure, UnitTypeID unit_type, Point2D location, bool isExpansion = false) {

        const ObservationInterface* observation = Observation();
        Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));

        //if we have no workers Don't build
        if (workers.empty()) {
            return false;
        }

        // Check to see if there is already a worker heading out to build it
        for (const auto& worker : workers) {
            for (const auto& order : worker->orders) {
                if (order.ability_id == ability_type_for_structure) {
                    return false;
                }
            }
        }



        // If no worker is already building one, get a random worker to build one
        const Unit* unit = nullptr;
        for (const auto& worker : workers) {
            //전진 프로브는 제외
            if (worker == advance_probe||worker == probe_scout) continue;
            for (const auto& order : worker->orders) {
                if(order.ability_id == ABILITY_ID::HARVEST_GATHER){
                    unit = worker;
                    break;
                }
            }
            if(unit!=nullptr) break;
        }
        if(unit==nullptr) return false;

        // Check to see if unit can make it there
        if (Query()->PathingDistance(unit, location) < 0.1f) {
            return false;
        }
        if (!isExpansion) {
            for (const auto& expansion : expansions_) {
                if (Distance2D(location, Point2D(expansion.x, expansion.y)) < 7) {
                    return false;
                }
            }
        }
        // Check to see if unit can build there
        if (Query()->Placement(ability_type_for_structure, location)) {
            Actions()->UnitCommand(unit, ability_type_for_structure, location);
            return true;
        }
        return false;

    }

    bool TryBuildStructure(AbilityID ability_type_for_structure, const Unit* unit, Point2D location) {

        const ObservationInterface* observation = Observation();

        if(unit==nullptr) return false;

        // Check to see if unit can make it there
        /*std::cout<<Query()->PathingDistance(unit, location)<<std::endl;
        if (Query()->PathingDistance(unit, location) < 0.1f) {
            return false;
        }*/

        // Check to see if unit can build there
        if (Query()->Placement(ability_type_for_structure, location)) {
            Actions()->UnitCommand(unit, ability_type_for_structure, location);
            return true;
        }
        return false;

    }

    bool TryBuildStructure(AbilityID ability_type_for_structure, UnitTypeID unit_type, Tag location_tag) {
        const ObservationInterface* observation = Observation();
        Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
        const Unit* target = observation->GetUnit(location_tag);

        if (workers.empty()) {
            return false;
        }

        // Check to see if there is already a worker heading out to build it
        for (const auto& worker : workers) {
            for (const auto& order : worker->orders) {
                if (order.ability_id == ability_type_for_structure) {
                    return false;
                }
            }
        }

        // If no worker is already building one, get a random worker to build one
        const Unit* unit = nullptr;
        for (const auto& worker : workers) {
            //전진 프로브는 제외
            if (worker == advance_probe||worker == probe_scout) continue;
            for (const auto& order : worker->orders) {
                if(order.ability_id == ABILITY_ID::HARVEST_GATHER){
                    unit = worker;
                    break;
                }
            }
            if(unit!=nullptr) break;
        }
        if(unit==nullptr) return false;

        // Check to see if unit can build there
        if (Query()->Placement(ability_type_for_structure, target->pos)) {
            Actions()->UnitCommand(unit, ability_type_for_structure, target);

            return true;
        }
        return false;

    }

    bool TryBuildStructureRandom(AbilityID ability_type_for_structure, UnitTypeID unit_type) {
        float rx = GetRandomScalar();
        float ry = GetRandomScalar();
        Point2D build_location = Point2D(staging_location_.x + rx * 15, staging_location_.y + ry * 15);

        Units units = Observation()->GetUnits(Unit::Self, IsStructure(Observation()));
        float distance = std::numeric_limits<float>::max();
        for (const auto& u : units) {
            if (u->unit_type == UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED) {
                continue;
            }
            float d = Distance2D(u->pos, build_location);
            if (d < distance) {
                distance = d;
            }
        }
        if (distance < 6) {
            return false;
        }
        return TryBuildStructure(ability_type_for_structure, unit_type, build_location);
    }



    bool TryBuildPylon() {
        const ObservationInterface* observation = Observation();

        // If we are not supply capped, don't build a supply depot.
        if (observation->GetFoodUsed() < observation->GetFoodCap() - 6) {
            return false;
        }

        if (observation->GetMinerals() < 100) {
            return false;
        }

        //check to see if there is already on building
        Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PYLON));
        if (observation->GetFoodUsed() < 40) {
            for (const auto& unit : units) {
                if (unit->build_progress != 1) {
                        return false;
                }
            }
        }
        if(observation->GetFoodUsed() < observation->GetFoodCap() - 15){
            return false;
        }

        // Try and build a pylon. Find a random Probe and give it the order.
        float rx = GetRandomScalar();
        float ry = GetRandomScalar();
        Point2D build_location = Point2D(staging_location_.x + rx * 15, staging_location_.y + ry * 15);
        return TryBuildStructure(ABILITY_ID::BUILD_PYLON, UNIT_TYPEID::PROTOSS_PROBE, build_location);
    }


    bool TryBuildAssimilator() {
        const ObservationInterface* observation = Observation();
        Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());

        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_ASSIMILATOR) >= observation->GetUnits(Unit::Alliance::Self, IsTownHall()).size() * 2) {
            return false;
        }

        for (const auto& base : bases) {
            if (base->assigned_harvesters >= base->ideal_harvesters) {
                if (base->build_progress == 1) {
                    if (TryBuildGas(ABILITY_ID::BUILD_ASSIMILATOR, UNIT_TYPEID::PROTOSS_PROBE, base->pos)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    bool TryBuildGas(AbilityID build_ability, UnitTypeID worker_type, Point2D base_location) {
        const ObservationInterface* observation = Observation();
        Units geysers = observation->GetUnits(Unit::Alliance::Neutral, IsVespeneGeyser());

        //only search within this radius
        float minimum_distance = 15.0f;
        Tag closestGeyser = 0;
        for (const auto& geyser : geysers) {
            float current_distance = Distance2D(base_location, geyser->pos);
            if (current_distance < minimum_distance) {
                if (Query()->Placement(build_ability, geyser->pos)) {
                    minimum_distance = current_distance;
                    closestGeyser = geyser->tag;
                }
            }
        }

        // In the case where there are no more available geysers nearby
        if (closestGeyser == 0) {
            return false;
        }
        return TryBuildStructure(build_ability, worker_type, closestGeyser);
    }


    bool TryGatherGeyser(UnitTypeID unit_type) {

        const ObservationInterface* observation = Observation();
        Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
        Units geysers = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_REFINERY));

        if (workers.empty()||geysers.empty()) {
            return false;
        }

        const Unit* unit = GetRandomEntry(workers);
        const Unit* geyser = GetRandomEntry(geysers);

        //std::cout<<geyser->ideal_harvesters<<std::endl;
        for (const auto& geyser : geysers) {
            if(geyser->assigned_harvesters<geyser->ideal_harvesters-1){
                Actions()->UnitCommand(unit,ABILITY_ID::HARVEST_GATHER, geyser);
                return true;
            }
        }

        return false;
    }


    void AttackWithUnitType(UnitTypeID unit_type, const ObservationInterface* observation) {
        Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
        for (const auto& unit : units) {
            AttackWithUnit(unit, observation);
        }
    }

    void AttackWithUnit(const Unit* unit, const ObservationInterface* observation) {
        Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
        if (enemy_units.empty()) {
            return;
        }

        if (unit->orders.empty()) {
            Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front()->pos);
            return;
        }

        if (unit->orders.front().ability_id != ABILITY_ID::ATTACK) {
            Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front()->pos);
        }
    }

    void ScoutWithUnits(UnitTypeID unit_type, const ObservationInterface* observation) {
        Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
        for (const auto& unit : units) {
            ScoutWithUnit(unit, observation);
        }
    }

    void ScoutWithUnit(const Unit* unit, const ObservationInterface* observation) {
        Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy, IsAttackable());
        if (!unit->orders.empty()) {
            return;
        }
        Point2D target_pos;

        if (FindEnemyPosition(target_pos)) {
            if (Distance2D(unit->pos, target_pos) < 20 && enemy_units.empty()) {
                if (TryFindRandomPathableLocation(unit, target_pos)) {
                    Actions()->UnitCommand(unit, ABILITY_ID::SMART, target_pos);
                    return;
                }
            }
            else if (!enemy_units.empty())
            {
                Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front());
                return;
            }
            Actions()->UnitCommand(unit, ABILITY_ID::SMART, target_pos);
        }
        else {
            if (TryFindRandomPathableLocation(unit, target_pos)) {
                Actions()->UnitCommand(unit, ABILITY_ID::SMART, target_pos);
            }
        }
    }

    void ManageUpgrades() {
        const ObservationInterface* observation = Observation();
        auto upgrades = observation->GetUpgrades();
        size_t base_count = observation->GetUnits(Unit::Alliance::Self, IsTownHall()).size();
        if (upgrades.empty()) {
            TryBuildUnit(ABILITY_ID::RESEARCH_WARPGATE, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
        }
        else {
            for (const auto& upgrade : upgrades) {
                if (air_build_) {
                    if (upgrade == UPGRADE_ID::PROTOSSAIRWEAPONSLEVEL1 && base_count > 2) {
                        TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRWEAPONS, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
                    }
                    else if (upgrade == UPGRADE_ID::PROTOSSAIRARMORSLEVEL1 && base_count > 2) {
                        TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRARMOR, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
                    }
                    else if (upgrade == UPGRADE_ID::PROTOSSSHIELDSLEVEL1 && base_count > 2) {
                        TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSSHIELDS, UNIT_TYPEID::PROTOSS_FORGE);
                    }
                    else if (upgrade == UPGRADE_ID::PROTOSSAIRARMORSLEVEL2 && base_count > 3) {
                        TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRARMOR, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
                    }
                    else if (upgrade == UPGRADE_ID::PROTOSSAIRWEAPONSLEVEL2 && base_count > 2) {
                        TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRWEAPONS, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
                    }
                    else if (upgrade == UPGRADE_ID::PROTOSSSHIELDSLEVEL2 && base_count > 2) {
                        TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSSHIELDS, UNIT_TYPEID::PROTOSS_FORGE);
                    }
                }
                if (upgrade == UPGRADE_ID::PROTOSSGROUNDWEAPONSLEVEL1 && base_count > 2) {
                    TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSGROUNDWEAPONS, UNIT_TYPEID::PROTOSS_FORGE);
                }
                else if (upgrade == UPGRADE_ID::PROTOSSGROUNDARMORSLEVEL1 && base_count > 2) {
                    TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSGROUNDARMOR, UNIT_TYPEID::PROTOSS_FORGE);
                }
                else if (upgrade == UPGRADE_ID::PROTOSSGROUNDWEAPONSLEVEL2 && base_count > 3) {
                    TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSGROUNDWEAPONS, UNIT_TYPEID::PROTOSS_FORGE);
                }
                else if (upgrade == UPGRADE_ID::PROTOSSGROUNDARMORSLEVEL2 && base_count > 3) {
                    TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSGROUNDARMOR, UNIT_TYPEID::PROTOSS_FORGE);
                }
                else {
                    if (air_build_) {
                        TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRARMOR, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
                        TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRWEAPONS, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
                        TryBuildUnit(ABILITY_ID::RESEARCH_ADEPTRESONATINGGLAIVES, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
                        TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSSHIELDS, UNIT_TYPEID::PROTOSS_FORGE);
                        TryBuildUnit(ABILITY_ID::RESEARCH_INTERCEPTORGRAVITONCATAPULT, UNIT_TYPEID::PROTOSS_FLEETBEACON);
                    }
                    else {
                        TryBuildUnit(ABILITY_ID::RESEARCH_EXTENDEDTHERMALLANCE, UNIT_TYPEID::PROTOSS_ROBOTICSBAY);
                        TryBuildUnit(ABILITY_ID::RESEARCH_BLINK, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
                        TryBuildUnit(ABILITY_ID::RESEARCH_CHARGE, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
                        TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSGROUNDWEAPONS, UNIT_TYPEID::PROTOSS_FORGE);
                        TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSGROUNDARMOR, UNIT_TYPEID::PROTOSS_FORGE);
                    }

                }
            }
        }
    }


    bool TryExpand(AbilityID build_ability, UnitTypeID worker_type) {
        const ObservationInterface* observation = Observation();
        float minimum_distance = std::numeric_limits<float>::max();
        Point3D closest_expansion;
        for (const auto& expansion : expansions_) {
            float current_distance = Distance2D(startLocation_, expansion);
            if (current_distance < .01f) {
                continue;
            }

            if (current_distance < minimum_distance) {
                if (Query()->Placement(build_ability, expansion)) {
                    closest_expansion = expansion;
                    minimum_distance = current_distance;
                }
            }
        }
        //only update staging location up till 3 bases.
        if (TryBuildStructure(build_ability, worker_type, closest_expansion, true) && observation->GetUnits(Unit::Self, IsTownHall()).size() < 4) {
            staging_location_ = Point3D(((staging_location_.x + closest_expansion.x) / 2), ((staging_location_.y + closest_expansion.y) / 2),
                ((staging_location_.z + closest_expansion.z) / 2));
            return true;
        }
        return false;

    }

    bool TryBuildExpansionNexus() {
        const ObservationInterface* observation = Observation();

        //Don't have more active bases than we can provide workers for
        if (GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR) > max_worker_count_) {
            return false;
        }
        // If we have extra workers around, try and build another nexus.
        if (GetExpectedWorkers(UNIT_TYPEID::PROTOSS_ASSIMILATOR) < observation->GetFoodWorkers() - 16) {
            return TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
        }
        //Only build another nexus if we are floating extra minerals
        if (observation->GetMinerals() > CountUnitType(observation, UNIT_TYPEID::PROTOSS_NEXUS) * 400) {
            return TryExpand(ABILITY_ID::BUILD_NEXUS, UNIT_TYPEID::PROTOSS_PROBE);
        }
        return false;
    }

    bool TryWarpInUnit(ABILITY_ID ability_type_for_unit) {
        const ObservationInterface* observation = Observation();
        std::vector<PowerSource> power_sources = observation->GetPowerSources();
        Units warpgates = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_WARPGATE));

        if (power_sources.empty()) {
            return false;
        }

        const PowerSource& random_power_source = GetRandomEntry(power_sources);
        float radius = random_power_source.radius;
        float rx = GetRandomScalar();
        float ry = GetRandomScalar();
        Point2D build_location = Point2D(random_power_source.position.x + rx * radius, random_power_source.position.y + ry * radius);

        // If the warp location is walled off, don't warp there.
        // We check this to see if there is pathing from the build location to the center of the map
        if (Query()->PathingDistance(build_location, Point2D(game_info_.playable_max.x / 2, game_info_.playable_max.y / 2)) < .01f) {
            return false;
        }

        for (const auto& warpgate : warpgates) {
            if (warpgate->build_progress == 1) {
                AvailableAbilities abilities = Query()->GetAbilitiesForUnit(warpgate);
                for (const auto& ability : abilities.abilities) {
                    if (ability.ability_id == ability_type_for_unit) {
                        Actions()->UnitCommand(warpgate, ability_type_for_unit, build_location);
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void ConvertGateWayToWarpGate() {
        const ObservationInterface* observation = Observation();
        Units gateways = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_GATEWAY));

        if (warpgate_reasearched_) {
            for (const auto& gateway : gateways) {
                if (gateway->build_progress == 1) {
                    Actions()->UnitCommand(gateway, ABILITY_ID::MORPH_WARPGATE);
                }
            }
        }
    }

    bool TryBuildStructureNearPylon(AbilityID ability_type_for_structure, UnitTypeID) {
        const ObservationInterface* observation = Observation();

        //Need to check to make sure its a pylon instead of a warp prism
        std::vector<PowerSource> power_sources = observation->GetPowerSources();
        if (power_sources.empty()) {
            return false;
        }

        const PowerSource& random_power_source = GetRandomEntry(power_sources);
        if (observation->GetUnit(random_power_source.tag) != nullptr) {
            if (observation->GetUnit(random_power_source.tag)->unit_type == UNIT_TYPEID::PROTOSS_WARPPRISM) {
                return false;
            }
        }
        else {
            return false;
        }
        float radius = random_power_source.radius;
        float rx = GetRandomScalar();
        float ry = GetRandomScalar();
        Point2D build_location = Point2D(random_power_source.position.x + rx * radius, random_power_source.position.y + ry * radius);
        return TryBuildStructure(ability_type_for_structure, UNIT_TYPEID::PROTOSS_PROBE, build_location);
    }


    void ManageArmy() {
        const ObservationInterface* observation = Observation();
        Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
        Units army = observation->GetUnits(Unit::Alliance::Self, IsArmy(observation));
        int wait_til_supply = 100;

        //There are no enemies yet, and we don't have a big army
        if (enemy_units.empty() && observation->GetFoodArmy() < wait_til_supply) {
            for (const auto& unit : army) {
                RetreatWithUnit(unit, staging_location_);
            }
        }
        else if(!enemy_units.empty()){
            for (const auto& unit : army) {
                AttackWithUnit(unit, observation);
                switch (unit->unit_type.ToType()) {
                    //Stalker blink micro, blinks back towards your base
                    case(UNIT_TYPEID::PROTOSS_OBSERVER): {
                        Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front()->pos);
                        break;
                    }
                    case(UNIT_TYPEID::PROTOSS_STALKER): {
                        if (blink_reasearched_) {
                            /*const Unit* old_unit = observation->GetPreviousUnit(unit.tag);
                            const Unit* target_unit = observation->GetUnit(unit.engaged_target_tag);
                            if (old_unit == nullptr) {
                                break;
                            }
                            Point2D blink_location = startLocation_;
                            if (old_unit->shield > 0 && unit.shield < 1) {
                                if (!unit.orders.empty()) {
                                    if (target_unit != nullptr) {
                                        Vector2D diff = unit.pos - target_unit->pos;
                                        Normalize2D(diff);
                                        blink_location = unit.pos + diff * 7.0f;
                                    }
                                    else {
                                        Vector2D diff = unit.pos - startLocation_;
                                        Normalize2D(diff);
                                        blink_location = unit.pos - diff * 7.0f;
                                    }
                                    Actions()->UnitCommand(unit.tag, ABILITY_ID::EFFECT_BLINK, blink_location);
                                }
                            }*/
                        }
                        break;
                    }
                    //Turns on guardian shell when close to an enemy
                    case (UNIT_TYPEID::PROTOSS_SENTRY): {
                        if (!unit->orders.empty()) {
                            if (unit->orders.front().ability_id == ABILITY_ID::ATTACK) {
                                float distance = std::numeric_limits<float>::max();
                                for (const auto& u : enemy_units) {
                                     float d = Distance2D(u->pos, unit->pos);
                                     if (d < distance) {
                                         distance = d;
                                     }
                                }
                                if (distance < 6 && unit->energy >= 75) {
                                    Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_GUARDIANSHIELD);
                                }
                            }
                        }
                        break;
                    }
                    //Turns on Damage boost when close to an enemy
                    case (UNIT_TYPEID::PROTOSS_VOIDRAY): {
                        if (!unit->orders.empty()) {
                            if (unit->orders.front().ability_id == ABILITY_ID::ATTACK) {
                                float distance = std::numeric_limits<float>::max();
                                for (const auto& u : enemy_units) {
                                    float d = Distance2D(u->pos, unit->pos);
                                    if (d < distance) {
                                        distance = d;
                                    }
                                }
                                if (distance < 8) {
                                    Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_VOIDRAYPRISMATICALIGNMENT);
                                }
                            }
                        }
                        break;
                    }
                    //Turns on oracle weapon when close to an enemy
                    case (UNIT_TYPEID::PROTOSS_ORACLE): {
                        if (!unit->orders.empty()) {
                             float distance = std::numeric_limits<float>::max();
                             for (const auto& u : enemy_units) {
                                 float d = Distance2D(u->pos, unit->pos);
                                 if (d < distance) {
                                     distance = d;
                                 }
                             }
                             if (distance < 6 && unit->energy >= 25) {
                                 Actions()->UnitCommand(unit, ABILITY_ID::BEHAVIOR_PULSARBEAMON);
                             }
                        }
                        break;
                    }
                    //fires a disruptor nova when in range
                    case (UNIT_TYPEID::PROTOSS_DISRUPTOR): {
                        float distance = std::numeric_limits<float>::max();
                        Point2D closest_unit;
                        for (const auto& u : enemy_units) {
                            float d = Distance2D(u->pos, unit->pos);
                            if (d < distance) {
                                distance = d;
                                closest_unit = u->pos;
                            }
                        }
                        if (distance < 7) {
                            Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_PURIFICATIONNOVA, closest_unit);
                        }
                        else {
                            Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, closest_unit);
                        }
                        break;
                    }
                    //controls disruptor novas.
                    case (UNIT_TYPEID::PROTOSS_DISRUPTORPHASED): {
                                float distance = std::numeric_limits<float>::max();
                                Point2D closest_unit;
                                for (const auto& u : enemy_units) {
                                    if (u->is_flying) {
                                        continue;
                                    }
                                    float d = DistanceSquared2D(u->pos, unit->pos);
                                    if (d < distance) {
                                        distance = d;
                                        closest_unit = u->pos;
                                    }
                                }
                                Actions()->UnitCommand(unit, ABILITY_ID::MOVE, closest_unit);
                        break;
                    }
                    default:
                        break;
                }
            }
        }
        else {
            for (const auto& unit : army) {
                ScoutWithUnit(unit, observation);
            }
        }
    }

    bool TryBuildArmy() {
        const ObservationInterface* observation = Observation();
        if (observation->GetFoodWorkers() <= target_worker_count_) {
            return false;
        }
        //Until we have 2 bases, hold off on building too many units
        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_NEXUS) < 2 && observation->GetFoodArmy() > 10) {
            return false;
        }

        //If we have a decent army already, try hold until we expand again
        if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_NEXUS) < 3 && observation->GetFoodArmy() > 40) {
            return false;
        }
        size_t mothership_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_MOTHERSHIPCORE);
        mothership_count += CountUnitType(observation, UNIT_TYPEID::PROTOSS_MOTHERSHIP);

        if (observation->GetFoodWorkers() > target_worker_count_ && mothership_count < 1) {
            if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE) > 0) {
                TryBuildUnit(ABILITY_ID::TRAIN_MOTHERSHIPCORE, UNIT_TYPEID::PROTOSS_NEXUS);
            }
        }
        size_t colossus_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_COLOSSUS);
        size_t carrier_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_CARRIER);
        Units templar = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_HIGHTEMPLAR));
        if (templar.size() > 1) {
            Units templar_merge;
            for (int i = 0; i < 2; ++i) {
                templar_merge.push_back(templar.at(i));
            }
            Actions()->UnitCommand(templar_merge, ABILITY_ID::MORPH_ARCHON);
        }

        if (air_build_) {
            //If we have a fleet beacon, and haven't hit our carrier count, build more carriers
            if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_FLEETBEACON) > 0 && carrier_count < max_colossus_count_) {
                //After the first carrier try and make a Mothership
                if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_MOTHERSHIP) < 1 && mothership_count > 0) {
                    if (TryBuildUnit(ABILITY_ID::MORPH_MOTHERSHIP, UNIT_TYPEID::PROTOSS_MOTHERSHIPCORE)) {
                        return true;
                    }
                    return false;
                }

                if (observation->GetMinerals() > 350 && observation->GetVespene() > 250) {
                    if (TryBuildUnit(ABILITY_ID::TRAIN_CARRIER, UNIT_TYPEID::PROTOSS_STARGATE)) {
                        return true;
                    }
                }
                else if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_TEMPEST) < 1) {
                    TryBuildUnit(ABILITY_ID::TRAIN_TEMPEST, UNIT_TYPEID::PROTOSS_STARGATE);
                }
                else if (carrier_count < 1){ //Try to build at least 1
                    return false;
                }
            }
            else {
                // If we can't build Carrier, try to build voidrays
                if (observation->GetMinerals() > 250 && observation->GetVespene() > 150) {
                    TryBuildUnit(ABILITY_ID::TRAIN_VOIDRAY, UNIT_TYPEID::PROTOSS_STARGATE);

                }
                //Have at least 1 void ray before we build the other units
                if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_VOIDRAY) > 0) {
                    if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_ORACLE) < 1) {
                        TryBuildUnit(ABILITY_ID::TRAIN_ORACLE, UNIT_TYPEID::PROTOSS_STARGATE);
                    }
                    else if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_PHOENIX) < 2) {
                        TryBuildUnit(ABILITY_ID::TRAIN_PHOENIX, UNIT_TYPEID::PROTOSS_STARGATE);
                    }
                }
            }
        }
        else {
            //If we have a robotics bay, and haven't hit our colossus count, build more colossus
            if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_OBSERVER) < 1) {
                TryBuildUnit(ABILITY_ID::TRAIN_OBSERVER, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);
            }
            if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_ROBOTICSBAY) > 0 && colossus_count < max_colossus_count_) {
                if (observation->GetMinerals() > 300 && observation->GetVespene() > 200) {
                    if (TryBuildUnit(ABILITY_ID::TRAIN_COLOSSUS, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY)) {
                        return true;
                    }
                }
                else if (CountUnitTypeTotal(observation, UNIT_TYPEID::PROTOSS_DISRUPTOR, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY, ABILITY_ID::TRAIN_DISRUPTOR) < 2) {
                    TryBuildUnit(ABILITY_ID::TRAIN_DISRUPTOR, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);
                }
            }
            else {
                // If we can't build Colossus, try to build immortals
                if (observation->GetMinerals() > 250 && observation->GetVespene() > 100) {
                    if (TryBuildUnit(ABILITY_ID::TRAIN_IMMORTAL, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY)) {
                        return true;
                    }
                }
            }
        }

        if (warpgate_reasearched_ &&  CountUnitType(observation, UNIT_TYPEID::PROTOSS_WARPGATE) > 0) {
            if (observation->GetMinerals() > 1000 && observation->GetVespene() < 200) {
                return TryWarpInUnit(ABILITY_ID::TRAINWARP_ZEALOT);
            }
            if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_STALKER) > max_stalker_count_){
                return false;
            }

            if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_ADEPT) > max_stalker_count_) {
                return false;
            }

            if (!air_build_) {
                if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_SENTRY) < max_sentry_count_) {
                    return TryWarpInUnit(ABILITY_ID::TRAINWARP_SENTRY);
                }

                if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_HIGHTEMPLAR) < 2 && CountUnitType(observation, UNIT_TYPEID::PROTOSS_ARCHON) < 2) {
                    return TryWarpInUnit(ABILITY_ID::TRAINWARP_HIGHTEMPLAR);
                }
            }
            //build adepts until we have robotics facility, then switch to stalkers.
            if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY) > 0) {
                return TryWarpInUnit(ABILITY_ID::TRAINWARP_STALKER);
            }
            else {
                return TryWarpInUnit(ABILITY_ID::TRAINWARP_ADEPT);
            }
        }
        else {
            //Train Adepts if we have a core otherwise build Zealots
            if (observation->GetMinerals() > 120 && observation->GetVespene() > 25) {
                if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE) > 0) {
                    return TryBuildUnit(ABILITY_ID::TRAIN_ADEPT, UNIT_TYPEID::PROTOSS_GATEWAY);
                }
            }
            else if (observation->GetMinerals() > 100) {
                return TryBuildUnit(ABILITY_ID::TRAIN_ZEALOT, UNIT_TYPEID::PROTOSS_GATEWAY);
            }
            return false;
        }
    }

    void BuildOrder() {
        const ObservationInterface* observation = Observation();
        size_t gateway_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_GATEWAY) + CountUnitType(observation,UNIT_TYPEID::PROTOSS_WARPGATE);
        size_t cybernetics_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
        size_t forge_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_FORGE);
        size_t twilight_council_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
        size_t templar_archive_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_TEMPLARARCHIVE);
        size_t base_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_NEXUS);
        size_t robotics_facility_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY);
        size_t robotics_bay_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ROBOTICSBAY);
        size_t stargate_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_STARGATE);
        size_t fleet_beacon_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_FLEETBEACON);

        // 3 Gateway per expansion
        if (gateway_count < std::min<size_t>(2 * base_count, 7)) {

            //If we have 1 gateway, prioritize building CyberCore
            if (cybernetics_count < 1 && gateway_count > 0) {
                TryBuildStructureNearPylon(ABILITY_ID::BUILD_CYBERNETICSCORE, UNIT_TYPEID::PROTOSS_PROBE);
                return;
            }
            else {
                //If we have 1 gateway Prioritize getting another expansion out before building more gateways
                if (base_count < 2 && gateway_count > 0) {
                    TryBuildExpansionNexus();
                    return;
                }

                if (observation->GetFoodWorkers() >= target_worker_count_  && observation->GetMinerals() > 150 + (100 * gateway_count)) {
                    TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_PROBE);
                }
            }
        }

        if (cybernetics_count > 0 && forge_count < 2) {
            TryBuildStructureNearPylon(ABILITY_ID::BUILD_FORGE, UNIT_TYPEID::PROTOSS_PROBE);
        }

        //go stargate or robo depending on build
        if (air_build_) {
            if (gateway_count > 1 && cybernetics_count > 0) {
                if (stargate_count < std::min<size_t>(base_count, 5)) {
                    if (observation->GetMinerals() > 150 && observation->GetVespene() > 150) {
                        TryBuildStructureNearPylon(ABILITY_ID::BUILD_STARGATE, UNIT_TYPEID::PROTOSS_PROBE);
                    }
                }
                else if (stargate_count > 0 && fleet_beacon_count < 1) {
                    if (observation->GetMinerals() > 300 && observation->GetVespene() > 200) {
                        TryBuildStructureNearPylon(ABILITY_ID::BUILD_FLEETBEACON, UNIT_TYPEID::PROTOSS_PROBE);
                    }
                }
            }
        }
        else {
            if (gateway_count > 2 && cybernetics_count > 0) {
                if (robotics_facility_count < std::min<size_t>(base_count, 4)) {
                    if (observation->GetMinerals() > 200 && observation->GetVespene() > 100) {
                        TryBuildStructureNearPylon(ABILITY_ID::BUILD_ROBOTICSFACILITY, UNIT_TYPEID::PROTOSS_PROBE);
                    }
                }
                else if (robotics_facility_count > 0 && robotics_bay_count < 1) {
                    if (observation->GetMinerals() > 200 && observation->GetVespene() > 200) {
                        TryBuildStructureNearPylon(ABILITY_ID::BUILD_ROBOTICSBAY, UNIT_TYPEID::PROTOSS_PROBE);
                    }
                }
            }
        }

        if (forge_count > 0 && twilight_council_count < 1 && base_count > 1) {
            TryBuildStructureNearPylon(ABILITY_ID::BUILD_TWILIGHTCOUNCIL, UNIT_TYPEID::PROTOSS_PROBE);
        }

        if (twilight_council_count > 0 && templar_archive_count < 1 && base_count > 2) {
            TryBuildStructureNearPylon(ABILITY_ID::BUILD_TEMPLARARCHIVE, UNIT_TYPEID::PROTOSS_PROBE);
        }

    }

    bool EarlyStrategy(){
        const ObservationInterface* observation = Observation();
        Units workers = observation->GetUnits(Unit::Alliance::Self,IsUnit(UNIT_TYPEID::PROTOSS_PROBE));
        Units bases = observation->GetUnits(Unit::Alliance::Self,IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));
        Units gateways = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_GATEWAY));
        Units warpgates = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_WARPGATE));
        Units adepts = observation->GetUnits(Unit::Alliance::Self,IsUnit(UNIT_TYPEID::PROTOSS_ADEPT));
        Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
        Units enemy_structures = observation->GetUnits(Unit::Alliance::Enemy,IsStructure(observation));
        Units enemy_townhalls = observation->GetUnits(Unit::Alliance::Enemy,IsTownHall());
        Units pylons = observation->GetUnits(Unit::Alliance::Self,IsUnit(UNIT_TYPEID::PROTOSS_PYLON));
        std::vector<PowerSource> power_sources = observation->GetPowerSources();

        //Actions()->UnitCommand(bases.front(), ABILITY_ID::EFFECT_CHRONOBOOST, bases.front());
        //Actions()->UnitCommand(bases.front(), ABILITY_ID::EFFECT_CHRONOBOOST, bases.front()->pos);
        const Unit* base = nullptr;
        base = bases.front();
        //GetRandomUnit(base, observation, UNIT_TYPEID::PROTOSS_NEXUS);

        if(advance_probe==nullptr){
            advance_probe = GetRandomEntry(workers);
        }
        else if(probe_scout==nullptr && observation->GetFoodUsed()>15){
            probe_scout = GetRandomEntry(workers);
            if(probe_scout==advance_probe) probe_scout=nullptr;
        }

        Point2D target_pos;
        FindEnemyPosition(target_pos);
        if (advance_pylon_location.x==0&&find_enemy_location==true) {
            advance_pylon_location = Point2D((startLocation_.x + target_pos.x*3)/4, (startLocation_.y + target_pos.y*3)/4);
        }
        size_t pylon_count = CountUnitType(observation,UNIT_TYPEID::PROTOSS_PYLON);
        size_t gateway_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_GATEWAY) + CountUnitType(observation,UNIT_TYPEID::PROTOSS_WARPGATE);
        size_t assimilator_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ASSIMILATOR);
        size_t cybernetics_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
        size_t twilight_council_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL);
        size_t adept_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ADEPT);

        const Unit* core = nullptr;
        const Unit* twilight = nullptr;

        //프로브 정찰
        if(find_enemy_location==false && probe_scout != nullptr){
            Actions()->UnitCommand(probe_scout, ABILITY_ID::MOVE, game_info_.enemy_start_locations.front());
            if(!enemy_townhalls.empty()||enemy_structures.size()>2||game_info_.enemy_start_locations.size()==1){
                if(Distance2D(probe_scout->pos,game_info_.enemy_start_locations.front())<10||game_info_.enemy_start_locations.size()==1) {
                    find_enemy_location=true;
                    std::cout<<"find!"<<std::endl;
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
            else{
                if(Distance2D(probe_scout->pos,game_info_.enemy_start_locations.front())<7) {
                    iter_esl=game_info_.enemy_start_locations.begin();
                    game_info_.enemy_start_locations.erase(iter_esl);
                }
            }


        }


        if(adept_count==1 && adept_first==nullptr){
            adept_first = adepts.front();
            /*std::cout<<startLocation_.x<<" "<<startLocation_.y<<std::endl;
            std::cout<<game_info_.width<<" "<<game_info_.height<<std::endl;
            if(startLocation_.x<game_info_.width/2&&startLocation_.y<game_info_.height/2){
                adept_dest_first = Point2D((float)game_info_.width/10*9,(float)game_info_.height/10);
            }
            else if(startLocation_.x<game_info_.width/2){
                adept_dest_first = Point2D((float)game_info_.width/10*9,(float)game_info_.height/10*9);
            }
            else if(startLocation_.y<game_info_.height/2){
                adept_dest_first = Point2D((float)game_info_.width/10,(float)game_info_.height/10);
            }
            else{
                adept_dest_first = Point2D((float)game_info_.width/10,(float)game_info_.height/10*9);
            }

            Actions()->UnitCommand(adepts.front(), ABILITY_ID::MOVE, adept_dest_first);
            adept_scout_first=true;*/
        }//첫 사도 지정
        if(adept_count==2 && adept_second==nullptr){
            for(const auto& adept : adepts){
                if(adept_first==nullptr){
                    adept_first = adept;
                }
                if(adept!=adept_first){
                    adept_second = adept;
                    /*if(startLocation_.x<game_info_.width/2&&startLocation_.y<game_info_.height/2){
                        adept_dest_second = Point2D((float)game_info_.width/10,(float)game_info_.height/10*9);
                    }
                    else if(startLocation_.x<game_info_.width/2){
                        adept_dest_second = Point2D((float)game_info_.width/10,(float)game_info_.height/10);
                    }
                    else if(startLocation_.y<game_info_.height/2){
                        adept_dest_second = Point2D((float)game_info_.width/10*9,(float)game_info_.height/10*9);
                    }
                    else{
                        adept_dest_second = Point2D((float)game_info_.width/10*9,(float)game_info_.height/10);
                    }

                    Actions()->UnitCommand(adept, ABILITY_ID::MOVE, adept_dest_second);
                    adept_scout_second=true;*/
                }
            }
        }//두 번째 사도 지정

        if(cybernetics_count != 0){
            Units cores = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_CYBERNETICSCORE));
            core = cores.front();
        }


        if(twilight_council_count == 1){
            Units twilights = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL));
            twilight = twilights.front();
        }


        if(core!=nullptr){
            if(core->build_progress==1&&core->buffs.size()==0&&base->energy>=50){
               Actions()->UnitCommand(base, ABILITY_ID::EFFECT_CHRONOBOOST, core);
               //std::cout<<"chrono"<<std::endl;
            }
        }

        /*if(advance_probe!=nullptr&&advance_pylon_location.x!=0){
            Actions()->UnitCommand(advance_probe, ABILITY_ID::MOVE, advance_pylon_location);
        }*/



        if(twilight_council_count==1){
            if(twilight->build_progress==1){
                if(twilight->orders.size()==0){
                    if(ADEPTRESONATINGGLAIVES==false){
                        Actions()->UnitCommand(twilight, ABILITY_ID::RESEARCH_ADEPTRESONATINGGLAIVES);
                    }
                    //std::cout<<"DONE!"<<std::endl;
                }
            }
        }

        if(observation->GetFoodWorkers()<23&&base->orders.size()==0&&observation->GetMinerals()>=50){
            TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
            return true;
        }

        if(observation->GetFoodUsed()<14){

        }
        else if(observation->GetFoodUsed()==14){
            if(observation->GetMinerals()>=100){
                if(pylon_count==0){
                    if(TryBuildPylon()){
                        return true;
                    }
                }
            }
        }
        else if(observation->GetFoodUsed()==15){

        }
        else if(observation->GetFoodUsed()==16){
            if(gateway_count==0&&observation->GetMinerals()>=150){
                TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_PROBE);
                return true;
            }
            if(assimilator_count==0&&gateway_count!=0&&observation->GetMinerals()>=75){
                TryBuildGas(ABILITY_ID::BUILD_ASSIMILATOR, UNIT_TYPEID::PROTOSS_PROBE, base->pos);
                return true;
            }
        }
        else if(observation->GetFoodUsed()==17){
            if(assimilator_count<2&&observation->GetMinerals()>=75){
                TryBuildGas(ABILITY_ID::BUILD_ASSIMILATOR, UNIT_TYPEID::PROTOSS_PROBE, base->pos);
                return true;
            }
        }
        else if(observation->GetFoodUsed()==18){
            if(gateway_count==1&&observation->GetMinerals()>=150){
                TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_PROBE);
                return true;
            }
        }
        else if(observation->GetFoodUsed()==19){

        }
        else if(observation->GetFoodUsed()==20){
            if(observation->GetMinerals()>=150){
                if(CountUnitType(observation, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE) == 0){
                    TryBuildStructureNearPylon(ABILITY_ID::BUILD_CYBERNETICSCORE, UNIT_TYPEID::PROTOSS_PROBE);
                    return true;
                }
            }
        }
        else if(observation->GetFoodUsed()==21){
            if(observation->GetMinerals()>=100){
                if(pylon_count==1){
                    if(TryBuildPylon()){
                        return true;
                    }
                }
            }
        }
        else if(observation->GetFoodUsed()==22){

        }
        /*else if(observation->GetFoodUsed()==23){
            if(twilight_council_count==0){
                if(observation->GetMinerals()>=150&&observation->GetVespene()>=100){
                    if(advance_probe==nullptr){
                        GetRandomUnit(advance_probe,Observation(),UNIT_TYPEID::PROTOSS_PROBE);
                        while(advance_probe==probe_scout) GetRandomUnit(advance_probe,Observation(),UNIT_TYPEID::PROTOSS_PROBE);
                        Actions()->UnitCommand(advance_probe, ABILITY_ID::MOVE, advance_pylon_location);
                    }

                    TryBuildStructureNearPylon(ABILITY_ID::BUILD_TWILIGHTCOUNCIL, UNIT_TYPEID::PROTOSS_PROBE);
                    return true;
                }
            }
        }*/


        if(observation->GetFoodUsed()>=23&&observation->GetFoodUsed()<=25){
            //if(twilight_council_count==1){
                for (const auto& gate : gateways){
                    if(gate->orders.size()==0){
                        Actions()->UnitCommand(gate, ABILITY_ID::TRAIN_ADEPT);
                    }
                }
            //}
        }

        /*Units adepts = observation->GetUnits(Unit::Alliance::Self,IsUnit(UNIT_TYPEID::PROTOSS_ADEPT));
        if(adept_count==2){
            Point2D location;
            float rx = GetRandomScalar();
            float ry = GetRandomScalar();
            for (const auto& adept : adepts){
                location = Point2D(adept->pos.x + rx*10,adept->pos.y + ry*10);
                //if(adept->game_loop==0) adept->game_loop=observation->GetGameLoop();
                std::cout<<adept->game_loop<<" "<<observation->GetGameLoop()<<std::endl;
                Actions()->UnitCommand(adept, ABILITY_ID::EFFECT_ADEPTPHASESHIFT,location);
            }

        }


        return false;*/
        if(observation->GetFoodUsed()==27){

            if(twilight_council_count==0){
                if(observation->GetMinerals()>=150&&observation->GetVespene()>=100){
                    TryBuildStructureNearPylon(ABILITY_ID::BUILD_TWILIGHTCOUNCIL, UNIT_TYPEID::PROTOSS_PROBE);
                }
            }
            else{
                if(gateway_count<4){
                    TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_PROBE);
                    Actions()->UnitCommand(advance_probe, ABILITY_ID::MOVE, advance_pylon_location);
                }
                if(gateway_count==4){
                    if(pylon_count==2){
                        if(!Query()->Placement(ABILITY_ID::BUILD_PYLON, advance_pylon_location)){
                            const PowerSource& random_power_source = GetRandomEntry(power_sources);
                            float radius = random_power_source.radius;
                            float rx = GetRandomScalar();
                            float ry = GetRandomScalar();
                            while(!Query()->Placement(ABILITY_ID::BUILD_PYLON, Point2D(advance_pylon_location.x+ry*radius,advance_pylon_location.y+ry*radius))){
                                rx = GetRandomScalar();
                                ry=GetRandomScalar();
                            }
                            advance_pylon_location = Point2D(advance_pylon_location.x+ry*radius*0.3,advance_pylon_location.y+ry*radius*0.3);
                            advance_pylon=true;
                        }
                        else{
                            Actions()->UnitCommand(advance_probe,ABILITY_ID::BUILD_PYLON, advance_pylon_location);
                            advance_pylon=true;
                        }
/*
                        while(!Query()->Placement(ABILITY_ID::BUILD_PYLON, advance_pylon_location)){

                            advance_pylon_location = Point2D(advance_pylon_location.x+ry*radius*0.5,advance_pylon_location.y+ry*radius*0.5);

                        }

                        Actions()->UnitCommand(advance_probe,ABILITY_ID::BUILD_PYLON, advance_pylon_location);
                        advance_pylon=true;
                            /*}
                        else advance_pylon=true;*/
                    }

                }
            }
        }

        if(observation->GetFoodUsed()==27||observation->GetFoodUsed()==29){
            if(pylon_count==3){
                if(gateway_count==4&&adept_count>=2){
                    for (const auto& gate : gateways){
                        if(gate->orders.size()==0){
                            Actions()->UnitCommand(gate, ABILITY_ID::TRAIN_ADEPT);
                        }
                    }
                }
            }
        }
        if(adept_count>=4){
            //std::cout<<"early_strategy : finish"<<std::endl;
            early_strategy = true;
            return true;
        }

        return false;
    }

    bool TryWarpAdept(){
        const ObservationInterface* observation = Observation();
        std::vector<PowerSource> power_sources = observation->GetPowerSources();
        Units warpgates = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_WARPGATE));

        if (power_sources.empty()) {
            return false;
        }

        const PowerSource& random_power_source = GetRandomEntry(power_sources);

        float radius = random_power_source.radius;
        float rx = GetRandomScalar();
        float ry = GetRandomScalar();
        Point2D build_location = Point2D(advance_pylon_location.x + rx * radius, advance_pylon_location.y + ry * radius);


        if (Query()->PathingDistance(build_location, Point2D(game_info_.playable_max.x / 2, game_info_.playable_max.y / 2)) < .01f) {
            return false;
        }

        for (const auto& warpgate : warpgates) {
            //Actions()->UnitCommand(warpgate, ABILITY_ID::TRAINWARP_ADEPT, build_location);
            if (warpgate->build_progress == 1) {
                AvailableAbilities abilities = Query()->GetAbilitiesForUnit(warpgate);
                for (const auto& ability : abilities.abilities) {
                    if (ability.ability_id == ABILITY_ID::TRAINWARP_ADEPT) {
                        Actions()->UnitCommand(warpgate, ABILITY_ID::TRAINWARP_ADEPT, build_location);
                        //std::cout<<"true"<<std::endl;
                        return true;
                    }
                }
            }
        }
        //std::cout<<"false"<<std::endl;
        return false;
    }

    bool ManageAdept(){
        const ObservationInterface* observation = Observation();
        Units adepts = observation->GetUnits(Unit::Alliance::Self,IsUnit(UNIT_TYPEID::PROTOSS_ADEPT));
        Units adeptphaseshifts = observation->GetUnits(Unit::Alliance::Self,IsUnit(UNIT_TYPEID::PROTOSS_ADEPTPHASESHIFT));
        uint32_t game_loop = Observation()->GetGameLoop();


        Point2D location;
        float rx = GetRandomScalar();
        float ry = GetRandomScalar();
        for (const auto& adept : adepts){
            //Actions()->UnitCommand(adept,ABILITY_ID::MOVE,enemy_expansion);
            continue;

            if(adept_map.count(adept->tag)==0){
                adept_map.insert(std::make_pair((adept->tag),observation->GetGameLoop()));
                location = Point2D(adept->pos.x + rx*10,adept->pos.y + ry*10);
                Actions()->UnitCommand(adept, ABILITY_ID::EFFECT_ADEPTPHASESHIFT,location);
                continue;
            }
            if(adept_map.find(adept->tag)->second+120>game_loop) continue;
            Actions()->UnitCommand(adept,ABILITY_ID::CANCEL_ADEPTPHASESHIFT);
            adept_map.erase(adept->tag);
        }
        for (const auto& adept : adepts){
            //std::cout<<Observation()->GetGameLoop()<<std::endl;

            for(const auto& buff : adept->buffs){
                //std::cout<<buff<<std::endl;
            }
            //std::cout<<adept->build_progress<<std::endl;


            //Actions()->UnitCommand(adept,ABILITY_ID::CANCEL_ADEPTPHASESHIFT);
            //Actions()->UnitCommand(adept,ABILITY_ID::CANCEL_ADEPTSHADEPHASESHIFT);
            //shade는 X
        }
        for (const auto& adeptphaseshift : adeptphaseshifts){
            //std::cout<<adeptphaseshift->build_progress<<std::endl;
            //Actions()->UnitCommand(adeptphaseshift,ABILITY_ID::CANCEL_ADEPTSHADEPHASESHIFT);
        }
        return true;


    }

    void scoutprobe(){
        const ObservationInterface* observation = Observation();

        Point2D tag_pos = FindNearestMineralPatch(*iter_exp)->pos;
        if(Distance2D(game_info_.enemy_start_locations.front(),tag_pos)<7 || Distance2D(enemy_expansion,tag_pos)<7){
            iter_exp++;
            return;
        }
        Actions()->UnitCommand(probe_scout,ABILITY_ID::MOVE,tag_pos);
        if(Distance2D(probe_scout->pos,tag_pos)<1){
            iter_exp++;
        }

    }

    void scoutadept(){

        const ObservationInterface* observation = Observation();
        Units adepts = observation->GetUnits(Unit::Alliance::Self,IsUnit(UNIT_TYPEID::PROTOSS_ADEPT));
        Point2D target_pos = game_info_.enemy_start_locations.front();
        Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy,IsArmy(observation));
        Units enemy_structures = observation->GetUnits(Unit::Alliance::Enemy,IsStructure(observation));

        if(adept_first==nullptr||adept_scout.first==true){}
        else if(adept_scout_first==false){
            //처음에 세로로 이동
            if(adept_dest_first.x<5){
                if(startLocation_.x*2<game_info_.width&&startLocation_.y*2<game_info_.height){
                    adept_dest_first = Point2D((float)game_info_.width/10*9,(float)game_info_.height/10);
                }
                else if(startLocation_.x*2<game_info_.width){
                    adept_dest_first = Point2D((float)game_info_.width/10*9,(float)game_info_.height/10*9);
                }
                else if(startLocation_.y*2<game_info_.height){
                    adept_dest_first = Point2D((float)game_info_.width/10,(float)game_info_.height/10);
                }
                else{
                    adept_dest_first = Point2D((float)game_info_.width/10,(float)game_info_.height/10*9);
                }
                adept_scout_first=true;
                Actions()->UnitCommand(adept_first, ABILITY_ID::MOVE, adept_dest_first);
            }
            else{
                //대각선 위치일 때
                adept_dest_first = Point2D(game_info_.width-startLocation_.x,game_info_.height-startLocation_.y);
                if(Distance2D(adept_dest_first,game_info_.enemy_start_locations.front())<20){
                    //adept_dest_first = Point2D((float)game_info_.width/2,(float)game_info_.height/2);
                    adept_scout.first = true;
                    //adept_scout_first = true;
                    Actions()->UnitCommand(adept_first, ABILITY_ID::MOVE, advance_pylon_location);
                }
            }
            //std::cout<<adept_dest_first.x<<" "<<adept_dest_first.y<<std::endl;
        }
        else{
            //첫 정찰이 상대 본진일 때
            if(Distance2D(adept_first->pos,game_info_.enemy_start_locations.front())<20){
                adept_dest_first = Point2D(game_info_.width-startLocation_.x,game_info_.height-startLocation_.y);
                Actions()->UnitCommand(adept_first,ABILITY_ID::MOVE,adept_dest_first);
            }
            else if(Distance2D(adept_first->pos,adept_dest_first)<10){
                adept_scout.first=true;
                Actions()->UnitCommand(adept_first,ABILITY_ID::MOVE,advance_pylon_location);
            }
        }

        if(adept_second==nullptr||adept_scout.second==true){}
        else if(adept_scout_second==false){
            if(startLocation_.x*2<game_info_.width&&startLocation_.y*2<game_info_.height){
                adept_dest_second = Point2D((float)game_info_.width/10,(float)game_info_.height/10*9);
            }
            else if(startLocation_.x*2<game_info_.width){
                adept_dest_second = Point2D((float)game_info_.width/10,(float)game_info_.height/10);
            }
            else if(startLocation_.y*2<game_info_.height){
                adept_dest_second = Point2D((float)game_info_.width/10*9,(float)game_info_.height/10);
            }
            else{
                adept_dest_second = Point2D((float)game_info_.width/10*9,(float)game_info_.height/10);
            }
            adept_scout_second = true;
            Actions()->UnitCommand(adept_second, ABILITY_ID::MOVE, adept_dest_second);
        }
        else{
            //상대 본진일 때
            if(Distance2D(adept_first->pos,game_info_.enemy_start_locations.front())<20){
                adept_dest_first = Point2D(game_info_.width-startLocation_.x,game_info_.height-startLocation_.y);
                Actions()->UnitCommand(adept_first,ABILITY_ID::MOVE,adept_dest_first);
            }
            else if(Distance2D(adept_first->pos,adept_dest_first)<10){
                adept_scout.first=true;
                Actions()->UnitCommand(adept_first,ABILITY_ID::MOVE,advance_pylon_location);
            }
        }
    }


    std::vector<UNIT_TYPEID> barrack_types = { UNIT_TYPEID::TERRAN_BARRACKSFLYING, UNIT_TYPEID::TERRAN_BARRACKS };
    std::vector<UNIT_TYPEID> factory_types = { UNIT_TYPEID::TERRAN_FACTORYFLYING, UNIT_TYPEID::TERRAN_FACTORY };
    std::vector<UNIT_TYPEID> starport_types = { UNIT_TYPEID::TERRAN_STARPORTFLYING, UNIT_TYPEID::TERRAN_STARPORT };
    std::vector<UNIT_TYPEID> supply_depot_types = { UNIT_TYPEID::TERRAN_SUPPLYDEPOT, UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED };
    std::vector<UNIT_TYPEID> bio_types = { UNIT_TYPEID::TERRAN_MARINE, UNIT_TYPEID::TERRAN_MARAUDER, UNIT_TYPEID::TERRAN_GHOST, UNIT_TYPEID::TERRAN_REAPER /*reaper*/ };
    std::vector<UNIT_TYPEID> widow_mine_types = { UNIT_TYPEID::TERRAN_WIDOWMINE, UNIT_TYPEID::TERRAN_WIDOWMINEBURROWED };
    std::vector<UNIT_TYPEID> siege_tank_types = { UNIT_TYPEID::TERRAN_SIEGETANK, UNIT_TYPEID::TERRAN_SIEGETANKSIEGED };
    std::vector<UNIT_TYPEID> viking_types = { UNIT_TYPEID::TERRAN_VIKINGASSAULT, UNIT_TYPEID::TERRAN_VIKINGFIGHTER };
    std::vector<UNIT_TYPEID> hellion_types = { UNIT_TYPEID::TERRAN_HELLION, UNIT_TYPEID::TERRAN_HELLIONTANK };

    bool air_build_ = false;
    bool warpgate_reasearched_ = false;
    bool blink_reasearched_ = false;
    bool ADEPTRESONATINGGLAIVES = false;
    int max_colossus_count_ = 5;
    int max_sentry_count_ = 2;
    int max_stalker_count_ = 20;

    bool early_strategy = false;
    Point2D advance_pylon_location = Point2D(0,0);
    const Unit* advance_probe = nullptr;
    bool advance_pylon = false;

    std::map<uint64_t, uint32_t> adept_map;//(adept_tag,phaseshift를 쓴 시점의 gameloop)

    //bool adept_scout = false;
    std::pair<bool,bool> adept_scout = std::make_pair(false, false);
    bool adept_scout_first = false;
    bool adept_scout_second = false;
    const Unit* adept_first = nullptr;
    const Unit* adept_second = nullptr;
    Point2D adept_dest_first=Point2D(0,0);
    Point2D adept_dest_second=Point2D(0,0);
    Point2D probe_dest = Point2D(0,0);
    const Unit* probe_scout = nullptr;
    bool find_enemy_location=false;
    std::vector<Point2D>::iterator iter_esl=game_info_.enemy_start_locations.begin();
    std::vector<Point3D>::iterator iter_exp;
    Point3D enemy_expansion;
};

int main(int argc, char* argv[]) {
    Coordinator coordinator;
    coordinator.LoadSettings(argc, argv);

    Bot bot;
    coordinator.SetParticipants({
        CreateParticipant(Race::Protoss, &bot),
        CreateComputer(Race::Random,Difficulty::VeryEasy)
    });

    coordinator.LaunchStarcraft();
    coordinator.StartGame(sc2::kMapDarknessSanctuaryLE);

    while (coordinator.Update()){
        // Slow down game speed for better look & feel while making experiments.
        //sc2::SleepFor(10);
    }

    return 0;
}
