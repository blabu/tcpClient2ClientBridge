#include "ComunicationService.hpp"
#include "MainProjectLoger.hpp"
#include <fstream>
#include <thread>
#include <json.hpp>
/*
Читает конфигурационный файл и инициализирует всех клиентов программы
*/
CommunicationService::CommunicationService(boost::asio::io_service * const s, const std::string & atCommandFile) : srv(s), conf(Configuration::getConfiguration(atCommandFile)) {
	std::string portName;
	try {
		Loger::setShowLevel(conf->getConfigInt("log:showLogLevel"));
		Loger::setSaveLevel(conf->getConfigInt("log:saveLogLevel"));
		ModemClient::setHost(conf->getConfigString("server:host"));
		ModemClient::setPort(conf->getConfigString("server:port"));
		SerialClient::setReadTimeout(conf->getConfigInt("serial:timeout"));
		TcpClient::setReadTimeout(conf->getConfigInt("server:timeout"));
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
	
	modem = std::shared_ptr<ModemClient>(new ModemClient(srv));

	serial = std::shared_ptr<SerialClient>(new SerialClient(srv, portName));
	serial->open();
	modem->open();
	globalLog.addLog(Loger::L_TRACE, "Setup default behavior");

	modem->receiveNewData.connect(boost::bind(&SerialClient::sendNewData, serial.get(), _1));
	serial->receiveNewData.connect(boost::bind(&ModemClient::sendNewData, modem.get(), _1));
}
