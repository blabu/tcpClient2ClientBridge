#include "NewProtocolDecorator.hpp"
#include "../MainProjectLoger.hpp"
#include "../Configuration.hpp"
#include "../picosha2.h"

#include <boost/beast/core/detail/base64.hpp>
#include <boost/scoped_array.hpp>
#include <boost/format.hpp>

#include <cstdlib>
#include <chrono>

const std::string NewProtocolDecorator::initMessage("$V1;%s;0;6;%x###%s"); // local name, message size, (salt + ; + BASE64(SHA256(name + salt + BASE64(SHA256(name + password)))
const std::string NewProtocolDecorator::initOkMessage("$V1;0;%s;6;7###INIT OK"); // local name
const std::string NewProtocolDecorator::connectMessage("$V1;%s;%s;8;0###"); //local name, to name
const std::string NewProtocolDecorator::connectOkMessage("$V1;%s;%s;8;a###CONNECT OK"); // to name, local name
const std::string NewProtocolDecorator::dataCMD("$V1;%s;%s;9;%x###"); //local name, to, message size
const std::string NewProtocolDecorator::closeCMD("$V1;%s;%s;a;0###"); //remote host close connection

NewProtocolDecorator::NewProtocolDecorator(boost::asio::io_service *const srv, const std::string& host, const std::string& port, const std::string& deviceID) {
	auto conf = Configuration::getConfiguration("");
	localName = conf->getConfigString("user:name");
	localPass = localName + conf->getConfigString("user:pass");
	std::vector<unsigned char> hashBin(picosha2::k_digest_size);
	picosha2::hash256(localPass.cbegin(), localPass.cend(), hashBin.begin(), hashBin.end());
	localPass = boost::beast::detail::base64_encode(hashBin.data(), hashBin.size());
	auto salt = randomString(16);
	auto signature = localName + salt + localPass;
	picosha2::hash256(signature.cbegin(), signature.cend(), hashBin.begin(), hashBin.end());
	signature = salt + ";" + boost::beast::detail::base64_encode(hashBin.data(), hashBin.size());
	ConnectionProperties c;
	c.connectionString = (boost::format(initMessage)%localName %signature.size() %signature).str();
	c.Host = host; c.Port = port;
	clientDelegate = std::shared_ptr<TcpClient>(new TcpClient(srv, c));
	clientDelegate->receiveNewData.connect(boost::bind(&NewProtocolDecorator::initHandler, this, _1));
}

void NewProtocolDecorator::initHandler(const message_ptr m){
	//TODO check INIT OK answer initOkMessage
}

std::string NewProtocolDecorator::randomString(std::size_t size) {
	static const char alfabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	auto time = std::chrono::system_clock::now();
	std::srand(time.time_since_epoch().count());
	std::string res;
	res.reserve(size);
	for (std::size_t i = 0; i < size; i++) {
		res[i] = alfabet[rand()%sizeof(alfabet)];
	}
	return res;
}

void NewProtocolDecorator::sendNewData(const message_ptr & msg) {
	if (clientDelegate != nullptr && clientDelegate.get() != nullptr) {
		std::size_t encodedSize = boost::beast::detail::base64::encoded_size(msg->currentSize());
		boost::scoped_array<std::uint8_t> dat(new std::uint8_t[encodedSize + 2]);
		//dat[0] = startPackage;
		encodedSize = boost::beast::detail::base64::encode((void*)(dat.get() + 1), (const void*)msg->data(), msg->currentSize());
		//dat[encodedSize + 1] = stopPackage;
		message_ptr res(new message(encodedSize + 2, dat.get()));
		clientDelegate->sendNewData(res);
		globalLog.addLog(Loger::L_INFO, "Form result message: ", res->toString());
	}
}

void NewProtocolDecorator::close() noexcept {
	if (clientDelegate != nullptr && clientDelegate.get() != nullptr) {
		clientDelegate->close();
	}
}

void NewProtocolDecorator::open() {
	if (clientDelegate != nullptr && clientDelegate.get() != nullptr) {
		clientDelegate->open();
	}
}
