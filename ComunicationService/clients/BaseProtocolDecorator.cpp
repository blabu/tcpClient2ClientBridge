#include "BaseProtocolDecorator.hpp"
#include "../MainProjectLoger.hpp"
#include "../Configuration.hpp"

#include "../picosha2.h"

#include <boost/beast/core/detail/base64.hpp>
#include <boost/scoped_array.hpp>
#include <boost/format.hpp>

#include <cstdlib>
#include <chrono>

const std::string BaseProtocolDecorator::initOkMessage("$V1;0;%s;6;3;6###OK"); // local name
const std::string BaseProtocolDecorator::initOkMsg("OK");
const std::string BaseProtocolDecorator::connectOkMessage("$V1;0;%s;8;3;6###OK"); // local name
const std::string BaseProtocolDecorator::connectOkMsg("OK");

BaseProtocolDecorator::BaseProtocolDecorator(boost::asio::io_service *const srv, const std::string& host, const std::string& port, 
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
}

void BaseProtocolDecorator::initHandler(const message_ptr m){
	globalLog.addLog(Loger::L_TRACE, "In init handler");
	std::string resultMessage(m->toString());
	std::string waitMessage((boost::format(initOkMessage) % localName).str());
	globalLog.addLog(Loger::L_TRACE, "Try compare receive message ", resultMessage, " and wait message ", waitMessage);
	if (resultMessage == waitMessage || resultMessage.find(initOkMsg) != std::string::npos) { // ����������� � ������������� �������
		globalLog.addLog(Loger::L_TRACE, "Try connect to ", connectTo);
		clientDelegate->receiveNewData.disconnect_all_slots();
		clientDelegate->receiveNewData.connect(boost::bind(&BaseProtocolDecorator::connectToDeviceHandler, this, _1));
		clientDelegate->sendNewData(message_ptr(new message(formMessage(connectTo, command::connectCMD, ""))));
		globalLog.addLog(Loger::L_INFO, "Init OK");
	}
	else {
		globalLog.addLog(Loger::L_WARNING, "Message not equal");
		close(); 
		open();
	}
}

// Check CONNECT OK answer connectOkMessage
void BaseProtocolDecorator::connectToDeviceHandler(const message_ptr m) {
	globalLog.addLog(Loger::L_TRACE, "In connect handler");
	std::string resultMessage(m->toString());
	std::string waitMessage ((boost::format(connectOkMessage)%connectTo%localName).str());
	globalLog.addLog(Loger::L_TRACE, "Try compare receive message ", resultMessage, " and wait message ", waitMessage);
	if (resultMessage == waitMessage || resultMessage.find(connectOkMsg) != std::string::npos) { // ����������� � ������� �������
		isConnected = true;
		clientDelegate->receiveNewData.disconnect_all_slots();
		clientDelegate->receiveNewData.connect(boost::bind(&BaseProtocolDecorator::parseMessage, this, _1));
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
bool BaseProtocolDecorator::checkHeader(const header& h) {
	if (h.to != localName) {
		globalLog.addLog(Loger::L_WARNING, "Error localName ", localName, " not equal ", h.to);
	}
	if (h.msgType != command::dataCMD) {
		if (h.msgType == command::errCMD) {
			globalLog.addLog(Loger::L_WARNING, "Error in remote service");
		}
		else {
			globalLog.addLog(Loger::L_ERROR, "Undefined message type");
		}
		return false;
	}
	return true;
}

void BaseProtocolDecorator::sendNewData(const message_ptr & msg) {
	if (clientDelegate != nullptr && clientDelegate.get() != nullptr) {
		if (isConnected) {
			clientDelegate->sendNewData(message_ptr(new message(formMessage(connectTo, command::dataCMD, msg->currentSize(), msg->data()))));
		}
		else {
			if (savedMsg != nullptr && savedMsg.get() != nullptr) {
				globalLog.addLog(Loger::L_ERROR, "Error try send new message ", msg->toString(), " but already have saved message ", savedMsg->toString());
			}
			else {
				globalLog.addLog(Loger::L_WARNING, "Try send message ", msg->toString(), " but not connected yet. Save it");
			}
			savedMsg = msg;
		}
	}
	else {
		globalLog.addLog(Loger::L_ERROR, "Internal client (tcp client) is null");
	}
}

void BaseProtocolDecorator::close() noexcept {
	if (clientDelegate != nullptr && clientDelegate.get() != nullptr) {
		clientDelegate->close();
	}
}

void BaseProtocolDecorator::open() {
	std::vector<unsigned char> hashBin(picosha2::k_digest_size);
	picosha2::hash256(localPass.cbegin(), localPass.cend(), hashBin.begin(), hashBin.end());
	localPass = ProtocolUtils::base64_encode(hashBin.data(), hashBin.size());
	std::string salt(ProtocolUtils::randomString(16));
	globalLog.addLog(Loger::L_DEBUG, "Random salt is ", salt);
	auto signature = localName + salt + localPass;
	picosha2::hash256(signature.cbegin(), signature.cend(), hashBin.begin(), hashBin.end());
	signature = salt + ";" + ProtocolUtils::base64_encode(hashBin.data(), hashBin.size());
	globalLog.addLog(Loger::L_DEBUG, "Signature is ", signature);
	ConnectionProperties c;
	c.connectionString = formMessage("0", command::initCMD, signature);
	c.Host = host; c.Port = port;
	globalLog.addLog(Loger::L_INFO, "Form init string ", c.connectionString);
	clientDelegate = std::shared_ptr<TcpClient>(new TcpClient(srv, c));
	clientDelegate->finishSession.connect([this]() { this->receiveNewData(message_ptr(new message(this->answerError))); this->finishSession(); });
	clientDelegate->receiveNewData.disconnect_all_slots();
	clientDelegate->receiveNewData.connect(boost::bind(&BaseProtocolDecorator::initHandler, this, _1));
	clientDelegate->open();
}
