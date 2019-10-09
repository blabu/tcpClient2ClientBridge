#ifndef BASE_CLIENT_HPP
#define BASE_CLIENT_HPP

#include "messagesDTO.hpp"
#include <functional>
#include <deque>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include "IBaseClient.hpp"

/*
Абстрактный базовый класс для клиентов TCP и Serial.
Содержит общий для конкретных клиентов код
Каждый из наследников реализует свой метод write, котрый должен писать полученные в процессе работы программы данные в свой внешний интерфейс 
(сокет или открытый последовательный порт)
*/
class BaseClient : public IBaseClient {
protected:
    boost::asio::io_service *const service;
    boost::asio::deadline_timer readTimer;
    std::chrono::milliseconds readTimeout;
	message readBuffer;
	std::deque<message_ptr> MessagesQueue; // Очередь сообщений (работаем через нее)

	void updateTimer(boost::asio::deadline_timer&t, std::function<void(boost::system::error_code er)> handler, std::chrono::microseconds time);
	BaseClient() = delete;
	BaseClient(const BaseClient&) = delete;
	virtual void write() = 0;
public:
	BaseClient(boost::asio::io_service*const srv, std::size_t readBufferSize, std::chrono::milliseconds ReadTmeout);
	virtual ~BaseClient() {}
	void sendNewData(const message_ptr& msg) override; // Частичная реализация интерфейса IBaseClient
};


#endif //BASE_CLIENT_HPP
