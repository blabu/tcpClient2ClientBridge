#pragma once
#include "TcpClient.hpp"
#include "boost/bind.hpp"

class NewProtocolDecorator : public IBaseClient
{
	static const std::string initMessage;
	static const std::string initOkMessage;
	static const std::string connectMessage;
	static const std::string connectOkMessage;
	static const std::string dataCMD;
	static const std::string closeCMD;
	std::string localName;
	std::string localPass;
	std::shared_ptr<TcpClient> clientDelegate;
	void initHandler(const message_ptr m);
	std::string randomString(std::size_t size);
public:
	NewProtocolDecorator(boost::asio::io_service *const srv, const std::string& host, const std::string& port, const std::string& deviceID);
	virtual ~NewProtocolDecorator() = default;
	void sendNewData(const message_ptr& msg);
	void close() noexcept;
	void open();
};

