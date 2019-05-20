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
����������� ������� ����� ��� �������� TCP � Serial.
�������� ����� ��� ���������� �������� ���
������ �� ����������� ��������� ���� ����� write, ������ ������ ������ ���������� � �������� ������ ��������� ������ � ���� ������� ��������� 
(����� ��� �������� ���������������� ����)
*/
class BaseClient : public IBaseClient {
protected:
	message readBuffer;
	std::deque<message_ptr> MessagesQueue; // ������� ��������� (�������� ����� ���)
	std::chrono::milliseconds readTimeout;
	boost::asio::deadline_timer readTimer;
	boost::asio::io_service *const service;

	void updateTimer(boost::asio::deadline_timer&t, std::function<void(boost::system::error_code er)> handler, std::chrono::microseconds time);
	BaseClient() = delete;
	BaseClient(const BaseClient&) = delete;
	virtual void write() = 0;
public:
	BaseClient(boost::asio::io_service*const srv, std::size_t readBufferSize, std::chrono::milliseconds ReadTmeout);
	virtual ~BaseClient() {}
	void sendNewData(const message_ptr& msg); // ��������� ���������� ���������� IBaseClient
};


#endif //BASE_CLIENT_HPP
