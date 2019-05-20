#include "ProtocolDecorator.hpp"

const std::string ProtocolDecorator::connectHeader("$V13="); // "$V13=" - передача только строковых данных как есть (в конце добавляем признак конца строки)

ProtocolDecorator::ProtocolDecorator(boost::asio::io_service * const srv, const std::string & host, const std::string & port, const std::string & key, const std::string & deviceID) {
	ConnectionProperties c;
	c.Host = host; c.Port = port;
	c.connectionString = connectHeader + key + ";" + deviceID + ";\n"; //protocol specific connection string 
	clientDelegate = std::shared_ptr<TcpClient>(new TcpClient(srv, c));
	clientDelegate->receiveNewData.connect(boost::bind(&ProtocolDecorator::emmitNewDataSlot, this, _1));
}

void ProtocolDecorator::sendNewData(const message_ptr & msg) {
	if (clientDelegate != nullptr && clientDelegate.get() != nullptr) {
		msg->push_back('\n');
		clientDelegate->sendNewData(msg);
	}
}

void ProtocolDecorator::close() noexcept {
	if (clientDelegate != nullptr && clientDelegate.get() != nullptr) {
		clientDelegate->close();
	}
}

void ProtocolDecorator::open() {
	if (clientDelegate != nullptr && clientDelegate.get() != nullptr) {
		clientDelegate->open();
	}
}
