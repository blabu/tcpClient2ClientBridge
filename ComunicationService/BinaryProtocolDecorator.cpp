#include "BinaryProtocolDecorator.hpp"
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

BinaryProtocolDecorator::BinaryProtocolDecorator(boost::asio::io_service *const srv, const std::string& host, const std::string& port,
	const std::string& deviceID, const std::string& answerIfOK, const std::string& answerIfError) : Base64ProtocolDecorator(srv,host,port,deviceID,answerIfOK,answerIfError){
}
