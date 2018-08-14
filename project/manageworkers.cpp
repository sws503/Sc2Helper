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

	bool noreassign = EnemyRush && !(branch == 6);
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

	std::vector<Vector2D> aux_vectors({ Vector2D(3,0), Vector2D(-3,0), Vector2D(0,3), Vector2D(0,-3) });

	// Search for a base that is missing mineral workers.
	if (has_space_for_half_mineral && !noreassign) {
		float min_distance = std::numeric_limits<float>::max();
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
			std::vector<QueryInterface::PathingQuery> query_vector;
			for (const auto& v : aux_vectors) {
				QueryInterface::PathingQuery query;
				query.start_unit_tag_ = worker->tag;
				query.start_ = worker->pos;
				query.end_ = target_base->pos + v;
				query_vector.push_back(query);
			}
			std::vector<float> results = Query()->PathingDistance(query_vector);
			for (const auto& d : results) {
				if (d > 0.01f) {
					Actions()->UnitCommand(worker, ABILITY_ID::HARVEST_GATHER, target_resource);
					return;
				}
			}
		}
	}

	// Search for a base that does not have full of gas workers.
	if (has_space_for_gas && !noreassign) {
		float min_distance = std::numeric_limits<float>::max();
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
			std::vector<QueryInterface::PathingQuery> query_vector;
			for (const auto& v : aux_vectors) {
				QueryInterface::PathingQuery query;
				query.start_unit_tag_ = worker->tag;
				query.start_ = worker->pos;
				query.end_ = target_base->pos + v;
				query_vector.push_back(query);
			}
			std::vector<float> results = Query()->PathingDistance(query_vector);
			for (const auto& d : results) {
				if (d > 0.01f) {
					Actions()->UnitCommand(worker, ABILITY_ID::HARVEST_GATHER, target_resource);
					return;
				}
			}
		}
	}

	// Search for a base that does not have full of mineral workers.
	if (has_space_for_mineral && !noreassign) {
		float min_distance = std::numeric_limits<float>::max();
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
			std::vector<QueryInterface::PathingQuery> query_vector;
			for (const auto& v : aux_vectors) {
				QueryInterface::PathingQuery query;
				query.start_unit_tag_ = worker->tag;
				query.start_ = worker->pos;
				query.end_ = target_base->pos + v;
				query_vector.push_back(query);
			}
			std::vector<float> results = Query()->PathingDistance(query_vector);
			for (const auto& d : results) {
				if (d > 0.01f) {
					Actions()->UnitCommand(worker, ABILITY_ID::HARVEST_GATHER, target_resource);
					return;
				}
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

	bool noreassign = EnemyRush && !(branch == 6);
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
	if (has_space_for_half_mineral && !noreassign) {
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
	if (has_space_for_gas && !noreassign) {
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

// Todo: 스킬 피하기, 공중유닛 피하기, disruptor, baneling 피하기, 작은 유닛 죽이기
void MEMIBot::FleeWorkers(const Unit* unit) {
	if (unit == nullptr) return;
	if (!ManyEnemyRush) {
		return;
	}
	const ObservationInterface* observation = Observation();
	Units workers = observation->GetUnits(Unit::Alliance::Self, IsWorker());

	Units nearworkers;
	for (const auto& worker : workers) {
		// consider near workers (distance < 10)
		if (DistanceSquared2D(unit->pos, worker->pos) > 100) continue;
		// do not consider killerworkers
		if (emergency_killerworkers.count(worker)) continue;

		nearworkers.push_back(worker);
	}
	Filter base_filter = [](const Unit& u) { return IsTownHall()(u) && u.build_progress >= 0.9; };

	const Unit* b = FindSecondNearestUnit(unit->pos, Unit::Alliance::Self, base_filter);
	if (b == nullptr) return;
	const Unit* m = FindNearestMineralPatch(b->pos);
	
	std::cout << nearworkers.size() << std::endl;
	Actions()->UnitCommand(nearworkers, ABILITY_ID::SMART, m);
	Print("MOVED!");
	return;
}

// 우선순위:
// 1. 병력이 1일 때 적 질럿, 저글링이 건물 주변이나 본진 내부 왔다: 프로브 적당히 끌고 와서 수비.
// 병력 수비 시작 범위: d 40 이내이고 건물 근처, 또는 본진 내부.
// 2. 병력이 0일 때 적 질럿, 저글링이 막을 수 있을 만큼 본진 안으로 왔다: 프로브 전부 끌고 와서 수비 (마중 ㄴㄴ 본진에서만 수비).
// 병력 수비 시작 범위: 본진 내부.
// 3. 병력이 1 이하일 때 일꾼러시가 왔다: 프로브 끌고 와서 수비
// 초반 적 프로브, 적 건물 부수러 가는 범위: d 40 이내
// 4. 병력이 1 또는 0이고 적 러시가 없고 건물 수비 범위 내에 적 건물이 있을 때 건물 터뜨리러 감
// 초반 적 프로브, 적 건물 부수러 가는 범위: d 40 이내
// 본진 범위: 본진 건물 높이 +-1 인 지형이고 d 40 이내
// 공식 : (일꾼 * 1 + 질럿 * 6 + 저글링 * 3 + 건물 * 6 + 벙커 * 3 + 1)

// todo : 0. 일꾼 피하기 (다 지어진 캐논하고 벙커하고 콜로니 피하기)
// 일꾼이 캐논 깨는 범위: 본진 15 이내. 다 달려 나와야됨.
// 일꾼이 캐논 깨는 범위: 본진 15 이내가 아니면 피하기.
void MEMIBot::ControlWorkers() {
	const ObservationInterface* observation = Observation();
	Units bases = observation->GetUnits(Unit::Alliance::Self, IsTownHall());
	Units workers = observation->GetUnits(Unit::Alliance::Self, IsWorker());
	Units my_armies = observation->GetUnits(Unit::Alliance::Self, IsArmy(observation));
	Units my_structures = observation->GetUnits(Unit::Alliance::Self, IsStructure(observation));
	Units enemy_units = FindUnitsNear(startLocation_, 40, Unit::Alliance::Enemy);
	Units enemy_structures = FindUnitsNear(startLocation_, 40, Unit::Alliance::Enemy, IsStructure(observation));
	Units enemy_rushers = FindUnitsNear(startLocation_, 15, Unit::Alliance::Enemy, CompletedRusher());
	size_t workers_size = workers.size();
	size_t my_armies_size = my_armies.size();
	Units enemy_units_killing_workers;

	Point3D mainbase_location = startLocation_;

	Filter filter_inbase = [mainbase_location](const Unit& u) {
		float diff = mainbase_location.z - u.pos.z;
		return -1.0f <= diff && diff <= 1.0f;
	};

	Filter filter_nearstructure = [my_structures](const Unit& u) {
		const float limit = 4.5;
		for (const auto& s : my_structures) {
			if (DistanceSquared2D(s->pos, u.pos) <= limit * limit)
				return true;
		}
		return false;
	};

	Filter filter_nearbase = [bases](const Unit& u) {
		const float limit = 10;
		for (const auto& b : bases) {
			if (DistanceSquared2D(b->pos, u.pos) <= limit * limit)
				return true;
		}
		return false;
	};
	
	int probecount = 1;
	bool situation_1 = false;
	bool situation_2 = false;
	bool situation_3 = false;
	bool situation_4 = false;

	
	for (const Unit*& i_eu : enemy_units) {
		const Unit& eu = *i_eu;
		bool inbase = filter_inbase(eu);
		bool nearbase = filter_nearbase(eu);
		bool nearstructure = filter_nearstructure(eu);

		// 공식 : (일꾼 * 1 + 질럿 * 6 + 저글링 * 3 + 건물 * 6 + 벙커 * 3 + 1)
		if (nearstructure || nearbase) {
			if (IsWorker()(eu)) {
				probecount += 1;
			}
			if (IsUnit(UNIT_TYPEID::PROTOSS_ZEALOT)(eu)) {
				probecount += 6;
			}
			if (IsUnit(UNIT_TYPEID::ZERG_ZERGLING)(eu)) {
				probecount += 3;
			}
			if (IsUnit(UNIT_TYPEID::TERRAN_BUNKER)(eu)) {
				probecount += 3;
			}
			if (IsStructure(observation)(eu)) {
				probecount += 6;
			}
		}
		// 1. 병력이 1일 때 적 질럿, 저글링이 건물 주변이나 본진 내부 왔다: 프로브 적당히 끌고 와서 수비.
		situation_1 |= (nearstructure || inbase) &&
			IsUnits({ UNIT_TYPEID::PROTOSS_ZEALOT, UNIT_TYPEID::ZERG_ZERGLING })(eu);

		// 2. 병력이 0일 때 적 질럿, 저글링이 막을 수 있을 만큼 본진 안으로 왔다: 프로브 전부 끌고 와서 수비 (본진에서만 수비).
		situation_2 |= inbase &&
			IsUnits({ UNIT_TYPEID::PROTOSS_ZEALOT, UNIT_TYPEID::ZERG_ZERGLING })(eu);

		// 3. 병력이 1 이하일 때 일꾼러시가 왔다: 프로브 끌고 와서 수비
		situation_3 |= nearbase && IsWorker()(eu);

		// 4. 병력이 1 또는 0이고 적 러시가 없고 건물 수비 범위 내에 적 건물이 있을 때 건물 터뜨리러 감
		situation_4 |= nearbase && IsStructure(observation)(eu);
	}

	if (my_armies_size == 0) {
		situation_1 = false;
	}
	if (my_armies_size > 1) {
		situation_1 = false;
		situation_2 = false;
		situation_3 = false;
		situation_4 = false;
	}

	bool should_defend = (situation_1 || situation_2 || situation_3 || situation_4);

	// push
	if (should_defend)
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

		bool need_all_probes = !situation_1 && situation_2;
		int32_t workers_short = static_cast<int32_t>(probecount - emergency_killerworkers.size());

		// push alive
		if (workers_short > 0 || need_all_probes) {
			for (const auto& worker : workers) {
				if (emergency_killerworkers.count(worker)) continue;
				if (probe_forward != nullptr && !work_probe_forward && worker->tag == probe_forward->tag) continue;
				if (probe_scout != nullptr && probe_scout->tag == worker->tag) continue;
				const Unit* target = GetTarget(worker, enemy_units_killing_workers);
				if (target == nullptr) continue;
				emergency_killerworkers.insert(worker);
				workers_short--;
				if (workers_short <= 0 && !need_all_probes) break;
			}
		}
		
		// todo: priority 지정
		// attack
		for (const auto& u : workers)
		{
			if (probe_forward != nullptr && !work_probe_forward && u->tag == probe_forward->tag) continue;
			if (probe_scout != nullptr && probe_scout->tag == u->tag) continue;
			if (EvadeEffect(u)) {}
			else if (EvadeExplosiveUnits(u)) {}
			else
			{
				if (emergency_killerworkers.count(u))
				{
					const Unit* killerworker = u;
					const Unit* target = GetTarget(killerworker, enemy_units_killing_workers);
					SmartAttackUnit(killerworker, target);
				}
				else {
					// harvest
				}
			}
		}
	}
	// pop
	else
	{
		if (!emergency_killerworkers.empty()) {
			Units tmp_killerworkers;
			tmp_killerworkers.reserve(emergency_killerworkers.size());
			for (const auto& killerworker : emergency_killerworkers) {
				if (killerworker->is_alive) {
					tmp_killerworkers.push_back(killerworker);
				}
			}
			Actions()->UnitCommand(tmp_killerworkers, ABILITY_ID::STOP);
			emergency_killerworkers.clear();
		}

		for (const auto& u : workers) {
			if (probe_forward != nullptr && !work_probe_forward && u->tag == probe_forward->tag) continue;
			if (probe_scout != nullptr && probe_scout->tag == u->tag) continue;
			if (EvadeEffect(u)) {}
			else if (EvadeExplosiveUnits(u)){}
			else {
				// harvest
			}
		}
	}

	std::cout << "prbecount 이 " << probecount << " 입니다" << std::endl;
	std::cout << "situation_1 이 " << situation_1 << " 입니다" << std::endl;
	std::cout << "situation_2 이 " << situation_2 << " 입니다" << std::endl;
	std::cout << "situation_3 이 " << situation_3 << " 입니다" << std::endl;
	std::cout << "situation_4 이 " << situation_4 << " 입니다" << std::endl;

}

bool MEMIBot::EvadeExplosiveUnits(const Unit* unit) {
	const ObservationInterface* observation = Observation();
	Units explodingunits = observation->GetUnits(Unit::Alliance::Enemy,
		IsUnits({ UNIT_TYPEID::TERRAN_WIDOWMINE, UNIT_TYPEID::TERRAN_WIDOWMINEBURROWED, UNIT_TYPEID::PROTOSS_DISRUPTORPHASED, UNIT_TYPEID::ZERG_BANELING, UNIT_TYPEID::ZERG_BANELINGBURROWED }));
	const Unit* nearestu = FindNearestUnit(unit->pos, explodingunits, 5.0);
	if (nearestu != nullptr) {
		Point2D pos = nearestu->pos;
		Vector2D diff = unit->pos - pos; // 7.3 적 유닛과의 반대 방향으로 도망
		Normalize2D(diff);
		Vector2D mul_diff = diff * (2.5f /*+ radius + unit->radius - dist*/);
		Point2D fleeingPos = unit->pos + mul_diff;
		SmartMove(unit, fleeingPos);
		Print("Enemy exploding units Run~");
		return true;
	}
	return false;
}
