#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "TcpClient.hpp"
#include "../MainProjectLoger.hpp"

unsigned int TcpClient::receiveDataTimeout;

/*���������� �������� (������ ���������)*/
void TcpClient::timeoutHandler(const boost::system::error_code & er) {
	if (!er) {
		globalLog.addLog(Loger::L_WARNING, "Timeout session");
		boost::system::error_code er;
		sock.cancel(er);
		if (!er) { globalLog.addLog(Loger::L_WARNING, "Error in timeout handler when try cancel all operation"); }
		if (sock.is_open()) { boost::system::error_code er; sock.close(er); }
	}
	else if (er != boost::asio::error::operation_aborted) {
		globalLog.addLog(Loger::L_ERROR, er.message(), " Error in timeout handler");
	}
}

void TcpClient::connectionHandler(boost::system::error_code er, boost::asio::ip::tcp::endpoint p) {
	if (!er) { // ����������� �������
		globalLog.addLog(Loger::L_TRACE, "Connection fine to the remote host");
		updateTimer(watchdog, boost::bind(&TcpClient::timeoutHandler, this, _1), connectionTimeout);
		message_ptr msg(new message(connection.connectionString.size(), (std::uint8_t*)(connection.connectionString.c_str())));
		globalLog.addLog(Loger::L_TRACE, "Form message ", (char*)msg->data());
		MessagesQueue.push_back(msg);
		write();
		read();
	}
	else { // ���� ������ ����������� ������� ��� ��� ����� reConnectionTimeout �������
		globalLog.addLog(Loger::L_ERROR, "Error when try open connection to the server host ", er.message());
		updateTimer(timer, [this](const boost::system::error_code& er) {connect(); }, reConnectionTimeout);
	}
}

void TcpClient::connect() {
	auto entryPoints = resolver.resolve(connection.Host, connection.Port);
	boost::system::error_code er;
	sock.cancel(er);
	if (er) {
		globalLog.addLog(Loger::L_WARNING, er.message().c_str());
	}
	globalLog.addLog(Loger::L_INFO, "All function has canceled before create new connection");
	boost::asio::async_connect(sock, entryPoints, boost::bind(&TcpClient::connectionHandler, this, _1, _2));
	// ���� ����������� �������� �������� ��������������� ������
}

void TcpClient::readHandler(boost::system::error_code er, std::size_t sz) {
	if (!er) {
		if (sz) {
			updateTimer(readTimer,
				[=](const boost::system::error_code& er) {
				if (!er) {
					this->service->post(boost::bind(&TcpClient::readHandler, this, er, 0));
				}
			},
				readTimeout);
			updateTimer(watchdog, boost::bind(&TcpClient::timeoutHandler, this, _1), connectionTimeout);
			readBuffer += *(readBuffer.data() + readBuffer.size() - 1);
			read(); // ������ �����, ����� ������� ���� ����� �� ��� ����� ���� �� ���������� ������ ������
		}
		else { // ���� �� �������, �� �������� (������ � ������� readTimeout ������� �� ������ �� ���� ����)
			message_ptr m(new message(readBuffer)); // �������� �������� ���������
			globalLog.addLog(Loger::L_TRACE, "Message from socket received ", (char*)readBuffer.data());
			readBuffer.clear(); // ������ ����� �������
			receiveNewData(m);  // ���������� ������
								// ��� ���� �������� ������ � ������ ��������
		}
	}
	else { // ������ ������ �������� � ���������� �������� ������
		if (sock.is_open()) { boost::system::error_code er; sock.close(er); } // TODO �������� ������ ������ �������� � ���������� ����������
		globalLog.addLog(Loger::L_ERROR, "Error when try read from remote connection");
		globalLog.addLog(Loger::L_ERROR, er.message().c_str());
	}
}

void TcpClient::read() {
	boost::asio::async_read(sock, boost::asio::buffer((readBuffer.data() + readBuffer.size() - 1), 1),
		boost::bind(&TcpClient::readHandler, this, _1, _2));
}

void TcpClient::write() {
	if (!MessagesQueue.empty()) {
		updateTimer(watchdog, boost::bind(&TcpClient::timeoutHandler, this, _1), connectionTimeout);
		message_ptr newMsg(MessagesQueue.back());

		boost::asio::async_write(sock, boost::asio::buffer(newMsg->data(), newMsg->currentSize()),
			[this](boost::system::error_code er, std::size_t length) {
			if (!er) {
				if (length == MessagesQueue.back()->currentSize()) { MessagesQueue.pop_back(); }
				if (MessagesQueue.size() > 0) { write(); }
			}
			else { // ������� ���������������
				globalLog.addLog(Loger::L_ERROR, "Error when try write to the server");
				connect();
			}
		});
	}
}

TcpClient::TcpClient(boost::asio::io_service * const srv, const ConnectionProperties & c) :
	BaseClient(srv, 4096, std::chrono::milliseconds(receiveDataTimeout)),
	connection(c),
	connectionTimeout(std::chrono::seconds(90)),
	reConnectionTimeout(std::chrono::seconds(30)),
	sock(*service),
	watchdog(*service),
	timer(*service),
	resolver(*service) {
	globalLog.addLog(Loger::L_INFO, "Try open connection");
}

TcpClient::~TcpClient() {
	globalLog.addLog(Loger::L_INFO, "Delete TCP client");
	globalLog.snapShot();
}
