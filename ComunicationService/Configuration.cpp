#include "Configuration.hpp"
#include "MainProjectLoger.hpp"
#include <fstream>
#include <thread>
#include <boost/algorithm/string.hpp>

Configuration::Configuration(const std::string& configFileName) {
	globalLog.addLog(Loger::L_TRACE, "Try open AT command map file in json format: ", configFileName);
	std::ifstream file(configFileName, std::ios::in);
	if (file.is_open()) { globalLog.addLog(Loger::L_TRACE, "File ", configFileName, " opened"); }
	globalLog.addLog(Loger::L_TRACE, "Try parse json file");
	try {
		file >> configuration;
		file.close();
	}
	catch (...) {
		globalLog.addLog(Loger::L_ERROR, "Unhandled ecxeption when try parse file ", configFileName);
		globalLog.snapShotLong();
		std::this_thread::sleep_for(std::chrono::seconds(2));
		exit(1);
	}
}

const Configuration*const Configuration::getConfiguration(const std::string & configFileName){
	static Configuration conf(configFileName);
	return &conf;
}

Configuration::~Configuration() {}

const std::string Configuration::getConfigString(const std::string & key1) const{
	std::list<std::string> keys;
	boost::algorithm::split(keys, key1, [](char c) {return c == ':'; });
	auto ref = configuration.at(keys.front());
	globalLog.addLog(Loger::L_TRACE, "First key is ", keys.front());
	keys.pop_front();
	for (auto it : keys) {
		globalLog.addLog(Loger::L_TRACE, "Next key is ", it);
		ref = ref.at(it);
	}
	return ref.get<std::string>();
}

const unsigned int Configuration::getConfigInt(const std::string & key1) const {
	std::list<std::string> keys;
	boost::algorithm::split(keys, key1, [](char c) {return c == ':'; });
	auto ref = configuration.at(keys.front());
	globalLog.addLog(Loger::L_TRACE, "First key is ", keys.front());
	keys.pop_front();
	for (auto it : keys) {
		globalLog.addLog(Loger::L_TRACE, "Next key is ", it);
		ref = ref.at(it);
	}
	return ref.get<unsigned int>();
}
