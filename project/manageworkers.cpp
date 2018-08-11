#include "memibot.h"

void MEMIBot::MakeBaseResourceMap() {
	const ObservationInterface* observation = Observation();

	if (last_map_renewal == observation->GetGameLoop() + 1) {
		return;
	}

	last_map_renewal = observation->GetGameLoop() + 1;

	resources_to_nearest_base.clear();
	resources_to_nearest_base.emplace(NullTag, NullTag);

	Filter filter_geyser = [](const Unit& u) {
		return u.build_progress == 1.0f &&
			IsUnits({ UNIT_TYPEID::PROTOSS_ASSIMILATOR,
				UNIT_TYPEID::TERRAN_REFINERY,
				UNIT_TYPEID::ZERG_EXTRACTOR })(u);
	};

	Filter filter_bases = [](const Unit& u) {
		return u.build_progress == 1.0f &&
			IsTownHall()(u);
	};

	Units geysers = observation->GetUnits(Unit::Alliance::Self, filter_geyser);
	Units bases = observation->GetUnits(Unit::Alliance::Self, filter_bases);
	Units minerals = observation->GetUnits(Unit::Alliance::Neutral, IsMineral());

	for (const auto& m : minerals) {
		const Unit* b = FindNearestUnit(m->pos, bases, 11.5);
		Tag b_tag = (b != nullptr) ? b->tag : NullTag;
		resources_to_nearest_base.emplace(m->tag, b_tag);
	}

	for (const auto& g : geysers) {
		const Unit* b = FindNearestUnit(g->pos, bases, 11.5);
		Tag b_tag = (b != nullptr) ? b->tag : NullTag;
		resources_to_nearest_base.emplace(g->tag, b_tag);
	}
}

// todo: pathingdistance
void MEMIBot::MineIdleWorkers(const Unit* worker) {
	const ObservationInterface* observation = Observation();

	Filter filter_geyser = [](const Unit& u) {
		return u.build_progress == 1.0f &&
			IsUnits({ UNIT_TYPEID::PROTOSS_ASSIMILATOR,
				UNIT_TYPEID::TERRAN_REFINERY,
				UNIT_TYPEID::ZERG_EXTRACTOR })(u);
	};

	Filter filter_bases = [](const Unit& u) {
		return u.build_progress == 1.0f &&
			IsTownHall()(u) &&
			u.ideal_harvesters != 0;
	};

	Units geysers = observation->GetUnits(Unit::Alliance::Self, filter_geyser);
	Units bases = observation->GetUnits(Unit::Alliance::Self, filter_bases);
	Units minerals = observation->GetUnits(Unit::Alliance::Neutral, IsMineral());
	size_t geysers_size = geysers.size();
	size_t bases_size = bases.size();
	size_t minerals_size = minerals.size();

	if (bases.empty()) {
		return;
	}

	MakeBaseResourceMap();

	bool has_space_for_half_mineral = true;
	bool has_space_for_gas = false;
	bool has_space_for_mineral = false;
	const Unit* gas_base = nullptr;
	const Unit* mineral_base = nullptr;
	const Unit* valid_mineral_patch = nullptr;

	// If there are very few workers gathering minerals.
	for (const auto& base : bases) {
		if (base->assigned_harvesters >= base->ideal_harvesters / 2 && base->assigned_harvesters >= 4) {
			has_space_for_half_mineral = false;
			break;
		}
	}
	// Search for a base that is missing workers.
	for (const auto& base : bases) {
		if (base->assigned_harvesters < base->ideal_harvesters) {
			has_space_for_mineral = true;
			mineral_base = base;
			break;
		}
	}
	// Search for a base that is missing workers.
	for (const auto& geyser : geysers) {
		Tag base_tag = resources_to_nearest_base.count(geyser->tag) ? resources_to_nearest_base.at(geyser->tag) : NullTag;
		if (base_tag == NullTag) continue;
		if (geyser->assigned_harvesters < geyser->ideal_harvesters) {
			has_space_for_gas = true;
			gas_base = observation->GetUnit(base_tag);
			break;
		}
	}

	float min_distance = std::numeric_limits<float>::max();

	// Search for a base that is missing mineral workers.
	if (has_space_for_half_mineral && !EnemyRush) {
		const Unit* target_resource = nullptr;
		const Unit* target_base = nullptr;

		for (const auto& mineral : minerals) {
			Tag b = resources_to_nearest_base.count(mineral->tag) ? resources_to_nearest_base.at(mineral->tag) : NullTag;
			if (b == NullTag) continue;
			const Unit* base = observation->GetUnit(b);
			if (base == nullptr) continue;
			if (base->assigned_harvesters >= base->ideal_harvesters / 2) continue;
			float current_distance = DistanceSquared2D(mineral->pos, worker->pos);
			if (current_distance < min_distance) {
				min_distance = current_distance;
				target_resource = mineral;
				target_base = base;
			}
		}

		if (target_resource != nullptr && target_base != nullptr) {
			float d = Query()->PathingDistance(worker, (target_resource->pos + target_base->pos)/2 );
			if (d > 0.01f) {
				Actions()->UnitCommand(worker, ABILITY_ID::HARVEST_GATHER, target_resource);
				return;
			}
		}
	}

	

	// Search for a base that does not have full of gas workers.
	if (has_space_for_gas && !EnemyRush) {
		const Unit* target_resource = nullptr;
		const Unit* target_base = nullptr;

		for (const auto& geyser : geysers) {
			if (geyser->assigned_harvesters >= geyser->ideal_harvesters) continue;
			Tag b = resources_to_nearest_base.count(geyser->tag) ? resources_to_nearest_base.at(geyser->tag) : NullTag;
			if (b == NullTag) continue;
			const Unit* base = observation->GetUnit(b);
			if (base == nullptr) continue;
			float current_distance = DistanceSquared2D(geyser->pos, worker->pos);
			if (current_distance < min_distance) {
				min_distance = current_distance;
				target_resource = geyser;
				target_base = base;
			}
		}

		if (target_resource != nullptr && target_base != nullptr) {
			float d = Query()->PathingDistance(worker, (target_resource->pos + target_base->pos) / 2);
			if (d > 0.01f) {
				Actions()->UnitCommand(worker, ABILITY_ID::HARVEST_GATHER, target_resource);
				return;
			}
		}
	}

	// Search for a base that does not have full of mineral workers.
	if (has_space_for_mineral && !EnemyRush) {
		const Unit* target_resource = nullptr;
		const Unit* target_base = nullptr;

		for (const auto& mineral : minerals) {
			Tag b = resources_to_nearest_base.count(mineral->tag) ? resources_to_nearest_base.at(mineral->tag) : NullTag;
			if (b == NullTag) continue;
			const Unit* base = observation->GetUnit(b);
			if (base == nullptr) continue;
			if (base->assigned_harvesters >= base->ideal_harvesters) continue;
			float current_distance = DistanceSquared2D(mineral->pos, worker->pos);
			if (current_distance < min_distance) {
				min_distance = current_distance;
				target_resource = mineral;
				target_base = base;
			}
		}

		if (target_resource != nullptr && target_base != nullptr) {
			float d = Query()->PathingDistance(worker, (target_resource->pos + target_base->pos) / 2);
			if (d > 0.01f) {
				Actions()->UnitCommand(worker, ABILITY_ID::HARVEST_GATHER, target_resource);
				return;
			}
		}
	}

	if (!worker->orders.empty()) {
		return;
	}

	//If all workers are spots are filled just go to any base.
	const Unit* random_base = GetRandomEntry(bases);
	valid_mineral_patch = FindNearestMineralPatch(random_base->pos);
	if (valid_mineral_patch == nullptr) return;
	Actions()->UnitCommand(worker, ABILITY_ID::HARVEST_GATHER, valid_mineral_patch);
}

int MEMIBot::GetExpectedWorkers(UNIT_TYPEID vespene_building_type) {
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

void MEMIBot::ManageWorkers() {
	const ObservationInterface* observation = Observation();

	Filter filter_geyser = [](const Unit& u) {
		return u.build_progress == 1.0f &&
			IsUnits({ UNIT_TYPEID::PROTOSS_ASSIMILATOR,
				UNIT_TYPEID::TERRAN_REFINERY,
				UNIT_TYPEID::ZERG_EXTRACTOR })(u);
	};

	Filter filter_bases = [](const Unit& u) {
		return u.build_progress == 1.0f &&
			IsTownHall()(u) &&
			u.ideal_harvesters != 0;
	};


	Units geysers = observation->GetUnits(Unit::Alliance::Self, filter_geyser);
	Units bases = observation->GetUnits(Unit::Alliance::Self, filter_bases);
	Units minerals = observation->GetUnits(Unit::Alliance::Neutral, IsMineral());
	Units workers = observation->GetUnits(Unit::Alliance::Self, IsWorker());
	size_t geysers_size = geysers.size();
	size_t bases_size = bases.size();
	size_t minerals_size = minerals.size();

	if (bases.empty()) {
		return;
	}

	MakeBaseResourceMap();

	bool has_space_for_half_mineral = true;
	bool has_space_for_gas = false;
	bool has_space_for_mineral = false;
	const Unit* gas_base = nullptr;
	const Unit* mineral_base = nullptr;
	const Unit* valid_mineral_patch = nullptr;

	// If there are very few workers gathering minerals.
	for (const auto& base : bases) {
		if (base->assigned_harvesters >= base->ideal_harvesters / 2 && base->assigned_harvesters >= 4) {
			has_space_for_half_mineral = false;
			break;
		}
	}
	// Search for a base that is missing workers.
	for (const auto& base : bases) {
		if (base->assigned_harvesters < base->ideal_harvesters) {
			has_space_for_mineral = true;
			mineral_base = base;
			break;
		}
	}
	// Search for a base that is missing workers.
	for (const auto& geyser : geysers) {
		Tag base_tag = resources_to_nearest_base.count(geyser->tag) ? resources_to_nearest_base.at(geyser->tag) : NullTag;
		if (base_tag == NullTag) continue;
		if (geyser->assigned_harvesters < geyser->ideal_harvesters) {
			has_space_for_gas = true;
			gas_base = observation->GetUnit(base_tag);
			break;
		}
	}

	if (has_space_for_mineral || has_space_for_gas) {
		for (const auto& worker : workers) {
			if (worker == probe_scout) continue;
			if (worker == probe_forward && !work_probe_forward) continue;
			if (worker->orders.empty()) continue;
			const UnitOrder& o = worker->orders.front();
			if (o.ability_id != ABILITY_ID::HARVEST_GATHER) continue;

			Tag target_tag = o.target_unit_tag;
			Tag nearest_base_tag = resources_to_nearest_base.count(target_tag) ? resources_to_nearest_base.at(target_tag) : NullTag;

			// reassign workers that mines resources far from nexuses. (get all)
			if (nearest_base_tag == NullTag) {
				MineIdleWorkers(worker);
				Print("reassigning no nexus workers");
				return;
			}

			const Unit* nearest_base = observation->GetUnit(nearest_base_tag);
			const Unit* target_resource = observation->GetUnit(target_tag);
			if (target_resource == nullptr) continue;
			if (nearest_base == nullptr) continue;

			// reassign overflowing workers (geysers)
			if (!IsMineral()(*target_resource)) {
				if (target_resource->assigned_harvesters - target_resource->ideal_harvesters <= 0) continue;
				MineIdleWorkers(worker);
				return;
			}
			// if there is a space
			// reassign overflowing workers (minerals)
			else {
				if (nearest_base->assigned_harvesters - nearest_base->ideal_harvesters <= 0) continue;
				MineIdleWorkers(worker);
				return;
			}
		}
	}

	// if few workers are mining minerals, then mine mineral rather than gas
	if (has_space_for_half_mineral && !EnemyRush) {
		for (const auto& geyser : geysers) {
			if (geyser->assigned_harvesters == 0) continue;

			for (const auto& worker : workers) {
				if (worker == probe_scout) continue;
				if (worker == probe_forward && !work_probe_forward) continue;
				// pick gas mining workers
				if (worker->orders.empty()) continue;
				const UnitOrder& o = worker->orders.front();
				if (o.ability_id != ABILITY_ID::HARVEST_GATHER) continue;
				if (o.target_unit_tag != geyser->tag) continue;

				MineIdleWorkers(worker);
				Print("reassigning for mineral workers");
				return;
			}
		}
	}

	// mine gas.
	if (has_space_for_gas && !EnemyRush) {
		// sort by distance
		Point2D gas_base_pos = gas_base->pos;
		std::function<bool(const Unit*, const Unit*)> f =
			[gas_base_pos](const Unit* b1, const Unit* b2) {
			return DistanceSquared2D(gas_base_pos, b1->pos) < DistanceSquared2D(gas_base_pos, b2->pos);
		};

		std::sort(bases.begin(), bases.end(), f);

		for (const auto& base : bases) {
			if (base->assigned_harvesters - 1 < (base->ideal_harvesters / 2)) continue;

			const Unit* target_worker = nullptr;
			float min_distance = std::numeric_limits<float>::max();

			for (const auto& worker : workers) {
				if (worker == probe_scout) continue;
				if (worker == probe_forward && !work_probe_forward) continue;
				// pick mineral mining workers first.
				if (worker->orders.empty()) continue;
				const UnitOrder& o = worker->orders.front();
				if (o.ability_id != ABILITY_ID::HARVEST_GATHER) continue;
				const Unit* target_resource = observation->GetUnit(o.target_unit_tag);
				Tag target_tag = o.target_unit_tag;
				if (target_resource == nullptr) continue;
				if (target_tag == NullTag) continue;
				if (!IsMineral()(*target_resource)) continue;

				Tag nearest_base = resources_to_nearest_base.count(target_tag) ? resources_to_nearest_base.at(target_tag) : NullTag;
				if (base->tag != nearest_base) continue;

				float current_distance = DistanceSquared2D(gas_base->pos, worker->pos);
				if (current_distance < min_distance) {
					target_worker = worker;
					min_distance = current_distance;
				}
			}

			if (target_worker != nullptr) {
				MineIdleWorkers(target_worker);
				Print("reassigning for gas workers");
				return;
			}
		}
	}
}

void MEMIBot::FleeWorkers(const Unit* unit) {
	if (unit == nullptr) return;
	if (!ManyEnemyRush) return;
	const ObservationInterface* observation = Observation();
	Units workers = observation->GetUnits(Unit::Alliance::Self, IsWorker());

	Units nearworkers(workers.size());
	for (const auto& worker : workers) {
		// consider near workers (distance < 10)
		if (DistanceSquared2D(unit->pos, worker->pos) > 100) continue;
		// do not consider killerworkers
		if (emergency_killerworkers.count(worker)) continue;

		nearworkers.push_back(worker);
	}
	const Unit* b = FindSecondNearestUnit(unit->pos, Unit::Alliance::Self, IsTownHall());
	if (b == nullptr) return;
	const Unit* m = FindNearestMineralPatch(b->pos);
	
	Actions()->UnitCommand(nearworkers, ABILITY_ID::SMART);
	return;
}

// todo: 포톤캐논, 해처리
// todo: fleeworkers와 충돌
void MEMIBot::DefendWorkers() {
	const ObservationInterface* observation = Observation();
	Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
	Units workers = observation->GetUnits(Unit::Alliance::Self, IsWorker());
	Units my_armies = observation->GetUnits(Unit::Alliance::Self, IsArmy(observation));

	size_t workers_size = workers.size();

	// push
	if (flags.status("defend_workers"))
	{
		// remove dead
		for (auto& it = emergency_killerworkers.begin(); it != emergency_killerworkers.end();)
		{
			const Unit* killerworker = *it;
			if (!killerworker->is_alive)
			{
				it = emergency_killerworkers.erase(it);
			}
			else {
				++it;
			}
		}

		// push alive
		for (const auto& worker : workers) {
			if (emergency_killerworkers.count(worker)) continue;
			const Unit* target = GetTarget(worker, enemyUnitsInRegion);
			if (target == nullptr) continue;
			if (emergency_killerworkers.size() <= enemyUnitsInRegion.size() + 1)
			{
				emergency_killerworkers.insert(worker);
			}
		}
		// attack
		for (const auto& killerworker : emergency_killerworkers)
		{
			{
				const Unit* target = GetTarget(killerworker, enemyUnitsInRegion);
				SmartAttackUnit(killerworker, target);
			}
		}
	}
	// pop
	else
	{
		if (!emergency_killerworkers.empty()) {
			Units tmp_killerworkers(emergency_killerworkers.size());
			for (const auto& killerworker : emergency_killerworkers) {
				if (killerworker->is_alive) {
					tmp_killerworkers.push_back(killerworker);
				}
			}
			Actions()->UnitCommand(tmp_killerworkers, ABILITY_ID::STOP);
			emergency_killerworkers.clear();
		}
	}
}