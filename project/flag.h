#pragma once
#include <unordered_set>
class Flags {
	std::unordered_set<std::string> flag_set;

public:
	void reset() {
		flag_set.clear();
	}

	bool status(std::string name) {
		return flag_set.count(name);
	}

	void raise(std::string name) {
		flag_set.insert(name);
	}

	void set(std::string name, bool status=true) {
		if (status) raise(name);
		else clear(name);
	}

	void clear(std::string name) {
		flag_set.erase(name);
	}
};