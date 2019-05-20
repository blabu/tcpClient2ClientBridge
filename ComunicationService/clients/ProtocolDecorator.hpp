#ifndef PROTOCOL_DECORATOR_H
#define PROTOCOL_DECORATOR_H
#include "TcpClient.hpp"
#include "boost/bind.hpp"

class ProtocolDecorator : public IBaseClient {
	static const std::string connectHeader;
	std::shared_ptr<TcpClient> clientDelegate;
	void emmitNewDataSlot(const message_ptr& m) { receiveNewData(m); }
public:
	ProtocolDecorator(boost::asio::io_service *const srv, const std::string& host, const std::string& port, const std::string& key, const std::string& deviceID);
	void sendNewData(const message_ptr& msg);
	void close() noexcept;
	void open();
};

#endif //ROTOCOL_DECORATOR_H