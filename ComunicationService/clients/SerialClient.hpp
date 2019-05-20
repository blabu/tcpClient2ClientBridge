#ifndef SERIAL_CLIENT_H
#define SERIAL_CLIENT_H

#include <BaseClient.hpp>

class SerialClient : public BaseClient {
	static unsigned int receiveDataTimeout;
	boost::asio::serial_port sp;
	const std::string deviceName;
	void readHandler(const boost::system::error_code& er, std::size_t byteTransfered);
	void read();
	void write();
public:
	SerialClient(boost::asio::io_service* const srv, const std::string& device);
	void close() noexcept {
		if (sp.is_open()) { boost::system::error_code er; sp.cancel(er); sp.close(er); }
	}
	void open();
	static void setReadTimeout(unsigned int timeout) {
		receiveDataTimeout = timeout;
	}
	virtual ~SerialClient() { close(); finishSession(); service->stop(); }
};

#endif //SERIAL_CLIENT_H