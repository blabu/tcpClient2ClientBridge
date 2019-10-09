#ifndef MODEM_CLIENT_H
#define MODEM_CLIENT_H

#include <IBaseClient.hpp>
#include <string>
#include <map>
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>

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
	static std::string connectOk;
	static std::string connectFail;


	boost::asio::io_service* const srv;
	const bool isBinary;
	std::atomic<bool> isStarted;
	std::atomic<bool> isFirstMessage;
	std::shared_ptr<IBaseClient> clientDelegate;
	ModemClient() = delete;
	ModemClient(const ModemClient&) = delete;
	void emmitNewDataFromDelegate(const message_ptr m) {
		receiveNewData(m);
	}
	void stopCommandHandler();
	std::string startCommandHandler(const std::string& command);
	std::string write(const std::string& command, const std::string& defaultAnswer = "OK\r\n");

public:
	/*
	В качестве параметра используется адрес файла со словарем
	словарь представляет из себя масив json объектов с минимуму двумя параметрами command и answer
	*/
	ModemClient(boost::asio::io_service* const service, bool isBinary) : srv(service), isBinary(isBinary) {}

	void open() override {}
	void close() noexcept override {}
	void sendNewData(const message_ptr& msg) override;

	static void setHost(const std::string& h) { host = h; }
	static void setPort(const std::string& p) { port = p; }
	static void appendNewCommand(const std::string& command, const std::string& answer) {
		vocabulary.insert(std::pair<std::string, std::string>(command, answer));
	}
	static void setConnectOKAnswer(const std::string& answer) { connectOk = answer; }
	static void setConnectFailAnswer(const std::string& answer) { connectFail = answer; }
};
#endif //MODEM_CLIENT_H
