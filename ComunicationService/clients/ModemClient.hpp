#ifndef MODEM_CLIENT_H
#define MODEM_CLIENT_H

#include <IBaseClient.hpp>
#include <string>
#include <map>
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include "ProtocolDecorator.hpp"

/*
ModemClient - реализует базовый интерфейс клиента. 
Является декоратором TcpClient
Иммитирует стандартные ответы на запросы клиента с последовательного порта (или другого клиента)
Ответы хранятся в специальном словаре, котрый формируется при создании объекта этого класса из json объекта,
передаваемого в конструктор класса
*/
class ModemClient : public IBaseClient {
	static std::map<std::string, std::string> vocabulary;
	static std::string host;
	static std::string port;

	boost::asio::io_service* const srv;
	std::atomic<bool> isStarted;
	std::atomic<bool> isFirstMessage;

	std::shared_ptr<IBaseClient> clientDelegate;

	ModemClient() = delete;
	ModemClient(const ModemClient&) = delete;

	void emmitNewDataFromDelegate(const message_ptr m) {
		receiveNewData(m);
	}

	bool stopCommandHandler(const std::string& command);

	bool startCommandHandler(const std::string& command);

	std::string write(const std::string& command, const std::string& defaultAnswer = "OK\r\n");

public:
	/*
	В качестве параметра используется адрес файла со словарем
	словарь представляет из себя масив json объектов с минимуму двумя параметрами command и answer
	*/
	ModemClient(boost::asio::io_service* const service) : srv(service) {}

	void open() {}
	void close() noexcept {}

	void sendNewData(const message_ptr& msg);

	static void setHost(const std::string& h) { host = h; }
	static void setPort(const std::string& p) { port = p; }
	static void appendNewCommand(const std::string& command, const std::string& answer) {
		vocabulary.insert(std::pair<std::string, std::string>(command, answer));
	}
};
#endif //MODEM_CLIENT_H
