#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "BaseClient.hpp"

/*
Простая DTO для объединения всех необходимых для работы TcpClient данных
*/
struct ConnectionProperties {
	std::string Host;
	std::string Port;
	std::string connectionString; // Строка, которая будет отправлена серверу сразу после успешного конекта
};

//TcpClient - Реализует интерфейс базового клиента для TCP соединения
class TcpClient : public BaseClient {
	static unsigned int receiveDataTimeout;
	const ConnectionProperties connection; // Настройки соодинения (нужны для переподключения)
	std::chrono::seconds connectionTimeout;// Таймоут соединения
	std::chrono::seconds reConnectionTimeout;
	boost::asio::ip::tcp::socket sock;
	boost::asio::deadline_timer watchdog;
	boost::asio::deadline_timer timer;
	boost::asio::ip::tcp::resolver resolver;
	 
	void timeoutHandler(const boost::system::error_code& er); 	/*Обработчик таймоута (сессия закончена)*/
    void connectionHandler(const boost::system::error_code& er, boost::asio::ip::tcp::resolver::iterator p);
	void connect();
    void readHandler(const boost::system::error_code& er, std::size_t sz);
	void read();
	void write();

	TcpClient() = delete;
	TcpClient(TcpClient&) = delete;
public:
	TcpClient(boost::asio::io_service *const srv, const ConnectionProperties& c);
	void open() {
		connect();
	}
	void close() noexcept {
		if (sock.is_open()) { boost::system::error_code er; sock.cancel(er); sock.close(er); }
	}
	virtual ~TcpClient();
	static void setReadTimeout(unsigned int timeout) {
		receiveDataTimeout = timeout;
	}
};

#endif // TCP_CLIENT_H
