#include "NewProtocolDecorator.hpp"
#include "../MainProjectLoger.hpp"
#include "../Configuration.hpp"
#include "../picosha2.h"

#include <boost/beast/core/detail/base64.hpp>
#include <boost/scoped_array.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/split.hpp>

#include <cstdlib>
#include <chrono>

std::string NewProtocolDecorator::base64_encode(std::uint8_t const* data, std::size_t len) {
	std::string dest;
	dest.resize(boost::beast::detail::base64::encoded_size(len));
	dest.resize(boost::beast::detail::base64::encode(&dest[0], data, len));
	return dest;
}

std::string NewProtocolDecorator::base64_encode(std::string const& s) {
	return base64_encode(reinterpret_cast<std::uint8_t const*>(s.data()), s.size());
}

const std::string NewProtocolDecorator::headerEnd("###");
const std::string NewProtocolDecorator::headerTemplate("$V1;%s;%s;%x;%x###%s"); // localName, toName, messageType, messageSize, message
const std::string NewProtocolDecorator::initOkMessage("$V1;0;%s;6;7###INIT OK"); // local name
const std::string NewProtocolDecorator::connectOkMessage("$V1;%s;%s;8;a###CONNECT OK"); // to name, local name

NewProtocolDecorator::NewProtocolDecorator(boost::asio::io_service *const srv, const std::string& host, const std::string& port, 
										   const std::string& deviceID, const std::string& answerIfOK, const std::string& answerIfError) : 
	answerOK(answerIfOK),
	answerError(answerIfError),
	srv(srv), 
	host(host), 
	port(port), 
	connectTo(deviceID) {
	auto conf = Configuration::getConfiguration("");
	localName = conf->getConfigString("user:name");
	localPass = localName + conf->getConfigString("user:pass");
	globalLog.addLog(Loger::L_DEBUG, "User name is ", localName);
	isConnected = false;
	std::vector<unsigned char> hashBin(picosha2::k_digest_size);
	picosha2::hash256(localPass.cbegin(), localPass.cend(), hashBin.begin(), hashBin.end());
	localPass = base64_encode(hashBin.data(), hashBin.size());
	std::string salt(randomString(16));
	globalLog.addLog(Loger::L_DEBUG, "Random salt is ", salt);
	auto signature = localName + salt + localPass;
	picosha2::hash256(signature.cbegin(), signature.cend(), hashBin.begin(), hashBin.end());
	signature = salt + ";" + base64_encode(hashBin.data(), hashBin.size());
	globalLog.addLog(Loger::L_DEBUG, "Signature is ", signature);
	ConnectionProperties c;
	c.connectionString = formMessage("0", messageTypes::initCMD, signature);
	c.Host = host; c.Port = port;
	globalLog.addLog(Loger::L_INFO, "Form init string ", c.connectionString);
	clientDelegate = std::shared_ptr<TcpClient>(new TcpClient(srv, c));
	clientDelegate->finishSession.connect([this]() { this->receiveNewData(message_ptr(new message(this->answerError))); this->finishSession(); });
}

std::string NewProtocolDecorator::formMessage(const std::string& to, messageTypes t, std::size_t sz, const std::uint8_t* data) const {
	return formMessage(to, t, base64_encode(data,sz));
}

std::string NewProtocolDecorator::formMessage(const std::string& to, messageTypes t, const std::string& data) const {
	return (boost::format(headerTemplate)%localName%to%t%data.size()%data).str();
}

//check INIT OK answer initOkMessage
void NewProtocolDecorator::initHandler(const message_ptr m){
	globalLog.addLog(Loger::L_TRACE, "In init handler");
	std::string resultMessage(m->toString());
	std::string waitMessage((boost::format(initOkMessage) % localName).str());
	globalLog.addLog(Loger::L_TRACE, "Try compare receive message ", resultMessage, " and wait message ", waitMessage);
	if (resultMessage == waitMessage) { // Подключение и инициализация удалась
		globalLog.addLog(Loger::L_TRACE, "Try connect to ", connectTo);
		clientDelegate->receiveNewData.disconnect_all_slots();
		clientDelegate->receiveNewData.connect(boost::bind(&NewProtocolDecorator::connectToDeviceHandler, this, _1));
		clientDelegate->sendNewData(message_ptr(new message(formMessage(connectTo, messageTypes::connectCMD, ""))));
		globalLog.addLog(Loger::L_INFO, "Init OK");
	}
	else {
		globalLog.addLog(Loger::L_WARNING, "Message not equal");
		close(); 
		open();
	}
}

void NewProtocolDecorator::connectToDeviceHandler(const message_ptr m) {
	globalLog.addLog(Loger::L_TRACE, "In connect handler");
	std::string resultMessage(m->toString());
	std::string waitMessage ((boost::format(connectOkMessage)%connectTo%localName).str());
	globalLog.addLog(Loger::L_TRACE, "Try compare receive message ", resultMessage, " and wait message ", waitMessage);
	if (resultMessage == waitMessage) { // Подключение к клиенту удалось
		isConnected = true;
		clientDelegate->receiveNewData.disconnect_all_slots();
		clientDelegate->receiveNewData.connect(boost::bind(&NewProtocolDecorator::emmitNewDataSlot, this, _1));
		globalLog.addLog(Loger::L_INFO, "Connect OK");
		receiveNewData(message_ptr(new message(answerOK)));
		if (savedMsg != nullptr && savedMsg.get() != nullptr) {
			sendNewData(savedMsg);
		}
	}
	else {
		globalLog.addLog(Loger::L_ERROR, "Connection can not be establish ", resultMessage);
		receiveNewData(message_ptr(new message(answerError)));
	}
}

void NewProtocolDecorator::emmitNewDataSlot(const message_ptr m) { // Принял данные от clientDelegate передаю их дальше из сети в COM порт
	globalLog.addLog(Loger::L_INFO, "Receive new messages ", m->toString(), ". And try decode it");
	auto mess(m->toString());
	auto headerSize = mess.find_first_of(headerEnd);
	if (headerSize == std::string::npos) { // НЕ Нашли конец заголовка
		globalLog.addLog(Loger::L_ERROR, "Error when try find end of header in message ", mess);
		return;
	}
	std::string header(m->data(), m->data() + headerSize);
	std::vector<std::string> parsedParam;
	boost::split(parsedParam, header, [](char ch) { return ch == ';'; });
	if (parsedParam.size() != 5) {
		globalLog.addLog(Loger::L_ERROR, "Error when try parse header ", header);
		return;
	}
	if (parsedParam[2] != localName) {
		globalLog.addLog(Loger::L_ERROR, "Error localName ", localName, " not equal ", parsedParam[2]);
	}
	auto messageType = std::stoi(parsedParam[3], 0, 16);
	if (messageType != messageTypes::dataCMD) {
		if (messageType == messageTypes::closeCMD) {
			isConnected = false;
			globalLog.addLog(Loger::L_WARNING, "Remote host close connection");
			return;
		}
		else {
			globalLog.addLog(Loger::L_ERROR, "Error message type not a data, but ", parsedParam[3]);
			return;
		}
	}	
	std::size_t sz = std::stoi(parsedParam[4], 0, 16);
	std::size_t decodedSize = boost::beast::detail::base64::decoded_size(sz);
	boost::scoped_array<std::uint8_t> dat(new std::uint8_t[decodedSize]);
	decodedSize = boost::beast::detail::base64::decode(dat.get(), m->toString().c_str()+headerSize+headerEnd.size(), sz).first;
	receiveNewData(message_ptr(new message(decodedSize, dat.get())));
}

std::string NewProtocolDecorator::randomString(std::size_t size) {
	static const char alfabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	std::srand(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));
	std::string res;
	res.reserve(size);
	for (std::size_t i = 0; i < size; i++) {
		res.push_back(alfabet[rand()%sizeof(alfabet)]);
	}
	return res;
}

void NewProtocolDecorator::sendNewData(const message_ptr & msg) {
	if (clientDelegate != nullptr && clientDelegate.get() != nullptr) {
		if (isConnected) {
			clientDelegate->sendNewData(message_ptr(new message(formMessage(connectTo, messageTypes::dataCMD, msg->currentSize(), msg->data()))));
		}
		else {
			if (savedMsg != nullptr && savedMsg.get() != nullptr) {
				globalLog.addLog(Loger::L_ERROR, "Error try send new message ", msg->toString(), " but already have saved message ", savedMsg->toString());
			}
			else {
				globalLog.addLog(Loger::L_WARNING, "Try send message ", msg->toString(), " but not connected yet");
			}
			savedMsg = msg;
		}
	}
	else {
		globalLog.addLog(Loger::L_ERROR, "Internal client (tcp client) is null");
	}
}

void NewProtocolDecorator::close() noexcept {
	if (clientDelegate != nullptr && clientDelegate.get() != nullptr) {
		clientDelegate->close();
	}
}

void NewProtocolDecorator::open() {
	if (clientDelegate != nullptr && clientDelegate.get() != nullptr) {
		globalLog.addLog(Loger::L_TRACE, "Try open connection to the server ", host, ":", port);
		clientDelegate->receiveNewData.disconnect_all_slots();
		clientDelegate->receiveNewData.connect(boost::bind(&NewProtocolDecorator::initHandler, this, _1)); // Прием сообщения
		clientDelegate->open();
	}
	else {
		globalLog.addLog(Loger::L_ERROR, "Can not establish connection. Tcp client is null");
	}
}
