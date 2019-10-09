#ifndef BASE64_PROTOCOL_DECORATOR
#define BASE64_PROTOCOL_DECORATOR

#include "BaseProtocolDecorator.hpp"

class Base64ProtocolDecorator : public BaseProtocolDecorator {
	static const std::string headerTemplate;
	Base64ProtocolDecorator() = delete;
	Base64ProtocolDecorator(Base64ProtocolDecorator&) = delete;
protected:
	std::vector<std::uint8_t> formMessage(const std::string& to, messageTypes t, std::size_t sz, const std::uint8_t* data) const override;
	std::string formMessage(const std::string& to, messageTypes t, const std::string& data) const override;
	void parseMessage(const message_ptr m) override;
public:
	Base64ProtocolDecorator(boost::asio::io_service *const srv, const std::string& host, const std::string& port,
		const std::string& deviceID, const std::string& answerIfOK, const std::string& answerIfError) 
		:BaseProtocolDecorator(srv, host, port, deviceID, answerIfOK, answerIfError) {}
	virtual ~Base64ProtocolDecorator() = default;
};

#endif // BASE64_PROTOCOL_DECORATOR