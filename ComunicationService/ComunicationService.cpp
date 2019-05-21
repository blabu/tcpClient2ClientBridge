#include "ComunicationService.hpp"
#include "MainProjectLoger.hpp"
#include <fstream>

void CommunictionService::readConfigurationFile(const std::string & configFile) {
	globalLog.addLog(Loger::L_TRACE, "Try open AT command map file in json format: ", configFile);
	std::ifstream file(configFile);
	if (file.is_open()) { globalLog.addLog(Loger::L_TRACE, "File ", configFile, " opened"); }
	globalLog.addLog(Loger::L_TRACE, "Try parse json file");
	try {
		file >> configuration;
		file.close();
	}
	catch (...) {
		globalLog.addLog(Loger::L_ERROR, "Unhandled ecxeption when try parse file ", configFile);
		globalLog.snapShotLong();
		std::this_thread::sleep_for(std::chrono::seconds(2));
		exit(1);
	}
}

/*
Читает конфигурационный файл и инициализирует всех клиентов программы
*/
CommunictionService::CommunictionService(boost::asio::io_service * const s, const std::string & atCommandFile) : srv(s) {
	readConfigurationFile(atCommandFile);
	Loger::setShowLevel(configuration.at("log").at("showLogLevel").get<unsigned int>());
	Loger::setSaveLevel(configuration.at("log").at("saveLogLevel").get<unsigned int>());
	ModemClient::setHost(configuration.at("server").at("host").get<std::string>());
	ModemClient::setPort(configuration.at("server").at("port").get<std::string>());
	ModemClient::setClientKey(configuration.at("server").at("clientKey").get<std::string>());
	auto commands = configuration.at("commands");
	for (auto& el : commands.items()) {
		globalLog.addLog(Loger::L_DEBUG, "Key " + el.key(), " Value " + el.value().get<std::string>());
		ModemClient::appendNewCommand(el.key(), el.value().get<std::string>());
	}
	SerialClient::setReadTimeout(configuration.at("serial").at("timeout").get<unsigned int>());
	TcpClient::setReadTimeout(configuration.at("server").at("timeout").get<unsigned int>());
	std::string portName(configuration.at("serial").at("portName").get<std::string>());

	modem = std::shared_ptr<ModemClient>(new ModemClient(srv));

	serial = std::shared_ptr<SerialClient>(new SerialClient(srv, portName));
	serial->open();
	modem->open();
	globalLog.addLog(Loger::L_TRACE, "Setup default behavior");

	modem->receiveNewData.connect(boost::bind(&SerialClient::sendNewData, serial.get(), _1));
	serial->receiveNewData.connect(boost::bind(&ModemClient::sendNewData, modem.get(), _1));
}
