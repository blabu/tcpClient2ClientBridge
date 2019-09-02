#ifndef COMMUNICATION_SERVICE_H
#define COMMUNICATION_SERVICE_H

#include "Configuration.hpp"
#include "ModemClient.hpp"
#include "SerialClient.hpp"
#include "MainProjectLoger.hpp"

#include <boost/asio/io_service.hpp>

/*
Сервис, занимающийся координацией все узлов программы
Дирижер всей логики
*/
class CommunicationService {
	boost::asio::io_service* const srv;
	std::shared_ptr<ModemClient> modem;
	std::shared_ptr<SerialClient> serial;
	const Configuration* const conf;
public:
    CommunicationService(boost::asio::io_service *const s, const std::string& atCommandFile);
	void run() {srv->run();}
    ~CommunicationService() { modem->close(); serial->close(); }
};

#endif //COMMUNICATION_SERVICE_H
