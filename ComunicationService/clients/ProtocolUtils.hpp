#ifndef PROTOCOL_UTILS
#define PROTOCOL_UTILS

#include "messagesDTO.hpp"

const enum messageTypes {
	initCMD = 6,
	connectCMD = 8,
	dataCMD = 9,
	closeCMD = 10,
};

struct header {
	std::size_t headerSize;
	std::size_t packetSize;
	messageTypes msgType;
	std::string from;
	std::string to;
};

class ProtocolUtils
{
	ProtocolUtils() = delete;
public:
	~ProtocolUtils() = default;
public:
	static const std::string headerEnd;
	static std::string base64_encode(std::uint8_t const* data, std::size_t len);
	static std::string base64_encode(std::string const& s);
	static header parseHeader(const message_ptr m);
	static std::string randomString(std::size_t size);
};

#endif //PROTOCOL_UTILS