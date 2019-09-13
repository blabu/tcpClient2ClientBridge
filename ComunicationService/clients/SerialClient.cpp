#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "SerialClient.hpp"
#include "../MainProjectLoger.hpp"

unsigned int SerialClient::receiveDataTimeout;

void SerialClient::readHandler(const boost::system::error_code & er, std::size_t byteTransfered) {
	if (!er) {
		if (byteTransfered) {
			updateTimer(readTimer,
				[=](const boost::system::error_code& er) {
				if (!er) {
					this->service->post(boost::bind(&SerialClient::readHandler, this, er, 0));
				}
			},
				readTimeout);
			readBuffer += *(readBuffer.data() + readBuffer.size() - 1);
			read();
			globalLog.addLog(Loger::L_TRACE, "Received message for now ", (char*)readBuffer.data());
		}
		else { // byteTransfered == 0 Значит сюда мы попали по таймоуту все сообщение передано
			message_ptr receivedMsg(new message(readBuffer));
			globalLog.addLog(Loger::L_TRACE, "Result message ", (char*)readBuffer.data());
			receiveNewData(receivedMsg);
			readBuffer.clear();
		}
	}
	else {
		globalLog.addLog(Loger::L_ERROR, "Error when try read from serial port");
		//TODO обработка ошибок
	}
}

void SerialClient::read() {
	globalLog.addLog(Loger::L_TRACE, "Try read some messages from serial port");
	sp.async_read_some(boost::asio::buffer((readBuffer.data() + readBuffer.size() - 1), 1),
		boost::bind(&SerialClient::readHandler, this, _1, _2));
}

void SerialClient::write() {
	if (!MessagesQueue.empty()) {
		message_ptr newMsg(MessagesQueue.back());
		sp.async_write_some(boost::asio::buffer(newMsg->data(), newMsg->currentSize()),
			[this](const boost::system::error_code& er, std::size_t length) {
			if (!er) {
				if (length == MessagesQueue.back()->currentSize()) { MessagesQueue.pop_back(); }
				if (!MessagesQueue.empty()) { write(); }
			}
			else {
				globalLog.addLog(Loger::L_ERROR, "Error when try write to serial port");
				// TODO Обработка ошибки
			}
		});
	}
}

SerialClient::SerialClient(boost::asio::io_service * const srv, const std::string & device) :
	BaseClient(srv, 4096, std::chrono::milliseconds(receiveDataTimeout)),
	sp(*srv),
	deviceName(device) {

}

void SerialClient::open() {
	boost::system::error_code er;
	sp.open(deviceName, er);
	if (!er) {
		if(speed != 0) sp.set_option(boost::asio::serial_port_base::baud_rate(speed));
		else sp.set_option(boost::asio::serial_port_base::baud_rate(921600));
		if(bitSize != 0) sp.set_option(boost::asio::serial_port_base::character_size(bitSize));
		switch(stopBits) {
		case 1:	sp.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
			break;
		case 2:	sp.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::onepointfive));
			break;
		case 3: sp.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::two));
			break;
		}
		switch (flowControl) {
		case 1:  sp.set_option(boost::asio::serial_port::flow_control(boost::asio::serial_port::flow_control::none));
			break;
		case 2:
			sp.set_option(boost::asio::serial_port::flow_control(boost::asio::serial_port::flow_control::software));
			break;
		case 3:
			sp.set_option(boost::asio::serial_port::flow_control(boost::asio::serial_port::flow_control::hardware));
			break;
		}
		read();
	}
	else {
		globalLog.addLog(Loger::L_ERROR, "Error when try open COM port ", std::string(deviceName));
	}
}

void SerialClient::setProperrties(const std::string & speed, const std::string & bitSize, const std::string & flowControl, const std::string & stop) {
	try {
		this->speed = std::stoi(speed);
	}
	catch (const std::invalid_argument&) {
		globalLog.addLog(Loger::L_WARNING, "Invalid argument for speed detection");
	}
	if (this->speed != 1200 ||
		this->speed != 2400 ||
		this->speed != 4800 ||
		this->speed != 9600 ||
		this->speed != 19200 ||
		this->speed != 38400 ||
		this->speed != 57600 ||
		this->speed != 115200 ||
		this->speed != 230400 ||
		this->speed != 460800 ||
		this->speed != 921600) this->speed = 921600;
	try {	
		this->bitSize = std::stoi(bitSize);
	}
	catch (const std::invalid_argument&) {
		globalLog.addLog(Loger::L_WARNING, "Invalid argument for bitSize detection");
	}
	if(this->bitSize > 8 || !this->bitSize) this->bitSize = 8;
	
	if (flowControl == "none") this->flowControl = 1;
	else if (flowControl == "soft") this->flowControl = 2;
	else if (flowControl == "hard") this->flowControl = 3;

	if (stop == "1") stopBits = 1;
	else if (stop == "1.5") stopBits = 2;
	else if(stop == "2") stopBits = 3;
}
