#include "BinaryProtocolDecorator.hpp"
#include "ProtocolUtils.hpp"
#include <boost/format.hpp>

const std::string BinaryProtocolDecorator::headerTemplateBinary("$V1;%s;%s;%x;%x###"); // localName, toName, messageType, messageSize, message

std::vector<std::uint8_t> BinaryProtocolDecorator::formMessage(const std::string & to, messageTypes t, std::size_t sz, const std::uint8_t * data) const {
	globalLog.addLog(Loger::L_TRACE, "BinaryProtocolDecorator::formMessage to ", to);
	auto header = (boost::format(headerTemplateBinary)%localName%to%t%sz).str();
	std::vector<std::uint8_t> res(header.cbegin(), header.cend());
	for (std::size_t i = 0; i < sz; i++) res.push_back(data[i]);
	globalLog.addLog(Loger::L_TRACE, "BinaryProtocolDecorator::formMessage fine");
	return res;
}

std::string BinaryProtocolDecorator::formMessage(const std::string & to, messageTypes t, const std::string & data) const {
	return (boost::format(headerTemplateBinary)%localName%to%t%data.size()).str().append(data);
}

void BinaryProtocolDecorator::parseMessage(const message_ptr m){
	header head;
	try {
		head = ProtocolUtils::parseHeader(m);
	}
	catch (std::invalid_argument er) {
		globalLog.addLog(Loger::L_ERROR, er.what());
		return;
	}
	if (!checkHeader(head)) return;
	globalLog.addLog(Loger::L_DEBUG, "Receive binary message size ", std::to_string(head.packetSize));
	receiveNewData(message_ptr(new message(head.packetSize, m->data()+head.headerSize)));
}

