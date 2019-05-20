#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "BaseClient.hpp"

/*
������� DTO ��� ����������� ���� ����������� ��� ������ TcpClient ������
*/
struct ConnectionProperties {
	std::string Host;
	std::string Port;
	std::string connectionString; // ������, ������� ����� ���������� ������� ����� ����� ��������� �������
};

//TcpClient - ��������� ��������� �������� ������� ��� TCP ����������
class TcpClient : public BaseClient {
	static unsigned int receiveDataTimeout;
	const ConnectionProperties connection; // ��������� ���������� (����� ��� ���������������)
	std::chrono::seconds connectionTimeout;// ������� ����������
	std::chrono::seconds reConnectionTimeout;
	boost::asio::ip::tcp::socket sock;
	boost::asio::deadline_timer watchdog;
	boost::asio::deadline_timer timer;
	boost::asio::ip::tcp::resolver resolver;
	 
	void timeoutHandler(const boost::system::error_code& er); 	/*���������� �������� (������ ���������)*/
	void connectionHandler(boost::system::error_code er, boost::asio::ip::tcp::endpoint p);
	void connect();
	void readHandler(boost::system::error_code er, std::size_t sz);
	void read();
	void write();

	TcpClient() = delete;
	TcpClient(TcpClient&) = delete;
public:
	TcpClient(boost::asio::io_service *const srv, const ConnectionProperties& c);
	void open() {
		connect();
	}
	void close() noexcept {
		if (sock.is_open()) { boost::system::error_code er; sock.cancel(er); sock.close(er); }
	}
	virtual ~TcpClient();
	static void setReadTimeout(unsigned int timeout) {
		receiveDataTimeout = timeout;
	}
};

#endif // TCP_CLIENT_H