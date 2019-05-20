#ifndef COMMUNICATION_SERVICE_H
#define COMMUNICATION_SERVICE_H

#include "json.hpp"
#include "ModemClient.hpp"
#include "SerialClient.hpp"
#include "MainProjectLoger.hpp"

#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>

/*
Сервис, занимающийся координацией все узлов программы
Дирижер всей логики
*/
class CommunictionService {
	boost::asio::io_service* const srv;
	std::shared_ptr<ModemClient> modem;
	std::shared_ptr<SerialClient> serial;
	nlohmann::json configuration;
	void readConfigurationFile(const std::string& configFile);
public:
	CommunictionService(boost::asio::io_service* s, const std::string& atCommandFile);
	void run() {srv->run();}
	~CommunictionService() { modem->close(); serial->close(); }
};

#endif //COMMUNICATION_SERVICE_H