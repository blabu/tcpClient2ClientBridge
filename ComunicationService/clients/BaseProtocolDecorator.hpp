#ifndef BASE_PROTOCOL_DECORATOR
#define BASE_PROTOCOL_DECORATOR

#include "TcpClient.hpp"
#include "ProtocolUtils.hpp"
#include <boost/bind.hpp>

// BaseProtocolDecorator - ��������� ������� ��������� �������������, ����������� ���������� � �������� � �������� ������ � ��������� ����
class BaseProtocolDecorator : public IBaseClient {
	static const std::string initOkMessage;
	static const std::string connectOkMessage;
	const std::string answerOK;
	const std::string answerError;
	bool isConnected;
	std::shared_ptr<TcpClient> clientDelegate;
	boost::asio::io_service*const srv;
	const std::string host;
	const std::string port;
	std::string connectTo;
	message_ptr savedMsg; // ���� ����������� ���������
	std::string localPass;
private: //methods
	void initHandler(const message_ptr m);
	void connectToDeviceHandler(const message_ptr m);
	BaseProtocolDecorator() = delete;
	BaseProtocolDecorator(BaseProtocolDecorator&) = delete;
protected:
	std::string localName;
	bool checkHeader(const header& h);
	// formMessage - ��������������� ����� ������ ���������� � ������ ��������� ��������� ��� ������� �������� ������
	virtual std::vector<std::uint8_t> formMessage(const std::string& to, messageTypes t, std::size_t sz, const std::uint8_t* data) const = 0;
	virtual std::string formMessage(const std::string& to, messageTypes t, const std::string& data) const = 0;
	// parseMessage - ��������������� ����� ������ ���������� � ������ ��������� ��������� ��� ��������� �������� ������
	virtual void parseMessage(const message_ptr m) = 0;
public:
	BaseProtocolDecorator(boost::asio::io_service *const srv, const std::string& host, const std::string& port, 
						const std::string& deviceID, const std::string& answerIfOK, const std::string& answerIfError);
	virtual ~BaseProtocolDecorator() = default;
	void sendNewData(const message_ptr& msg);
	void close() noexcept;
	void open();
};

#endif //BASE_PROTOCOL_DECORATOR