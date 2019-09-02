#pragma once

#include "json.hpp"

class Configuration
{
	nlohmann::json configuration;
	Configuration(const std::string& configFileName);
	Configuration() = delete;
public:
	static const Configuration*const getConfiguration(const std::string& configFileName);
	~Configuration();
	const std::string getConfigString(const std::string& key1) const; // key1 can be key:subKey or key:subKey:subKey...
	const unsigned int getConfigInt(const std::string& key1) const;// key1 can be key:subKey or key:subKey:subKey...
	auto at(const std::string& key1) const { return configuration.at(key1); }
};

