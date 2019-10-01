#include "ProtocolUtils.hpp"
#include <chrono>

#include <boost/algorithm/string/split.hpp>
#include <boost/beast/core/detail/base64.hpp>


const std::string ProtocolUtils::headerEnd("###");

std::string ProtocolUtils::base64_encode(std::uint8_t const* data, std::size_t len) {
	std::string dest;
	dest.resize(boost::beast::detail::base64::encoded_size(len));
	dest.resize(boost::beast::detail::base64::encode(&dest[0], data, len));
	return dest;
}

std::string ProtocolUtils::base64_encode(std::string const& s) {
	return base64_encode(reinterpret_cast<std::uint8_t const*>(s.data()), s.size());
}

header ProtocolUtils::parseHeader(const message_ptr m) {
	header res;
	auto mess(m->toString());
	auto headerSize = mess.find(headerEnd);
	if (headerSize == std::string::npos) { // НЕ Нашли конец заголовка
		throw std::invalid_argument("Undefined header size or can not find header end in " + mess);
	}
	std::string header(m->data(), m->data() + headerSize);
	std::vector<std::string> parsedParam;
	boost::split(parsedParam, header, [](char ch) { return ch == ';'; });
	if (parsedParam.size() != 5) {
		throw std::invalid_argument("Incorrect number of data in header " + header);
	}
	res.headerSize = headerSize + headerEnd.size();
	res.to = parsedParam[2];
	res.from = parsedParam[1];
	res.msgType = static_cast<messageTypes>(std::stoi(parsedParam[3], 0, 16));
	res.packetSize = std::stoi(parsedParam[4], 0, 16);
	return res;
}


std::string ProtocolUtils::randomString(std::size_t size) {
	static const char alfabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	std::srand(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));
	std::string res;
	res.reserve(size);
	for (std::size_t i = 0; i < size; i++) {
		res.push_back(alfabet[rand() % sizeof(alfabet)]);
	}
	return res;
}