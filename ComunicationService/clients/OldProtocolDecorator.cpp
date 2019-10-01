#include "OldProtocolDecorator.hpp"
#include "../MainProjectLoger.hpp"
#include "../Configuration.hpp"
#include <boost/beast/core/detail/base64.hpp>
#include <boost/scoped_array.hpp>

const std::string OldProtocolDecorator::connectHeader("$V23="); // "$V23=" - передача только строковых данных как есть (в конце добавляем признак конца строки)
const char OldProtocolDecorator::startPackage = '#';
const char OldProtocolDecorator::stopPackage = '\n';


void OldProtocolDecorator::emmitNewDataSlot(const message_ptr m) { // Принял данные от clientDelegate передаю их дальше из сети в COM порт
	globalLog.addLog(Loger::L_INFO, "Receive new messages ", m->toString(), ". And try decode it");
	std::size_t sz = m->currentSize();
	if (m->data() != nullptr && sz > 2) { // Парсим сообщение
		if (m->data()[0] == startPackage && m->data()[sz-1] == stopPackage) { // Валидное сообщение
			std::size_t decodedSize = boost::beast::detail::base64::decoded_size(sz-2);
			boost::scoped_array<std::uint8_t> dat(new std::uint8_t[decodedSize]);
			decodedSize = boost::beast::detail::base64::decode(dat.get(), m->toString().c_str()+1, sz-2).first;
			message_ptr newM(new message(decodedSize, dat.get()));
			receiveNewData(newM);
		}
		else {
			auto pos = m->toString().find(connectHeader);
			if (pos != std::string::npos) {
				pos += connectHeader.size();
				message_ptr msg( new message( (m->currentSize()-pos-1), (m->data()+pos) ) );
				receiveNewData(msg);
			}
			else { //TODO Обработка ошибки не корректного пакета
				globalLog.addLog(Loger::L_INFO, "Undefine # in start message ", m->toString());
				receiveNewData(m);
			}
		}

	}
}

OldProtocolDecorator::OldProtocolDecorator(boost::asio::io_service * const srv, const std::string & host, const std::string & port, const std::string & deviceID) {
	ConnectionProperties c;
	auto conf = Configuration::getConfiguration("");
	c.Host = host; c.Port = port;
	c.connectionString = connectHeader + conf->getConfigString("user:name") + ";" + deviceID + ";" + stopPackage; //protocol specific connection string 
	clientDelegate = std::shared_ptr<TcpClient>(new TcpClient(srv, c));
	clientDelegate->receiveNewData.connect(boost::bind(&OldProtocolDecorator::emmitNewDataSlot, this, _1));
}

void OldProtocolDecorator::sendNewData(const message_ptr & msg) {
	if (clientDelegate != nullptr && clientDelegate.get() != nullptr) {
		std::size_t encodedSize = boost::beast::detail::base64::encoded_size(msg->currentSize());
		boost::scoped_array<std::uint8_t> dat(new std::uint8_t[encodedSize+2]);
		dat[0] = startPackage;
		encodedSize = boost::beast::detail::base64::encode((void*)(dat.get()+1), (const void*)msg->data(), msg->currentSize());
		dat[encodedSize + 1] = stopPackage;
		message_ptr res(new message(encodedSize+2, dat.get()));
		clientDelegate->sendNewData(res);
		globalLog.addLog(Loger::L_INFO, "Form result message: ", res->toString());
	}
}

void OldProtocolDecorator::close() noexcept {
	if (clientDelegate != nullptr && clientDelegate.get() != nullptr) {
		clientDelegate->close();
	}
}

void OldProtocolDecorator::open() {
	if (clientDelegate != nullptr && clientDelegate.get() != nullptr) {
		clientDelegate->open();
	}
}
