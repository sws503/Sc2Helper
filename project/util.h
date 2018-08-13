#pragma once

using namespace sc2;

// Control 시작
// 7.3 안좋은 효과들 목록
enum class EFFECT_ID
{
	INVALID = 0,
	PSISTORM = 1,
	GUARDIANSHIELD = 2,
	TEMPORALFIELDGROWING = 3,
	TEMPORALFIELD = 4,
	THERMALLANCES = 5, // 거신
	SCANNERSWEEP = 6,
	NUKEDOT = 7,
	LIBERATORMORPHING = 8,
	LIBERATORMORPHED = 9,
	BLINDINGCLOUD = 10,
	CORROSIVEBILE = 11,
	LURKERATTACK = 12
};
typedef SC2Type<EFFECT_ID>  EffectID;
// Control 끝

struct Rusher {
	Rusher(Point2D startLocation_) : sl(startLocation_) {
	}
	bool operator()(const Unit& unit) {
		return (unit.unit_type.ToType() == UNIT_TYPEID::PROTOSS_PHOTONCANNON ||
			unit.unit_type.ToType() == UNIT_TYPEID::PROTOSS_PYLON ||
			unit.unit_type.ToType() == UNIT_TYPEID::ZERG_HATCHERY ||
			unit.unit_type.ToType() == UNIT_TYPEID::TERRAN_BUNKER ||
			unit.unit_type.ToType() == UNIT_TYPEID::TERRAN_BARRACKS)
			&& Distance2D(sl, unit.pos) < 20;
	}
private:
	Point2D sl;
};

struct IsTurretType {
	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_PHOTONCANNON: return true;
		case UNIT_TYPEID::ZERG_SPORECRAWLER: return true;
		case UNIT_TYPEID::ZERG_SPINECRAWLER: return true;
		case UNIT_TYPEID::TERRAN_BUNKER: return true;
		case UNIT_TYPEID::TERRAN_MISSILETURRET: return true;

		default: return false;
		}
	}
};

struct AirAttacker { // 공중 공격 가능한 적들 (폭풍함이 우선 공격하는 적) //시간이 남으면 weapon.type == sc2::Weapon::TargetType::Air 으로 할수있지만 시간이 없음
	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {

		case UNIT_TYPEID::PROTOSS_STALKER:
		case UNIT_TYPEID::PROTOSS_PHOTONCANNON: 

		case UNIT_TYPEID::TERRAN_MARINE: 
		case UNIT_TYPEID::TERRAN_MISSILETURRET: 
		case UNIT_TYPEID::TERRAN_BUNKER:

		case UNIT_TYPEID::ZERG_SPORECRAWLER:
		case UNIT_TYPEID::ZERG_QUEEN: 

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
		case UNIT_TYPEID::PROTOSS_ARCHON: return unit.build_progress == 1.0f;

		default: return false;
		}
	}
};

struct AirAttacker2 { // 공중 공격 가능한 적들: 예언자가 기피해야하는 적. 완성이 아직 안 된 건물들은 안 피하도록
	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_PHOTONCANNON:
		case UNIT_TYPEID::TERRAN_MISSILETURRET:
		case UNIT_TYPEID::TERRAN_BUNKER:
		case UNIT_TYPEID::ZERG_SPORECRAWLER:
			return (unit.build_progress >= 0.95);

		case UNIT_TYPEID::PROTOSS_STALKER:
		case UNIT_TYPEID::TERRAN_MARINE:
		case UNIT_TYPEID::ZERG_QUEEN:
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

struct IsRanged {
	IsRanged(const ObservationInterface* obs) : observation_(obs) {}

	bool operator()(const Unit& unit) {
		auto attributes = observation_->GetUnitTypeData().at(unit.unit_type).attributes;
		for (const auto& attribute : attributes) {
			if (attribute == Attribute::Structure) {
				return false;
			}
		}

		if (unit.unit_type == UNIT_TYPEID::PROTOSS_ORACLE || unit.unit_type == UNIT_TYPEID::PROTOSS_CARRIER)
			return true;

		auto Weapon = observation_->GetUnitTypeData().at(unit.unit_type).weapons;
		for (const auto& weapon : Weapon) {
			if (weapon.range > 2.0f) {
				return true;
			}
		}
		/*
		switch (unit.unit_type.ToType()) {
		default: return false;
		}*/
		return false;
	}
private:
	const ObservationInterface* observation_;
};

struct IsWorker {
	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_PROBE: return true;
		case UNIT_TYPEID::ZERG_DRONE: return true;
		case UNIT_TYPEID::ZERG_DRONEBURROWED: return true;
		case UNIT_TYPEID::TERRAN_SCV: return true;
		default: return false;
		}
	}
};

struct IsArmy {
	IsArmy(const ObservationInterface* obs) : observation_(obs) {}

	bool operator()(const Unit& unit) {
		auto attributes = observation_->GetUnitTypeData().at(unit.unit_type).attributes;
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
		case UNIT_TYPEID::PROTOSS_WARPPRISM: return false;
		case UNIT_TYPEID::PROTOSS_WARPPRISMPHASING: return false;
		case UNIT_TYPEID::PROTOSS_PHOTONCANNON: return true;
		case UNIT_TYPEID::ZERG_SPORECRAWLER: return true;
		case UNIT_TYPEID::ZERG_SPINECRAWLER: return true;
		case UNIT_TYPEID::TERRAN_BUNKER: return true;
		case UNIT_TYPEID::TERRAN_MISSILETURRET: return true;
		default:
		{
			for (const auto& attribute : attributes) {
				if (attribute == Attribute::Structure) {
					return false;
				}
			}
			return true;
		}
		}
	}
private:
	const ObservationInterface* observation_;
};

struct IsTownHall {
	bool operator()(const Unit& unit) {

		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::ZERG_HATCHERY: return true;
		case UNIT_TYPEID::ZERG_LAIR: return true;
		case UNIT_TYPEID::ZERG_HIVE: return true;
		case UNIT_TYPEID::TERRAN_COMMANDCENTER: return true;
		case UNIT_TYPEID::TERRAN_COMMANDCENTERFLYING: return true;
		case UNIT_TYPEID::TERRAN_ORBITALCOMMAND: return true;
		case UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING: return true;
		case UNIT_TYPEID::TERRAN_PLANETARYFORTRESS: return true;
		case UNIT_TYPEID::PROTOSS_NEXUS: return true;
		default: return false;
		}
	}
};

struct IsNotStructure {
	IsNotStructure(const ObservationInterface* obs) : observation_(obs) {};

	bool operator()(const Unit& unit) {
		auto& attributes = observation_->GetUnitTypeData().at(unit.unit_type).attributes;
		bool is_structure = true;
		for (const auto& attribute : attributes) {
			if (attribute == Attribute::Structure) {
				is_structure = false;
			}
		}
		return is_structure;
	}
private:
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
private:
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

struct IsMineral {
	bool operator()(const Unit& unit) {
		return unit.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD750 ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD750 ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERMINERALFIELD750 ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERRICHMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_PURIFIERRICHMINERALFIELD750 ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_LABMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_LABMINERALFIELD750 ||
			unit.unit_type == UNIT_TYPEID::NEUTRAL_BATTLESTATIONMINERALFIELD || unit.unit_type == UNIT_TYPEID::NEUTRAL_BATTLESTATIONMINERALFIELD750;
	}
};

struct HasBuff {
	HasBuff(BuffID buff) : buff_(buff) {};

	bool operator()(const Unit& unit) {
		// is buffed?
		for (const auto& buff : unit.buffs) {
			if (buff == buff_) return true;
		}
		return false;
	}

private:
	const BuffID buff_;
};

struct IsUnpowered {

	bool operator()(const Unit& unit) {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::PROTOSS_GATEWAY:
		case UNIT_TYPEID::PROTOSS_PHOTONCANNON:
		case UNIT_TYPEID::PROTOSS_SHIELDBATTERY:
		case UNIT_TYPEID::PROTOSS_FORGE:
		case UNIT_TYPEID::PROTOSS_CYBERNETICSCORE:
		case UNIT_TYPEID::PROTOSS_STARGATE:
		case UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY:
		case UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL:
		case UNIT_TYPEID::PROTOSS_FLEETBEACON:
		case UNIT_TYPEID::PROTOSS_DARKSHRINE:
		case UNIT_TYPEID::PROTOSS_ROBOTICSBAY:
		case UNIT_TYPEID::PROTOSS_TEMPLARARCHIVE:
		case UNIT_TYPEID::PROTOSS_WARPGATE:
			return !(unit.is_powered);
		default:
			return false;
		}
	}
};

struct IsNearbyArmies {
	IsNearbyArmies(const ObservationInterface* obs, Point2D MyPosition, int Radius) :
		observation_(obs), mp(MyPosition), radius(Radius) {}

	bool operator()(const Unit& unit) {
		auto attributes = observation_->GetUnitTypeData().at(unit.unit_type).attributes;
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::ZERG_OVERLORD: return false;
		case UNIT_TYPEID::ZERG_LARVA: return false;
		case UNIT_TYPEID::ZERG_EGG: return false;
		case UNIT_TYPEID::TERRAN_MULE: return false;
		case UNIT_TYPEID::TERRAN_NUKE: return false;
		case UNIT_TYPEID::ZERG_DRONE:
		case UNIT_TYPEID::TERRAN_SCV:
		case UNIT_TYPEID::PROTOSS_PROBE:return false;

		case UNIT_TYPEID::PROTOSS_PHOTONCANNON:
		case UNIT_TYPEID::TERRAN_BUNKER:
		case UNIT_TYPEID::ZERG_SPINECRAWLER:
		case UNIT_TYPEID::TERRAN_PLANETARYFORTRESS: return Distance2D(mp, unit.pos) < radius;


		default:
			for (const auto& attribute : attributes) {
				if (attribute == Attribute::Structure) {
					return false;
				}
			}
			return Distance2D(mp, unit.pos) < radius;
		}
	}
private:
	const ObservationInterface* observation_;
	Point2D mp;
	int radius;
};
