#pragma once
#include "Base64ProtocolDecorator.hpp"
#include "../MainProjectLoger.hpp"
#include "messagesDTO.hpp"

// BinaryProtocolDecorator - добавляет поддержку передачи бинарных данных по протоколу
class BinaryProtocolDecorator :
	public Base64ProtocolDecorator
{
	static const std::string headerTemplateBinary;
	BinaryProtocolDecorator() = delete;
	BinaryProtocolDecorator(BinaryProtocolDecorator&) = delete;
protected: 
	virtual std::vector<std::uint8_t> formMessage(const std::string& to, messageTypes t, std::size_t sz, const std::uint8_t* data) const;
public:
	BinaryProtocolDecorator(boost::asio::io_service *const srv, const std::string& host, const std::string& port,
		const std::string& deviceID, const std::string& answerIfOK, const std::string& answerIfError);
	virtual ~BinaryProtocolDecorator() = default;
};

