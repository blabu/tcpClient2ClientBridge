#ifndef SERIAL_CLIENT_H
#define SERIAL_CLIENT_H

#include "BaseClient.hpp"

class SerialClient : public BaseClient {
	static unsigned int receiveDataTimeout;
	boost::asio::serial_port sp;
	const std::string deviceName;
	std::uint32_t speed;
	std::uint8_t flowControl;
	std::uint8_t stopBits;
	std::uint8_t bitSize;

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
	void setProperrties(const std::string& speed, const std::string& bitSize, 
						const std::string& flowControl, const std::string& stop);
	virtual ~SerialClient() { close(); finishSession(); service->stop(); }
};

#endif //SERIAL_CLIENT_H
