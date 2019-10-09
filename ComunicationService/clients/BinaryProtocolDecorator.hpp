#ifndef BINARY_PROTOCOL_DECORATOR
#define BINARY_PROTOCOL_DECORATOR

#include "BaseProtocolDecorator.hpp"
#include "../MainProjectLoger.hpp"
#include "messagesDTO.hpp"

// BinaryProtocolDecorator - ��������� ��������� �������� �������� ������ �� ���������
class BinaryProtocolDecorator : public BaseProtocolDecorator {
	static const std::string headerTemplateBinary;
	BinaryProtocolDecorator() = delete;
	BinaryProtocolDecorator(BinaryProtocolDecorator&) = delete;
protected: 
	std::vector<std::uint8_t> formMessage(const std::string& to, messageTypes t, std::size_t sz, const std::uint8_t* data) const override;
	std::string formMessage(const std::string& to, messageTypes t, const std::string& data) const override;
	void parseMessage(const message_ptr m) override;
public:
	BinaryProtocolDecorator(boost::asio::io_service *const srv, const std::string& host, const std::string& port,
		const std::string& deviceID, const std::string& answerIfOK, const std::string& answerIfError)
		:BaseProtocolDecorator(srv, host, port, deviceID, answerIfOK, answerIfError) {}
	virtual ~BinaryProtocolDecorator() = default;
};

#endif //BINARY_PROTOCOL_DECORATOR