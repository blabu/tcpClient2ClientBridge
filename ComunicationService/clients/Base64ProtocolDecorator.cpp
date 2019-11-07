#include "Base64ProtocolDecorator.hpp"
#include "../MainProjectLoger.hpp"

#include <boost/format.hpp>
#include <boost/beast/core/detail/base64.hpp>

const std::string Base64ProtocolDecorator::headerTemplate("$V1;%s;%s;%x;4;%x###%s"); // localName, toName, messageType, messageSize, message

std::vector<std::uint8_t> Base64ProtocolDecorator::formMessage(const std::string& to, messageTypes t, std::size_t sz, const std::uint8_t* data) const {
	auto res = formMessage(to, t, ProtocolUtils::base64_encode(data, sz));
	return std::vector<std::uint8_t>(res.cbegin(), res.cend());
}

std::string Base64ProtocolDecorator::formMessage(const std::string& to, messageTypes t, const std::string& data) const {
	return (boost::format(headerTemplate)%localName%to%t%data.size()%data).str();
}

void Base64ProtocolDecorator::parseMessage(const message_ptr m) { // ѕрин€л данные от clientDelegate передаю их дальше из сети в COM порт
	globalLog.addLog(Loger::L_INFO, "Receive new messages ", m->toString(), ". And try decode it");
	header head;
	try {
		head = ProtocolUtils::parseHeader(m);
	}
	catch (const std::invalid_argument& er) {
		globalLog.addLog(Loger::L_ERROR, er.what());
		return;
	}
	if (!checkHeader(head)) return;
	if (head.headerSize == 0 || head.packetSize == 0) return;
	std::size_t decodedSize = boost::beast::detail::base64::decoded_size(head.packetSize);
	boost::scoped_array<std::uint8_t> dat(new std::uint8_t[decodedSize]);
	decodedSize = boost::beast::detail::base64::decode(dat.get(), m->toString().c_str() + head.headerSize, head.packetSize).first;
	receiveNewData(message_ptr(new message(decodedSize, dat.get())));
}
