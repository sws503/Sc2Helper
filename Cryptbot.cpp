#include <sc2api/sc2_api.h>
#include "sc2lib/sc2_lib.h"
#include <sc2utils/sc2_manage_process.h>

//#include "bot_examples.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <random>
#include <iterator>
#include <typeinfo>
#include <map>

using namespace sc2;

// Control 시작
namespace sc2 // 7.3 안좋은 효과들 목록
{

	enum class EFFECT_ID;
	typedef SC2Type<EFFECT_ID>  EffectID;
	enum class EFFECT_ID
	{
		INVALID = 0,
		PSISTORM = 1,
		GUARDIANSHIELD = 2,
		TEMPORALFIELDGROWING = 3,
		TEMPORALFIELD = 4,
		THERMALLANCES = 5,
		SCANNERSWEEP = 6,
		NUKEDOT = 7,
		LIBERATORMORPHING = 8,
		LIBERATORMORPHED = 9,
		BLINDINGCLOUD = 10,
		CORROSIVEBILE = 11,
		LURKERATTACK = 12
	};
}
// Control 끝

class Bot : public Agent {
public:
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
	struct ExpansionParameters {
		// Some nice parameters that generally work but may require tuning for certain maps.
		ExpansionParameters() :
			radiuses_({ 6.4f, 5.3f }),
			circle_step_size_(0.5f),
			cluster_distance_(15.0f),
			debug_(nullptr) {
		}

		// The various radius to check at from the center of an expansion.
		std::vector<float> radiuses_;

		// With what granularity to step the circumference of the circle.
		float circle_step_size_;

		// With what distance to cluster mineral/vespene in, this will be used for center of mass calulcation.
		float cluster_distance_;

		// If filled out CalculateExpansionLocations will render spheres to show what it calculated.
		DebugInterface* debug_;
	};


	

	size_t CalculateQueries(float radius, float step_size, const Point2D& center, std::vector<QueryInterface::PlacementQuery>& queries) {
		static const float PI = 3.1415927f;
		Point2D current_grid, previous_grid(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		size_t valid_queries = 0;
		// Find a buildable location on the circumference of the sphere
		float loc = 0.0f;
		while (loc < 360.0f) {
			Point2D point = Point2D(
				(radius * std::cos((loc * PI) / 180.0f)) + center.x,
				(radius * std::sin((loc * PI) / 180.0f)) + center.y);

			QueryInterface::PlacementQuery query(ABILITY_ID::BUILD_COMMANDCENTER, point);

			current_grid = Point2D(floor(point.x), floor(point.y));

			if (previous_grid != current_grid) {
				queries.push_back(query);
				++valid_queries;
			}

			previous_grid = current_grid;
			loc += step_size;
		}

		return valid_queries;
	}


	// Clusters units within some distance of each other and returns a list of them and their center of mass.
	std::vector<std::pair<Point3D, std::vector<Unit> > > Cluster(const Units& units, float distance_apart) {
		float squared_distance_apart = distance_apart * distance_apart;
		std::vector<std::pair<Point3D, std::vector<Unit> > > clusters;
		for (size_t i = 0, e = units.size(); i < e; ++i) {
			const Unit& u = *units[i];

			float distance = std::numeric_limits<float>::max();
			std::pair<Point3D, std::vector<Unit> >* target_cluster = nullptr;
			// Find the cluster this mineral patch is closest to.
			for (auto& cluster : clusters) {
				float d = DistanceSquared3D(u.pos, cluster.first);
				if (d < distance) {
					distance = d;
					target_cluster = &cluster;
				}
			}

			// If the target cluster is some distance away don't use it.
			if (distance > squared_distance_apart) {
				clusters.push_back(std::pair<Point3D, std::vector<Unit> >(u.pos, std::vector<Unit>{u}));
				continue;
			}

			// Otherwise append to that cluster and update it's center of mass.
			target_cluster->second.push_back(u);
			size_t size = target_cluster->second.size();
			target_cluster->first = ((target_cluster->first * (float(size) - 1)) + u.pos) / float(size);
		}

		return clusters;
	}

	std::vector<Point3D> TempCalculateExpansionLocations(const ObservationInterface* observation, QueryInterface* query, ExpansionParameters parameters=ExpansionParameters()) {
		Units resources = observation->GetUnits(
			[](const Unit& unit) {
			return unit.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD750 ||
				unit.unit_type == UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD750 ||
				unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERMINERALFIELD750 ||
				unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERRICHMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERRICHMINERALFIELD750 ||
				unit.unit_type == UNIT_TYPEID::NEUTRAL_LABMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_LABMINERALFIELD750 ||
				unit.unit_type == UNIT_TYPEID::NEUTRAL_BATTLESTATIONMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_BATTLESTATIONMINERALFIELD750 ||
				unit.unit_type == UNIT_TYPEID::NEUTRAL_VESPENEGEYSER || unit.unit_type == UNIT_TYPEID::NEUTRAL_PROTOSSVESPENEGEYSER ||
				unit.unit_type == UNIT_TYPEID::NEUTRAL_SPACEPLATFORMGEYSER || unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERVESPENEGEYSER ||
				unit.unit_type == UNIT_TYPEID::NEUTRAL_SHAKURASVESPENEGEYSER || unit.unit_type == UNIT_TYPEID::NEUTRAL_RICHVESPENEGEYSER;
		}
		);

		std::vector<Point3D> expansion_locations;
		std::vector<std::pair<Point3D, std::vector<Unit> > > clusters = Cluster(resources, parameters.cluster_distance_);

		std::vector<size_t> query_size;
		std::vector<QueryInterface::PlacementQuery> queries;
		for (size_t i = 0; i < clusters.size(); ++i) {
			std::pair<Point3D, std::vector<Unit> >& cluster = clusters[i];
			if (parameters.debug_) {
				for (auto r : parameters.radiuses_) {
					parameters.debug_->DebugSphereOut(cluster.first, r, Colors::Green);
				}
			}

			// Get the required queries for this cluster.
			size_t query_count = 0;
			for (auto r : parameters.radiuses_) {
				query_count += CalculateQueries(r, parameters.circle_step_size_, cluster.first, queries);
			}

			query_size.push_back(query_count);
		}

		std::vector<bool> results = query->Placement(queries);
		size_t start_index = 0;
		for (int i = 0; i < clusters.size(); ++i) {
			std::pair<Point3D, std::vector<Unit> >& cluster = clusters[i];
			float distance = std::numeric_limits<float>::max();
			Point2D closest;

			// For each query for the cluster minimum distance location that is valid.
			for (size_t j = start_index, e = start_index + query_size[i]; j < e; ++j) {
				if (!results[j]) {
					continue;
				}

				Point2D& p = queries[j].target_pos;

				float d = Distance2D(p, cluster.first);
				if (d < distance) {
					distance = d;
					closest = p;
				}
			}

			Point3D expansion(closest.x, closest.y, cluster.second.begin()->pos.z);

			if (parameters.debug_) {
				parameters.debug_->DebugSphereOut(expansion, 0.35f, Colors::Red);
			}

			expansion_locations.push_back(expansion);
			start_index += query_size[i];
		}

		return expansion_locations;
	}

	void PrintPoint3D(Point3D p) {
		//std::cout << "(" << p.x << ", " << p.y << ", " << p.z << ")" << std::endl;
	}
	virtual void OnGameStart() final {
		game_info_ = Observation()->GetGameInfo();
		std::cout << "Game started!" << std::endl;
		expansions_ = TempCalculateExpansionLocations(Observation(), Query());

		iter_exp = expansions_.begin();

		//Temporary, we can replace this with observation->GetStartLocation() once implemented
		startLocation_ = Observation()->GetStartLocation();
		staging_location_ = startLocation_;

		PrintPoint3D(startLocation_);
		if (game_info_.enemy_start_locations.size() == 1) find_enemy_location = true;

		for (auto e : expansions_) {
			PrintPoint3D(e);
		}

		float minimum_distance = std::numeric_limits<float>::max();
		for (const auto& expansion : expansions_) {
			float current_distance = Distance2D(startLocation_, expansion);
			if (current_distance < .01f) {
				continue;
			}

			if (current_distance < minimum_distance) {
				if (Query()->Placement(ABILITY_ID::BUILD_NEXUS, expansion)) {
					front_expansion = expansion;
					minimum_distance = current_distance;
				}
			}
		}
		staging_location_ = Point3D(((staging_location_.x + front_expansion.x) / 2), ((staging_location_.y + front_expansion.y) / 2),
			((staging_location_.z + front_expansion.z) / 2));
	}

	virtual void OnStep() final {
		const ObservationInterface* observation = Observation();
		Units units = observation->GetUnits(Unit::Self, IsArmy(observation));

		ManageWorkers(UNIT_TYPEID::PROTOSS_PROBE, ABILITY_ID::HARVEST_GATHER, UNIT_TYPEID::PROTOSS_ASSIMILATOR);


		if (!early_strategy) {
			EarlyStrategy();
		}
		if (iter_exp < expansions_.end() && find_enemy_location == true) {
			scoutprobe();
		}

		ManageUpgrades();

		// Control 시작
		Defend();
		//ManageArmy();
		ManageRush();
		// Control 끝
	}

	virtual void OnUnitIdle(const Unit* unit) override {
		const ObservationInterface* observation = Observation();

		switch (unit->unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_PROBE: {
			MineIdleWorkers(unit, ABILITY_ID::HARVEST_GATHER, UNIT_TYPEID::PROTOSS_ASSIMILATOR);
			break;
		}
		case UNIT_TYPEID::PROTOSS_CARRIER: {
			ScoutWithUnit(unit, observation);
			break;
		}
		default: {
			break;
		}
		}
		return;
	}

	GameInfo game_info_;
	std::vector<Point3D> expansions_;
	Point3D startLocation_;
	Point3D staging_location_;

	Point3D front_expansion;

	// Control 시작
	int max_worker_count_ = 70;

	//When to start building attacking units
	int target_worker_count_ = 15;

	bool warpgate_reasearched_ = false;
	bool blink_reasearched_ = false;
	bool Carrier_researched = false;
	int max_colossus_count_ = 5;
	int max_sentry_count_ = 2;
	int max_stalker_count_ = 20;
	int Rushtime = 0;

	sc2::Point2D RushLocation;
	sc2::Point2D EnemyLocation;
	sc2::Point2D ReadyLocation1;
	sc2::Point2D ReadyLocation2;
	sc2::Point2D EscapeLocation;
	sc2::Point2D KitingLocation;
	uint64_t RushUnitTag;

	bool OracleCanAttack = false;
	int OracleCount = 0;
	int TempestCount = 0;
	int OracleStop = 0;
	int CarrierCount = 0;
	// Control 끝

private:
	// Control 시작
	void Chat(std::string Message) // 6.29 채팅 함수
	{
		Actions()->SendChat(Message);
	}

	const std::vector<sc2::Effect> effects = Observation()->GetEffects();

	const bool isBadEffect(const sc2::EffectID id)
	{
		switch (id.ToType())
		{
		case sc2::EFFECT_ID::BLINDINGCLOUD:
		case sc2::EFFECT_ID::CORROSIVEBILE:
		case sc2::EFFECT_ID::LIBERATORMORPHED:
		case sc2::EFFECT_ID::LIBERATORMORPHING:
		case sc2::EFFECT_ID::LURKERATTACK:
		case sc2::EFFECT_ID::NUKEDOT:
		case sc2::EFFECT_ID::PSISTORM:
			//case sc2::EFFECT_ID::THERMALLANCES:
			return true;
		}
		return false;
	}

	void EvadeEffect(Units unit)
	{
		for (auto & unit : unit)
		{
			for (const auto & effect : effects)
			{
				if (isBadEffect(effect.effect_id))
				{
					const float radius = Observation()->GetEffectData()[effect.effect_id].radius;
					for (const auto & pos : effect.positions)
					{
						const float dist = Distance2D(unit->pos, pos);
						if (dist < radius + 2.0f)
						{
							sc2::Point2D fleeingPos;
							if (dist > 0)
							{
								Vector2D diff = unit->pos - pos; // 7.3 적 유닛과의 반대 방향으로 도망
								Normalize2D(diff);
								fleeingPos = unit->pos + diff * radius;
								//fleeingPos = pos + normalizeVector(rangedUnit->getPos() - pos, radius + 2.0f);
							}
							else
							{
								fleeingPos = pos + sc2::Point2D(0.1f, 0.1f);
							}
							Actions()->UnitCommand(unit, ABILITY_ID::MOVE, fleeingPos);
							Chat("Enemy Skill Run~");
							break;
						}
					}
				}
			}
		}
	}

	void EvadeEffect(const Unit* unit)
	{
		for (const auto & effect : effects)
		{
			if (isBadEffect(effect.effect_id))
			{
				const float radius = Observation()->GetEffectData()[effect.effect_id].radius;
				for (const auto & pos : effect.positions)
				{
					const float dist = Distance2D(unit->pos, pos);
					if (dist < radius + 2.0f)
					{
						sc2::Point2D fleeingPos;
						if (dist > 0)
						{
							Vector2D diff = unit->pos - pos; // 7.3 적 유닛과의 반대 방향으로 도망
							Normalize2D(diff);
							fleeingPos = unit->pos + diff * radius;
							//fleeingPos = pos + normalizeVector(rangedUnit->getPos() - pos, radius + 2.0f);
						}
						else
						{
							fleeingPos = pos + sc2::Point2D(0.1f, 0.1f);
						}
						Actions()->UnitCommand(unit, ABILITY_ID::MOVE, fleeingPos);
						Chat("Enemy Skill Run~");
						break;
					}
				}
			}
		}
	}

	void SetupRushLocation(const ObservationInterface *observation)
	{
		if (find_enemy_location) {
			ReadyLocation1 = game_info_.enemy_start_locations.front() + Point2D(30.0f, 0.0f);
			ReadyLocation2 = game_info_.enemy_start_locations.front() + Point2D(0.0f, 30.0f);
		}
		else
			ReadyLocation1 = startLocation_;
		ReadyLocation2 = startLocation_;
	}

	float OracleRange = 2.5f; // 절대적으로 생존
	float TempestRange = 3.0f;
	float CarrierRange = 1.9f;
	bool TimetoAttack = false;
	bool OracleTrained = false;
	Point2D pylonlocation;
	const Unit* oracle_second = nullptr;
	Point2D StasisLocation;

	void Defend() { // 유닛 포인터 오류
		const ObservationInterface* observation = Observation();
		Units Oracles = observation->GetUnits(Unit::Alliance::Self, IsOracle());
		Units nexus = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));
		int CurrentOracle = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ORACLE);

		Point2D StasisLocation;
		bool selected = false;

		/*
		if (Oracles.empty())
		return;
		else if (!Oracles.empty() && CurrentOracle > 1 && !selected) {
		//Vector2D diff = pylon_first->pos - nexus.at(1)->pos; // 7.3 적 유닛과의 반대 방향으로 도망
		//Normalize2D(diff);
		//StasisLocation = pylon_first->pos + diff * 1.0f;
		float rx = GetRandomScalar();
		float ry = GetRandomScalar();
		StasisLocation = Point2D(pylonlocation.x + rx * 5, pylonlocation.y + ry * 5);
		Chat("HI~ I'm Second");
		oracle_second = Oracles.at(1);
		selected = true;
		}
		else if (oracle_second->energy > 51)
		{
		Chat("OK~");
		Actions()->UnitCommand(oracle_second, ABILITY_ID::BUILD_STASISTRAP, StasisLocation);
		}
		else
		{
		ScoutWithUnit(oracle_second, observation);
		}
		*/

		Units Workers = observation->GetUnits(Unit::Alliance::Self, IsWorker());
		Units EnemyWorkers = observation->GetUnits(Unit::Alliance::Enemy, IsWorker());
		int cannon_count = observation->GetUnits(Unit::Alliance::Enemy, Rusher()).size();
		int EnemyWorkercount = observation->GetUnits(Unit::Alliance::Enemy, IsWorker()).size();
		Units EnemyCannon = observation->GetUnits(Unit::Alliance::Enemy, Rusher());


		bool killscouter = false;
		if (!OracleTrained)            killscouter = true;
		else                             killscouter = false;


		if (killscouter)
		{
			if (EnemyWorkercount >= 1)
			{
				if (WorkerKiller == nullptr || !WorkerKiller->is_alive) {
					GetRandomUnit(WorkerKiller, observation, UNIT_TYPEID::PROTOSS_PROBE);
					if (WorkerKiller == probe_scout || WorkerKiller == probe_forge) WorkerKiller = nullptr;
				}
				Actions()->UnitCommand(WorkerKiller, ABILITY_ID::ATTACK, EnemyWorkers.front()->pos);
				Chat("I want chase enemy scout worker~ *^^*");
			}
			else if (EnemyWorkercount > 1)
			{
				for (int i = 0; i < EnemyWorkercount + 1; i++) {
					Actions()->UnitCommand(Workers.at(i), ABILITY_ID::ATTACK, EnemyWorkers.front()->pos);
				}
				Chat("Enemy's Cheese Rush~!!!! ^0^");
			}

			if (!EnemyCannon.empty())
			{
				/*
				if (Killers.size()<8) Killers.resize(8);
				for (int i = 0; i <= 8; i++)
				{
				const Unit * Killer = Killers.at(i);
				if (Killers.at(i) == nullptr || !Killer->is_alive)
				{
				Chat("I'm Chosen!");
				GetRandomUnit(Killers.at(i), observation, UNIT_TYPEID::PROTOSS_PROBE);
				}
				}

				for(const auto& unit : Killers)
				Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyCannon.front()->pos);


				for (int i = 0; i < 8; i++) {
				Actions()->UnitCommand(Workers.at(i), ABILITY_ID::ATTACK, EnemyCannon.front()->pos);
				}*/

				for (const auto& worker : Workers) {
					Actions()->UnitCommand(worker, ABILITY_ID::ATTACK, EnemyCannon.front()->pos);
				}
			}
		}
	}

	Units Killers;
	const Unit* WorkerKiller = nullptr;
	const Unit* oracle_first = nullptr;

	void ManageRush() { // 5.17 오라클 유닛 관리 +6.25 폭풍함 유닛 관리
		const ObservationInterface* observation = Observation();
		Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
		Units Oracles = observation->GetUnits(Unit::Alliance::Self, IsOracle());
		Units Tempests = observation->GetUnits(Unit::Alliance::Self, IsTempest()); //6.25 폭풍함 컨트롤 추가
		Units Carriers = observation->GetUnits(Unit::Alliance::Self, IsCarrier());
		Units EnemyWorker = observation->GetUnits(Unit::Alliance::Enemy, IsWorker());
		Units AirAttackers = observation->GetUnits(Unit::Alliance::Enemy, AirAttacker()); //적 방어 유닛 및 건물
																						  //Units ProxyEnemy = observation->GetUnits(Unit::Alliance::Enemy, ExceptBuilding());
		float rx = GetRandomScalar();
		float ry = GetRandomScalar();
		StasisLocation = Point2D(baselocation.x + rx * 10, baselocation.y + ry * 10);

		if (OracleTrained)
		{
			if (oracle_first == nullptr && !Oracles.empty())
			{
				Chat("HI~");
				oracle_first = Oracles.front();
			}
		}

		//const Unit* unit = oracle_first;
		//if (unit != nullptr) {
		for (const auto& unit : Oracles) {

			if (!unit->orders.empty()) { // 펄서광선  ON / OFF
				float distance = std::numeric_limits<float>::max();
				for (const auto& u : EnemyWorker) {
					float d = Distance2D(u->pos, unit->pos);
					if (d < distance) {
						distance = d;
					}
				}
				if (unit->energy == 1)
					OracleCanAttack = false;
				else if (distance < 6 && unit->energy >= 50) {
					Actions()->UnitCommand(unit, ABILITY_ID::BEHAVIOR_PULSARBEAMON);
					OracleCanAttack = true;
				}
				else if (distance > 20) {
					Actions()->UnitCommand(unit, ABILITY_ID::BEHAVIOR_PULSARBEAMOFF);
					OracleCanAttack = false;
				}
			}

			if (find_enemy_location == 1) {
				if (unit->energy > 50) {
					Actions()->UnitCommand(unit, ABILITY_ID::MOVE, game_info_.enemy_start_locations.front());

				}
				if (unit->energy <= 50 && !OracleCanAttack) {
					ScoutWithUnit(unit, observation);
				}
			}

			EvadeEffect(unit);

			float distance = std::numeric_limits<float>::max(); // 5.21 방어 건물,유닛이 근처로 가지 않는다
			float UnitAttackRange = getAttackRange(unit); // 7.3 이 유닛의 공격사정거리
			float TargetAttackRange = 0.0f; // 7.3 나를 공격할 수 있는 유닛의 공격 사정거리

			for (const auto& u : AirAttackers) {
				float d = Distance2D(u->pos, unit->pos);
				if (d < distance) {
					distance = d; // 가장 가까운 거리의 적을 고른다
				}

				float TargetAttackRange = getAttackRange(u);

				Vector2D diff = unit->pos - u->pos; // 7.3 적 유닛과의 반대 방향으로 도망
				Normalize2D(diff);
				KitingLocation = unit->pos + diff * 7.0f;

				if (u->unit_type == sc2::UNIT_TYPEID::PROTOSS_PHOTONCANNON || u->unit_type == sc2::UNIT_TYPEID::ZERG_SPORECRAWLER || u->unit_type == sc2::UNIT_TYPEID::TERRAN_MISSILETURRET)
				{
					TargetAttackRange = TargetAttackRange + 1.0f; // -OracleRange + 1.0f;
				}
				if (TargetAttackRange + OracleRange < distance) // 내가 공격할 수 있고 적 사거리보다 멀리 있을 때 공격한다
																// 7.5 OracleRange 는 안전거리 (적도 나를 향해 움직이기 때문)
				{
					for (const auto& Proxy1 : EnemyWorker) {
						//Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
						Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, Proxy1->pos);
						//Chat("Target Attack!"); 7.6 너무 시끄러움 ㅋㅋ
						break;
					}
				}
				else if (distance <= TargetAttackRange + OracleRange) {
					Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
					break;
				}


				else {
					for (const auto& Proxy1 : EnemyWorker) {
						//Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
						Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, Proxy1->pos);
						Chat("No AirAttacker~ Target Attack!");
						break;
					}
				}

			}



			/*
			if (distance < 11) {
			Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation); // 5.21 깔짝깔짝 대는걸 구현하고 싶다 // 5.24 구현이 안됨
			} //6.25 구현됨




			if (!EnemyWorker.empty() && OracleCanAttack && distance > 10.5) { //6.26 적
			for (const auto& Proxy1 : EnemyWorker) {
			//Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
			Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, Proxy1->pos);
			Chat("Target Attack!");
			}
			}
			*/
			/*
			else if (!ProxyEnemy.empty() && OracleCanAttack && distance > 10.5 && distance < 20) { //6.28 예언자가 건물때리는건 불필요하다
			for (const auto& Proxy2 : ProxyEnemy) {
			Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, ProxyEnemy.front()->pos);
			}
			}
			*/

			//}
		}





		for (const auto& unit : Tempests) {
			float distance = std::numeric_limits<float>::max(); // 6.25 폭풍함은 사거리를 활용해 방어 건물,유닛 근처로 가지 않는다
			float UnitAttackRange = getAttackRange(unit); // 7.3 이 유닛의 공격사정거리
			float TargetAttackRange = 0.0f; // 7.3 나를 공격할 수 있는 유닛의 공격 사정거리

			EvadeEffect(unit);

			for (const auto& u : AirAttackers) {
				float d = Distance2D(u->pos, unit->pos);
				if (d < distance) {
					distance = d;
				}

				float TargetAttackRange = getAttackRange(u);

				Vector2D diff = unit->pos - u->pos; // 7.3 적 유닛과의 반대 방향으로 도망
				Normalize2D(diff);
				KitingLocation = unit->pos + diff * 7.0f;

				if (unit->weapon_cooldown == 0.0f || TargetAttackRange + TempestRange < distance) // 내가 공격할 수 있고 적 사거리보다 멀리 있을 때 공격한다
				{

					for (const auto& Proxy2 : AirAttackers) {
						//Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
						Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, AirAttackers.front()->pos);

						break; // 타겟할 유닛을 찾고 찾으면 공격하는 걸로
					}

				}
				else if (distance <= TargetAttackRange + TempestRange) {
					Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
					break;
				}
				else
				{
					for (const auto& Proxy1 : EnemyWorker) {
						//Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
						Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, Proxy1->pos);

						break;
					}
				}
			}


			/*
			for (const auto& u : AirAttackers) {
			float d = Distance2D(u->pos, unit->pos);
			if (d < distance) {
			distance = d;
			}
			Vector2D diff = unit->pos - u->pos;
			Normalize2D(diff);
			KitingLocation = unit->pos + diff * 7.0f;
			}
			if (distance < 10) {
			Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
			}

			if (!AirAttackers.empty() && distance > 10) { //6.26 적
			for (const auto& Proxy2 : AirAttackers) {
			Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, AirAttackers.front()->pos);
			}
			}
			else if (!EnemyWorker.empty() && distance > 10 && distance < 20) { //6.26 적
			for (const auto& Proxy1 : EnemyWorker) {
			Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
			}
			}*/

		}

		for (const auto& unit : Carriers) {
			float distance = std::numeric_limits<float>::max(); // 6.25 캐리어 거리유지
			float UnitAttackRange = getAttackRange(unit); // 7.3 이 유닛의 공격사정거리
			float TargetAttackRange = 0.0f; // 7.3 나를 공격할 수 있는 유닛의 공격 사정거리

			int CurrentCarrier = CountUnitType(observation, UNIT_TYPEID::PROTOSS_CARRIER);

			EvadeEffect(unit);


			if (CurrentCarrier <= 3) {
				for (const auto& u : AirAttackers) {
					if (!u->is_alive)
					{
						continue;
					}

					float d = Distance2D(u->pos, unit->pos);
					if (d < distance) {
						distance = d;
					}

					float TargetAttackRange = getAttackRange(u);

					Vector2D diff = unit->pos - u->pos; // 7.3 적 유닛과의 반대 방향으로 도망
					Normalize2D(diff);
					KitingLocation = unit->pos + diff * 7.0f;
					if (unit->shield < 10)
					{
						if (unit->weapon_cooldown == 0.0f || distance > TargetAttackRange + 4.5) // 내가 공격할 수 있고 적 사거리보다 멀리 있을 때 공격한다
						{
							if (unit->orders.empty())
								RetreatWithCarrier(unit);
							break;
						}
						else if (distance <= TargetAttackRange + 4.5) {
							Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
							break;
						}
					}
					else
					{

						if (unit->weapon_cooldown == 0.0f || distance > TargetAttackRange + CarrierRange) // 내가 공격할 수 있고 적 사거리보다 멀리 있을 때 공격한다
						{
							if (unit->orders.empty())
								RetreatWithCarrier(unit);
							break;
						}
						else if (distance <= TargetAttackRange + CarrierRange) {
							Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
							break;
						}
					}
				}

			}
			else {
				OracleRange = 2.5f;
				TimetoAttack = true;
				if (AirAttackers.empty())
				{
					AttackWithUnit(unit, observation);
				}
				else if (!enemy_units.empty()) {
					for (const auto& u : AirAttackers) {
						if (!u->is_alive)
						{
							continue;
						}

						float d = Distance2D(u->pos, unit->pos);
						if (d < distance) {
							distance = d;
						}

						float TargetAttackRange = getAttackRange(u);

						Vector2D diff = unit->pos - u->pos; // 7.3 적 유닛과의 반대 방향으로 도망
						Normalize2D(diff);
						KitingLocation = unit->pos + diff * 7.0f;
						if (unit->shield < 10)
						{
							if (unit->weapon_cooldown == 0.0f || distance > TargetAttackRange + 4.5) // 내가 공격할 수 있고 적 사거리보다 멀리 있을 때 공격한다
							{
								for (const auto& Proxy2 : AirAttackers) {
									//Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
									Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, AirAttackers.front()->pos);
									break;
								}

							}
							else if (distance <= TargetAttackRange + 4.5) {
								Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
								break;
							}
							else {
								for (const auto& Proxy1 : EnemyWorker) {
									//Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
									Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
									break;
								}
							}
						}
						else
						{

							if (unit->weapon_cooldown == 0.0f || distance > TargetAttackRange + CarrierRange) // 내가 공격할 수 있고 적 사거리보다 멀리 있을 때 공격한다
							{
								for (const auto& Proxy2 : AirAttackers) {
									//Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
									Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, AirAttackers.front()->pos);
									break;
								}

							}
							else if (distance <= TargetAttackRange + CarrierRange) {
								Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
								break;
							}
							else {
								for (const auto& Proxy1 : EnemyWorker) {
									//Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
									Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
									break;
								}
							}
						}
					}
				}
				else // 지도상에 적 유닛이 아예 없는 상황에선 캐리어가 주도적으로 탐색해야함
				{
					ScoutWithUnit(unit, observation);
					scoutprobe();
				}
			}
			/*
			for (const auto& u : AirAttackers) {
			float d = Distance2D(u->pos, unit->pos);
			if (d < distance) {
			distance = d;
			}
			Vector2D diff = unit->pos - u->pos;
			Normalize2D(diff);
			KitingLocation = unit->pos + diff * 7.0f;
			}

			if (distance < 10) {
			Actions()->UnitCommand(unit, ABILITY_ID::MOVE, KitingLocation);
			}

			if (!AirAttackers.empty() && distance > 10) { //6.29
			for (const auto& Proxy2 : AirAttackers) {
			Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, AirAttackers.front()->pos);
			}
			}
			else if (!EnemyWorker.empty() && distance >= 10 && distance < 20) { //6.29
			for (const auto& Proxy1 : EnemyWorker) {
			Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, EnemyWorker.front()->pos);
			}
			}
			*/
		}
	}

	/*
	Point3D CarrierLocation;

	void Location()
	{
	const ObservationInterface* observation = Observation();
	Units nexus = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));

	if (nexus.empty()) return;
	else {
	for (int i = 0; i < nexus.size();) {
	if (i < nexus.size()) {
	CarrierLocation = nexus.at(i)->pos;
	if (observation->GetVespene() > 200) {
	i++;
	}
	}
	}
	}
	}
	*/
	void RetreatWithCarrier(const Unit* unit) {
		Actions()->UnitCommand(unit, ABILITY_ID::PATROL, pylonlocation);

		/*Location();
		float dist = Distance2D(unit->pos, CarrierLocation);

		if (dist < 10) {
		if (unit->orders.empty()) {
		return;
		}
		Actions()->UnitCommand(unit, ABILITY_ID::STOP);
		return;
		}

		if (unit->orders.empty() && dist > 14) {
		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, CarrierLocation);
		}
		else if (!unit->orders.empty() && dist > 14) {
		if (unit->orders.front().ability_id != ABILITY_ID::MOVE) {
		Actions()->UnitCommand(unit, ABILITY_ID::MOVE, CarrierLocation);
		}
		}*/
	}


	const float getAttackRange(const Unit* target) const
	{
		sc2::Weapon groundWeapons;
		sc2::Weapon AirWeapons;

		for (const auto & Weapon : Observation()->GetUnitTypeData()[target->unit_type].weapons)
		{
			if (Weapon.type == sc2::Weapon::TargetType::Air || Weapon.type == sc2::Weapon::TargetType::Any)
			{
				AirWeapons = Weapon;
			}
			if ((Weapon.type == sc2::Weapon::TargetType::Ground || Weapon.type == sc2::Weapon::TargetType::Any) && Weapon.range > groundWeapons.range)//Siege tanks
			{
				groundWeapons = Weapon;
				if (groundWeapons.range < 0.11f)//melee. Not exactly 0.1
				{
					groundWeapons.range += target->radius;
				}
			}
		}
		return AirWeapons.range; // 7.5 이건 오로지 공중유닛을 위한 함수!!
								 // return groundWeapons.range; // 7.5 사용하고 싶으면 쓸것
	}

	struct Rusher {
		bool operator()(const Unit& unit) {
			switch (unit.unit_type.ToType()) {
			case UNIT_TYPEID::PROTOSS_PHOTONCANNON: return true;

			default: return false;
			}
		}
	};

	struct AirAttacker { // 공중 공격 가능한 적들 (예언자가 기피해야하는 적 && 폭풍함이 우선 공격하는 적) //시간이 남으면 weapon.type == sc2::Weapon::TargetType::Air 으로 할수있지만 시간이 없음
		bool operator()(const Unit& unit) {
			switch (unit.unit_type.ToType()) {

			case UNIT_TYPEID::PROTOSS_STALKER: return true;
			case UNIT_TYPEID::PROTOSS_PHOTONCANNON: return true;

			case UNIT_TYPEID::TERRAN_MARINE: return true;
			case UNIT_TYPEID::TERRAN_MISSILETURRET: return true;
			case UNIT_TYPEID::TERRAN_BUNKER: return true;

			case UNIT_TYPEID::ZERG_SPORECRAWLER: return true;
			case UNIT_TYPEID::ZERG_QUEEN: return true;

			case UNIT_TYPEID::TERRAN_BATTLECRUISER:
			case UNIT_TYPEID::TERRAN_CYCLONE:
			case UNIT_TYPEID::TERRAN_GHOST:
			case UNIT_TYPEID::TERRAN_LIBERATOR:
			case UNIT_TYPEID::TERRAN_THOR:
			case UNIT_TYPEID::TERRAN_VIKINGASSAULT:
			case UNIT_TYPEID::TERRAN_VIKINGFIGHTER:
			case UNIT_TYPEID::TERRAN_WIDOWMINEBURROWED:
			case UNIT_TYPEID::TERRAN_WIDOWMINE:

			case UNIT_TYPEID::ZERG_HYDRALISK:
			case UNIT_TYPEID::ZERG_MUTALISK:
			case UNIT_TYPEID::ZERG_INFESTORTERRAN:
			case UNIT_TYPEID::ZERG_CORRUPTOR:


			case UNIT_TYPEID::PROTOSS_TEMPEST:
			case UNIT_TYPEID::PROTOSS_VOIDRAY:
			case UNIT_TYPEID::PROTOSS_PHOENIX:
			case UNIT_TYPEID::PROTOSS_CARRIER:
			case UNIT_TYPEID::PROTOSS_SENTRY:
			case UNIT_TYPEID::PROTOSS_ARCHON: return true;

			default: return false;
			}
		}
	};


	struct IsOracle { // 예언자인지 감지
		bool operator()(const Unit& unit) {

			switch (unit.unit_type.ToType()) {
			case UNIT_TYPEID::PROTOSS_ORACLE: return true;
			default: return false;
			}
		}
	};

	struct IsTempest {
		bool operator()(const Unit& unit) {
			switch (unit.unit_type.ToType()) {
			case UNIT_TYPEID::PROTOSS_TEMPEST: return true;
			default: return false;
			}
		}
	};

	struct IsCarrier {
		bool operator()(const Unit& unit) {
			switch (unit.unit_type.ToType()) {
			case UNIT_TYPEID::PROTOSS_CARRIER: return true;
			default: return false;
			}
		}
	};

	/*
	void ManageArmy() {
	const ObservationInterface* observation = Observation();
	Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
	Units army = observation->GetUnits(Unit::Alliance::Self, IsArmy(observation));
	// 유닛 모으는 기준

	//There are no enemies yet, and we don't have a big army
	// 인구수 50이 되기 전까지는 유닛을 모으고 있는다.
	if (enemy_units.empty() && !TimetoAttack) {
	for (const auto& unit : army) {
	RetreatWithUnit(unit, staging_location_);
	}
	}
	else if (!enemy_units.empty() && TimetoAttack) {
	for (const auto& unit : army) {
	AttackWithUnit(unit, observation); // 인구수를 다 채웠다면 공격하러간다

	switch (unit->unit_type.ToType()) {

	case(UNIT_TYPEID::PROTOSS_TEMPEST): {
	}
	case(UNIT_TYPEID::PROTOSS_CARRIER): {

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
	*/

	void AttackWithUnitType(UnitTypeID unit_type, const ObservationInterface* observation) {
		Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
		for (const auto& unit : units) {
			AttackWithUnit(unit, observation);
		}
	}

	void AttackWithUnit(const Unit* unit, const ObservationInterface* observation) {
		//If unit isn't doing anything make it attack.
		Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy);
		if (enemy_units.empty()) {
			return;
		}

		// 유닛이 하는게 없을 때 공격
		if (unit->orders.empty()) {
			Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, enemy_units.front()->pos);
			return;
		}

		//If the unit is doing something besides attacking, make it attack. // 공격을 안하면 공격명령
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

	// Control 끝

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
			case UNIT_TYPEID::PROTOSS_ORACLE: return false;
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

	struct IsTownHall {
		bool operator()(const Unit& unit) {

			switch (unit.unit_type.ToType()) {
			case UNIT_TYPEID::ZERG_HATCHERY: return true;
			case UNIT_TYPEID::ZERG_LAIR: return true;
			case UNIT_TYPEID::ZERG_HIVE: return true;
			case UNIT_TYPEID::TERRAN_COMMANDCENTER: return true;
			case UNIT_TYPEID::TERRAN_ORBITALCOMMAND: return true;
			case UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING: return true;
			case UNIT_TYPEID::TERRAN_PLANETARYFORTRESS: return true;
			case UNIT_TYPEID::PROTOSS_NEXUS: return true;
			default: return false;
			}
		}
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

	int CountUnitType(const ObservationInterface* observation, UnitTypeID unit_type) {
		return observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
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
	// Control 시작
	bool TryBuildUnitChrono(AbilityID ability_type_for_unit, UnitTypeID unit_type) {
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
		Chronoboost(unit);
		return true;
	}

	bool Chronoboost(const Unit * unit) {
		const ObservationInterface* observation = Observation();
		Units nexus = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));

		if (nexus.empty()) return false;
		else {
			for (int i = 0; i < nexus.size(); ++i) {
				if (nexus.at(i)->build_progress != 1) {
					continue;
				}
				else {
					if (i < nexus.size()) {
						if (nexus.at(i)->energy >= 50) {
							Actions()->UnitCommand(nexus.at(i), ABILITY_ID::EFFECT_CHRONOBOOST, unit);
							return true;
						}
						else
						{
							return false;
							//Chat("Not enough Energy for Chronoboost~"); Too loud
						}
					}
				}
			}
			return false;
		}
	}
	// Control 끝
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
			if (worker == probe_scout) continue;
			for (const auto& order : worker->orders) {
				if (order.ability_id == ABILITY_ID::HARVEST_GATHER) {
					unit = worker;
					break;
				}
			}
			if (unit != nullptr) break;
		}
		if (unit == nullptr) return false;

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

	bool TryBuildStructure(AbilityID ability_type_for_structure, UnitTypeID unit_type, Tag location_tag) {
		const ObservationInterface* observation = Observation();
		Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
		const Unit* target = observation->GetUnit(location_tag);

		if (workers.empty()) {
			return false;
		}

		// Check to see if there is already a worker heading out to build it

		if (ability_type_for_structure != ABILITY_ID::BUILD_ASSIMILATOR) {
			for (const auto& worker : workers) {
				if (worker == probe_scout || worker == probe_forge) continue;
				for (const auto& order : worker->orders) {
					if (order.ability_id == ability_type_for_structure) {
						return false;
					}
				}
			}
		}

		// If no worker is already building one, get a random worker to build one
		const Unit* unit = GetRandomEntry(workers);

		// Check to see if unit can build there
		if (Query()->Placement(ability_type_for_structure, target->pos)) {
			Actions()->UnitCommand(unit, ability_type_for_structure, target);
			return true;
		}
		return false;

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

	bool TryBuildStructureNearPylonWithUnit(const Unit* unit, AbilityID ability_type_for_structure, const Unit* pylon) {

		const ObservationInterface* observation = Observation();

		if (unit == nullptr) return false;
		if (pylon == nullptr) return false;
		std::vector<PowerSource> power_sources = observation->GetPowerSources();
		if (power_sources.empty()) {
			return false;
		}
		float radius = power_sources.front().radius;
		float rx = GetRandomScalar();
		float ry = GetRandomScalar();
		Point2D location = Point2D(pylon->pos.x + rx * radius, pylon->pos.y + ry * radius);

		// Check to see if unit can make it there
		if (Query()->PathingDistance(unit, location) < 0.1f) {
			return false;
		}
		// Check to see if unit can build there
		if (Query()->Placement(ability_type_for_structure, location)) {
			Actions()->UnitCommand(unit, ability_type_for_structure, location);
			return true;
		}
		return false;

	}

	bool TryBuildForge(const Unit* unit, const Unit* pylon) {

		const ObservationInterface* observation = Observation();
		Units bases = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));

		if (unit == nullptr) return false;
		if (pylon == nullptr) return false;
		std::vector<PowerSource> power_sources = observation->GetPowerSources();
		if (power_sources.empty()) {
			return false;
		}
		float radius = power_sources.front().radius;
		float rx = GetRandomScalar();
		float ry = GetRandomScalar();
		Point2D location = Point2D(pylon->pos.x + rx * radius, pylon->pos.y + ry * radius);

		if (!bases.empty())
		{
			if (Distance2D(location, front_expansion) < bases.front()->radius*1.3) {
				return false;
			}
		}
		// Check to see if unit can make it there
		if (Query()->PathingDistance(unit, location) < 0.1f) {
			return false;
		}
		// Check to see if unit can build there
		if (Query()->Placement(ABILITY_ID::BUILD_FORGE, location)) {
			Actions()->UnitCommand(unit, ABILITY_ID::BUILD_FORGE, location);
			return true;
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

		for (const auto& unit : units) {
			if (unit->build_progress != 1) {
				return false;
			}
		}

		if (observation->GetFoodUsed() < observation->GetFoodCap() - 15) {
			return false;
		}

		// Try and build a pylon. Find a random Probe and give it the order.
		float rx = GetRandomScalar();
		float ry = GetRandomScalar();
		Point2D build_location = Point2D(staging_location_.x + rx * 15, staging_location_.y + ry * 15);
		return TryBuildStructure(ABILITY_ID::BUILD_PYLON, UNIT_TYPEID::PROTOSS_PROBE, build_location);
	}
	bool TryBuildPylon(Point2D location) {
		const ObservationInterface* observation = Observation();

		if (observation->GetMinerals() < 100) {
			return false;
		}

		//check to see if there is already on building
		Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PYLON));

		for (const auto& unit : units) {
			if (unit->build_progress != 1) {
				return false;
			}
		}


		// Try and build a pylon. Find a random Probe and give it the order.
		float rx = GetRandomScalar();
		float ry = GetRandomScalar();
		Point2D build_location = Point2D(location.x + rx * 3, location.y + ry * 3);
		return TryBuildStructure(ABILITY_ID::BUILD_PYLON, UNIT_TYPEID::PROTOSS_PROBE, build_location);
	}

	void MineIdleWorkers(const Unit* worker, AbilityID worker_gather_command, UnitTypeID vespene_building_type) {
		if (worker == probe_scout || worker == probe_forge) return;

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
					if (worker == probe_scout) continue;
					if (!worker->orders.empty()) {
						if (worker->orders.front().target_unit_tag == base->tag) {
							//This should allow them to be picked up by mineidleworkers()
							MineIdleWorkers(worker, worker_gather_command, vespene_building_type);
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
					if (worker == probe_scout) continue;
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

	void ManageUpgrades() {
		const ObservationInterface* observation = Observation();
		auto upgrades = observation->GetUpgrades();
		if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_FLEETBEACON)>0) {
			TryBuildUnit(ABILITY_ID::RESEARCH_PROTOSSAIRWEAPONS, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
			TryBuildUnit(ABILITY_ID::RESEARCH_INTERCEPTORGRAVITONCATAPULT, UNIT_TYPEID::PROTOSS_FLEETBEACON);
		}
	}

	void BuildPylonEarly(Units pylons, Point2D probe_scout_dest)
	{
		if (pylons.size()<1 && probe_scout != nullptr) {
			if (Distance2D(probe_scout->pos, front_expansion)<5) {
				probe_scout_dest = Point2D((double)game_info_.width / 2, (double)game_info_.height / 2);
				Actions()->UnitCommand(probe_scout, ABILITY_ID::MOVE, probe_scout_dest);
			}

			if (Distance2D(probe_scout->pos, front_expansion) > 10 && Distance2D(probe_scout->pos, startLocation_) > 25) {
				probe_scout_dest = probe_scout->pos;
				float rx = GetRandomScalar();
				float ry = GetRandomScalar();
				Point2D build_location = Point2D(probe_scout->pos.x + rx * 3, probe_scout->pos.y + ry * 3);
				if (Query()->PathingDistance(probe_scout, build_location) < 0.1f) {
					return;
				}
				if (Query()->Placement(ABILITY_ID::BUILD_PYLON, build_location)) {
					Actions()->UnitCommand(probe_scout, ABILITY_ID::BUILD_PYLON, build_location);
					pylonlocation = build_location; // Control
				}
			}
		}
	}
	Point2D baselocation;

	bool EarlyStrategy() {
		const ObservationInterface* observation = Observation();
		Units workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PROBE));
		Units bases = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_NEXUS));
		Units pylons = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PYLON));
		Units enemy_structures = observation->GetUnits(Unit::Alliance::Enemy, IsStructure(observation));
		Units enemy_townhalls = observation->GetUnits(Unit::Alliance::Enemy, IsTownHall());
		Units Cannons = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_PHOTONCANNON));

		int forge_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_FORGE);
		int cannon_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_PHOTONCANNON);
		int shield_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_SHIELDBATTERY);
		int gateway_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_GATEWAY) + CountUnitType(observation, UNIT_TYPEID::PROTOSS_WARPGATE);
		int assimilator_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_ASSIMILATOR);
		int cybernetics_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_CYBERNETICSCORE);
		int stargate_count = CountUnitType(observation, UNIT_TYPEID::PROTOSS_STARGATE);

		Point2D probe_scout_dest = Point2D((double)game_info_.width / 2, (double)game_info_.height / 2);

		bool done = false;

		const Unit* base = nullptr;
		const Unit* frontbase = nullptr;
		if (bases.size()>1) {
			for (const auto& b : bases) {
				if (Distance2D(b->pos, startLocation_)<3) {
					base = b;
				}
				else
				{
					frontbase = b;
				}
			}
		}
		else if (!bases.empty()) {
			base = bases.front();
			frontbase = bases.front();
		}
		baselocation = base->pos;

		//BuildPylonEarly(pylons, probe_scout_dest);

		if (pylons.size()<1 && probe_scout != nullptr) {
			if (Distance2D(probe_scout->pos, front_expansion)<5) {
				probe_scout_dest = Point2D((double)game_info_.width / 2, (double)game_info_.height / 2);
				Actions()->UnitCommand(probe_scout, ABILITY_ID::MOVE, probe_scout_dest);
			}

			if (Distance2D(probe_scout->pos, front_expansion) > 10 && Distance2D(probe_scout->pos, startLocation_) > 25) {
				probe_scout_dest = probe_scout->pos;
				float rx = GetRandomScalar();
				float ry = GetRandomScalar();
				Point2D build_location = Point2D(probe_scout->pos.x + rx * 3, probe_scout->pos.y + ry * 3);
				if (Query()->PathingDistance(probe_scout, build_location) < 0.1f) {
					return false;
				}
				if (Query()->Placement(ABILITY_ID::BUILD_PYLON, build_location)) {
					Actions()->UnitCommand(probe_scout, ABILITY_ID::BUILD_PYLON, build_location);
					pylonlocation = build_location; // Control
				}
			}
		}

		if (find_enemy_location == false && pylons.size()>0) {
			Actions()->UnitCommand(probe_scout, ABILITY_ID::MOVE, game_info_.enemy_start_locations.front());
			if (!enemy_townhalls.empty() || enemy_structures.size()>2 || game_info_.enemy_start_locations.size() == 1) {
				if (Distance2D(probe_scout->pos, game_info_.enemy_start_locations.front())<10 || game_info_.enemy_start_locations.size() == 1) {
					find_enemy_location = true;
					//std::cout << "find!" << std::endl;
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





		if (observation->GetFoodUsed()<14) {
			if (probe_scout == nullptr || !probe_forge->is_alive) {
				probe_scout_dest = front_expansion;
				GetRandomUnit(probe_scout, observation, UNIT_TYPEID::PROTOSS_PROBE);
			}
			if (probe_forge == nullptr || !probe_forge->is_alive) {
				GetRandomUnit(probe_forge, observation, UNIT_TYPEID::PROTOSS_PROBE);
				if (probe_scout == probe_forge) probe_forge = nullptr;
			}

			TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
		}
		else if (observation->GetFoodUsed() == 14) {
			if (Distance2D(startLocation_, probe_scout->pos)<5) Actions()->UnitCommand(probe_scout, ABILITY_ID::MOVE, probe_scout_dest);
			TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
		}
		else if (observation->GetFoodUsed() == 15) {
			if (pylon_first == nullptr && pylons.size()>0) pylon_first = pylons.front();
			if (pylon_first != nullptr && observation->GetMinerals()>100) {
				Actions()->UnitCommand(probe_forge, ABILITY_ID::MOVE, pylon_first->pos);
			}
			TryBuildUnitChrono(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
		}
		else if (observation->GetFoodUsed() == 16) {
			if (forge_count<1) {
				if (!TryBuildForge(probe_forge, pylon_first))
					TryBuildStructureNearPylon(ABILITY_ID::BUILD_FORGE, UNIT_TYPEID::PROTOSS_PROBE);
			}
			else TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);

		}
		else if (observation->GetFoodUsed()<19) {
			if (bases.size()<2) {
				if (observation->GetMinerals()>400) {
					TryBuildExpansionNexus();
					//Actions()->UnitCommand(probe_forge, ABILITY_ID::BUILD_NEXUS, front_expansion);
				}
				else if (observation->GetMinerals()>300) {
					Actions()->UnitCommand(probe_forge, ABILITY_ID::MOVE, front_expansion);
				}
			}
			else {
				if (pylon_first != nullptr && observation->GetMinerals()>150 && cannon_count<4 && !probe_forge->orders.size()) {
					TryBuildStructureNearPylonWithUnit(probe_forge, ABILITY_ID::BUILD_PHOTONCANNON, pylon_first);
				}
			}
			//if (observation->GetFoodUsed() != 18) TryBuildUnitChrono(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
			if (cannon_count>3) TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);

		}
		else if (observation->GetFoodUsed()<20) {
			if (observation->GetMinerals()>150) {
				if (TryBuildStructureNearPylonWithUnit(probe_forge, ABILITY_ID::BUILD_GATEWAY, pylon_first) && !done)
					bool done = true;
				else if (!done)
				{
					if (TryBuildPylon(staging_location_)) {
						bool done = true;
						TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_PROBE);
					}
				}
			}
			if (gateway_count>0) {
				TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
			}
		}
		else if (observation->GetFoodUsed()<21) {
			if (observation->GetMinerals()>75 && assimilator_count<2) {
				TryBuildGas(ABILITY_ID::BUILD_ASSIMILATOR, UNIT_TYPEID::PROTOSS_PROBE, base->pos);
			}
			else if (assimilator_count == 2) TryBuildUnit(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
		}
		else {
			if (pylons.size()<2) {
				TryBuildPylon(staging_location_);
			}
			else if (pylons.size() == 2) {
				if (observation->GetFoodUsed()>observation->GetFoodCap() - 6)
					TryBuildPylon(Point2D((FindNearestMineralPatch(base->pos)->pos.x + base->pos.x) / 2, (FindNearestMineralPatch(base->pos)->pos.y + base->pos.y) / 2));
			}
			else if (pylons.size() == 3) {
				TryBuildPylon(FindNearestMineralPatch(front_expansion)->pos);
			}
			else if (observation->GetFoodUsed()>observation->GetFoodCap() - 7) {
				if (!TryBuildPylon(frontbase->pos))
					if (!TryBuildPylon(base->pos))
						TryBuildPylon();
			}

			if (cybernetics_count<1 && gateway_count > 0) {
				TryBuildStructureNearPylon(ABILITY_ID::BUILD_CYBERNETICSCORE, UNIT_TYPEID::PROTOSS_PROBE);
			}
			else if (gateway_count == 0) // Control
				TryBuildStructureNearPylon(ABILITY_ID::BUILD_GATEWAY, UNIT_TYPEID::PROTOSS_PROBE);
			else if (cannon_count<6) {
				TryBuildStructureNearPylonWithUnit(probe_forge, ABILITY_ID::BUILD_PHOTONCANNON, pylon_first);
			}
			else if (cannon_count == 6) {
				bool ShouldRecoverCannon = true;
				if (stargate_count<1 && cybernetics_count == 1) {
					TryBuildStructureNearPylon(ABILITY_ID::BUILD_STARGATE, UNIT_TYPEID::PROTOSS_PROBE);
				}
				else if (cybernetics_count == 0) { // Control
					TryBuildStructureNearPylon(ABILITY_ID::BUILD_CYBERNETICSCORE, UNIT_TYPEID::PROTOSS_PROBE);
				}
				else {
					for (const auto& b : bases) {
						if (b != base) {
							if (b->assigned_harvesters >= 10 && assimilator_count<4) {
								TryBuildGas(ABILITY_ID::BUILD_ASSIMILATOR, UNIT_TYPEID::PROTOSS_PROBE, b->pos);
							}
						}
					}
				}
			}
			if (observation->GetFoodUsed()<60 && CountUnitType(observation, UNIT_TYPEID::PROTOSS_ORACLE) < 2 && OracleCount < 2) {
				//Control
				if (TryBuildUnitChrono(ABILITY_ID::TRAIN_ORACLE, UNIT_TYPEID::PROTOSS_STARGATE))
				{
					OracleCount++;
					OracleTrained = true;
					Chat("Oracle is Trained!");
				}
			}
			if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_FLEETBEACON)<1 && stargate_count > 0) {
				TryBuildStructureNearPylon(ABILITY_ID::BUILD_FLEETBEACON, UNIT_TYPEID::PROTOSS_PROBE);
			}
			else if (stargate_count == 0)
				TryBuildStructureNearPylon(ABILITY_ID::BUILD_STARGATE, UNIT_TYPEID::PROTOSS_PROBE);
			else {
				if (cannon_count<8) {
					TryBuildStructureNearPylonWithUnit(probe_forge, ABILITY_ID::BUILD_PHOTONCANNON, pylon_first);
				}
				else if (stargate_count< 3) {
					TryBuildStructureNearPylon(ABILITY_ID::BUILD_STARGATE, UNIT_TYPEID::PROTOSS_PROBE);
				}
				else {
					if ((observation->GetMinerals() - 300) * 250>observation->GetVespene() * 350) {
						if (shield_count<2) {
							TryBuildStructureNearPylonWithUnit(probe_forge, ABILITY_ID::BUILD_SHIELDBATTERY, pylon_first);
						}
					}
					if (cannon_count>4 && cannon_count<10) {
						for (const auto& pylon : pylons) {
							if (Distance2D(pylon->pos, base->pos)<5) {
								TryBuildStructureNearPylonWithUnit(probe_forge, ABILITY_ID::BUILD_PHOTONCANNON, pylon);
								return false;
							}
						}
					}
					if (cannon_count >= 10) {
						if (observation->GetMinerals() > 1500) // Control
							TryBuildStructureNearPylon(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE);
						else if ((observation->GetMinerals() - 500) * 250 > observation->GetVespene() * 350) {
							TryBuildStructureNearPylon(ABILITY_ID::BUILD_PHOTONCANNON, UNIT_TYPEID::PROTOSS_PROBE);
						}
					}
					if (observation->GetMinerals() > 350 && observation->GetVespene() > 250) {
						if (TryBuildUnitChrono(ABILITY_ID::TRAIN_CARRIER, UNIT_TYPEID::PROTOSS_STARGATE)) {
							CarrierCount++;
							return true;
						}
					}
				}
			}
			if (CountUnitType(observation, UNIT_TYPEID::PROTOSS_PROBE)<42) {
				TryBuildUnitChrono(ABILITY_ID::TRAIN_PROBE, UNIT_TYPEID::PROTOSS_NEXUS);
			}
		}
		return false;
	}

	void scoutprobe() {
		const ObservationInterface* observation = Observation();
		if (observation->GetFoodUsed() < 15) return;

		Point2D tag_pos = FindNearestMineralPatch(*iter_exp)->pos;
		if (Distance2D(game_info_.enemy_start_locations.front(), tag_pos)<7 || Distance2D(enemy_expansion, tag_pos)<7) {
			iter_exp++;
			return;
		}
		Actions()->UnitCommand(probe_scout, ABILITY_ID::MOVE, tag_pos);
		if (Distance2D(probe_scout->pos, tag_pos)<1) {
			iter_exp++;
		}

	}

	bool early_strategy = false;
	const Unit* probe_scout = nullptr;
	const Unit* pylon_first = nullptr;
	const Unit* probe_forge = nullptr;

	bool find_enemy_location = false;
	std::vector<Point2D>::iterator iter_esl = game_info_.enemy_start_locations.begin();
	std::vector<Point3D>::iterator iter_exp;
	Point3D enemy_expansion;
};

class Human : public sc2::Agent {
public:
	void OnGameStart() final {
		Debug()->DebugTextOut("Human");
		Debug()->SendDebug();
	}
};