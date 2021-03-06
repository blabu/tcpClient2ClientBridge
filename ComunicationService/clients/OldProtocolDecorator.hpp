#ifndef PROTOCOL_DECORATOR_H
#define PROTOCOL_DECORATOR_H
#include "TcpClient.hpp"
#include "boost/bind.hpp"

// Декоратор реализующий обрамление полезных данных дополнительной информацией
class OldProtocolDecorator : public IBaseClient {
	static const std::string connectHeader;
	static const char startPackage;
	static const char stopPackage;
	std::shared_ptr<TcpClient> clientDelegate;
	void emmitNewDataSlot(const message_ptr m);
public:
	OldProtocolDecorator(boost::asio::io_service *const srv, const std::string& host, const std::string& port, const std::string& deviceID);
	void sendNewData(const message_ptr& msg);
	void close() noexcept;
	void open();
};

#endif //ROTOCOL_DECORATOR_H
