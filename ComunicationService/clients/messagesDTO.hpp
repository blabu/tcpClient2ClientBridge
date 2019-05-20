#ifndef MESSAGES_DTO
#define MESSAGES_DTO

#include <cstdio>
#include <string>
#include <memory>

class message {
	const std::size_t sz;
	std::size_t current;
	const bool isAllocate;
	std::uint8_t *const dat;
public:
	message(const std::size_t size) :isAllocate(true), sz(size), current(0), dat(new std::uint8_t[sz]) {}
	message(const message&& m) : isAllocate(false), sz(m.sz), current(m.current), dat(std::move(m.dat)) {}
	message(const message& m) : isAllocate(true), sz(m.current), current(m.current), dat(new std::uint8_t[sz]) {
		memcpy(dat, m.dat, current);
	}
	message(const std::string& m) : sz(m.size()), current(sz), dat(new std::uint8_t[sz]), isAllocate(true) {
		memcpy(dat, m.c_str(), sz);
	}
	message(const std::size_t size, std::uint8_t* data) : isAllocate(false), sz(size), current(sz), dat(data) {}
	~message() { if (dat != nullptr && isAllocate) delete[] dat; }
	std::uint8_t *const data()const { return dat; }
	const std::size_t size()const { return sz; }
	const std::size_t currentSize() const { return current; }
	void clear() { current = 0; }
	message& operator+=(std::uint8_t byte) {
		if (current >= sz) return *this;
		dat[current++] = byte;
		return *this;
	}
	void push_back(std::uint8_t byte) {
		this->operator+=(byte);
	}
};

typedef std::shared_ptr<message> message_ptr;

#endif //MESSAGES_DTO