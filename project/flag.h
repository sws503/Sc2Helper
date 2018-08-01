#pragma once
#include <unordered_map>
class Flags {
	std::unordered_map<std::string, int> flag_map;

public:
	void reset() {
		flag_map.clear();
	}

	bool status(std::string name) {
		return flag_map.count(name);
	}

	void raise(std::string name) {
		flag_map.emplace(name, 1);
		//flag_map.insert(name, 1);
	}

	void set(std::string name, bool status=true) {
		if (status) raise(name);
		else clear(name);
	}

	void clear(std::string name) {
		flag_map.erase(name);
	}

	void print() {
#ifdef DEBUG
		
#endif
	}
};