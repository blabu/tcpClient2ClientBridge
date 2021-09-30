#ifndef PROTOCOL_UTILS
#define PROTOCOL_UTILS

#include "messagesDTO.hpp"


/*
	ErrorCOMMAND      uint16 = 1
	PingCOMMAND       uint16 = 2
	RegisterCOMMAND   uint16 = 3
	GenerateCOMMAND   uint16 = 4
	AuthCOMMAND       uint16 = 5
	DataCOMMAND       uint16 = 6
	SaveDataCOMMAND   uint16 = 7
	PropertiesCOMMAND uint16 = 8
*/
enum command {
	errCMD = 1,
	pingCMD = 2,
	initCMD = 5,
	dataCMD = 6,
	connectCMD = 9,
};

struct header {
	std::size_t headerSize;
	std::size_t packetSize;
	command msgType;
	std::string from;
	std::string to;
};

class ProtocolUtils
{
	static const std::string headerEnd;
	ProtocolUtils() = delete;
public:
	~ProtocolUtils() = default;
public:
	static std::string base64_encode(std::uint8_t const* data, std::size_t len);
	static std::string base64_encode(std::string const& s);
	static header parseHeader(const message_ptr m);
	static std::string randomString(std::size_t size);
};

#endif //PROTOCOL_UTILS