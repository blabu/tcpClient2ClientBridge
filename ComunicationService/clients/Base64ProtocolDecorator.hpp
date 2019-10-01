#pragma once
#include "TcpClient.hpp"
#include "boost/bind.hpp"
#include "../MainProjectLoger.hpp"
#include <list>

// Base64ProtocolDecorator - Реализует базовый интерфейс инициализации, авторизации соединения с сервером и передачу данных в строковом виде
class Base64ProtocolDecorator : public IBaseClient {
	static const std::string headerEnd;
	static const std::string headerTemplate;
	static const std::string initOkMessage;
	static const std::string connectOkMessage;
	const std::string answerOK;
	const std::string answerError;
	static std::string base64_encode(std::uint8_t const* data, std::size_t len);
	static std::string base64_encode(std::string const& s);
	std::shared_ptr<TcpClient> clientDelegate;
	void initHandler(const message_ptr m);
	void connectToDeviceHandler(const message_ptr m);
	void emmitNewDataSlot(const message_ptr m);
	std::string randomString(std::size_t size);
	boost::asio::io_service*const srv;
	const std::string host;
	const std::string port;
	std::string connectTo;
	bool isConnected;
	message_ptr savedMsg; // Одно сохраненное сообщение
	Base64ProtocolDecorator() = delete;
	Base64ProtocolDecorator(Base64ProtocolDecorator&) = delete;
protected:
	std::string localName;
	std::string localPass;
	const enum messageTypes {
		initCMD = 6,
		connectCMD = 8,
		dataCMD = 9,
		closeCMD = 10,
	};
	// Переопределения этого метода необходимо в случае бинарного протокола
	virtual std::vector<std::uint8_t> formMessage(const std::string& to, messageTypes t, std::size_t sz, const std::uint8_t* data) const;
	virtual std::string formMessage(const std::string& to, messageTypes t, const std::string& data) const;
public:
	Base64ProtocolDecorator(boost::asio::io_service *const srv, const std::string& host, const std::string& port, 
						const std::string& deviceID, const std::string& answerIfOK, const std::string& answerIfError);
	virtual ~Base64ProtocolDecorator() = default;
	void sendNewData(const message_ptr& msg);
	void close() noexcept;
	void open();
};

