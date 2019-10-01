#include "ComunicationService.hpp"
#include "MainProjectLoger.hpp"
#include "clients/TcpClient.hpp"
#include <fstream>
#include <thread>
#include <json.hpp>
/*
Читает конфигурационный файл и инициализирует всех клиентов программы
*/
CommunicationService::CommunicationService(boost::asio::io_service * const s, const std::string & atCommandFile) : srv(s), conf(Configuration::getConfiguration(atCommandFile)) {
	std::string portName;
	bool isBinaryProtocolDecorator = false;
	try {
		Loger::setShowLevel(conf->getConfigInt("log:showLogLevel"));
		Loger::setSaveLevel(conf->getConfigInt("log:saveLogLevel"));
		Loger::SetAddr(conf->getConfigString("log:path"));
		ModemClient::setHost(conf->getConfigString("server:host"));
		ModemClient::setPort(conf->getConfigString("server:port"));
		SerialClient::setReadTimeout(conf->getConfigInt("serial:timeout"));
		TcpClient::setReadTimeout(conf->getConfigInt("server:timeout"));
		if (conf->getConfigString("protocol:base64").find("disabled") != std::string::npos || 
			conf->getConfigString("protocol:base64").find("disable") != std::string::npos ||
			conf->getConfigString("protocol:base64") == "0" ) {
			isBinaryProtocolDecorator = true;
		}
		portName = conf->getConfigString("serial:portName");
	}
	catch (const nlohmann::detail::out_of_range&) {
		globalLog.addLog(Loger::L_ERROR, "Out of range in json");
		globalLog.snapShotLong();
		std::exit(1);
	}
	auto commands = conf->at("commands");
	for (auto& el : commands.items()) {
		globalLog.addLog(Loger::L_DEBUG, "Key " + el.key(), " Value " + el.value().get<std::string>());
		ModemClient::appendNewCommand(el.key(), el.value().get<std::string>());
	}
	
	modem = std::shared_ptr<ModemClient>(new ModemClient(srv, isBinaryProtocolDecorator));

	serial = std::shared_ptr<SerialClient>(new SerialClient(srv, portName));
	try {
		serial->setProperrties(conf->getConfigString("serial:speed"),
			conf->getConfigString("serial:bitSize"),
			conf->getConfigString("serial:flowControl"),
			conf->getConfigString("serial:stopBits"));
	}
	catch (const nlohmann::detail::out_of_range& er) {
		globalLog.addLog(Loger::L_ERROR, "Exception when try find some serial properties, ", er.what());
	}
	serial->open();
	modem->open();
	globalLog.addLog(Loger::L_TRACE, "Setup default behavior");

	modem->receiveNewData.connect(boost::bind(&SerialClient::sendNewData, serial.get(), _1));
	serial->receiveNewData.connect(boost::bind(&ModemClient::sendNewData, modem.get(), _1));
}
