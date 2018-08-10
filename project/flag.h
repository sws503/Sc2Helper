#pragma once
#include <unordered_map>
class Flags {
	std::unordered_map<std::string, int> flag_map;

public:
	void reset() {
		flag_map.clear();
	}

	int status(std::string name) {
		return flag_map.count(name)? flag_map.at(name) : 0;
	}

	void raise(std::string name) {
		set(name, 1);
	}

	void set(std::string name, int status=1) {
		clear(name);
		if (status) flag_map.emplace(name, status);
	}

	void clear(std::string name) {
		flag_map.erase(name);
	}

	void print() {
#ifdef DEBUG
		
#endif
	}
};
