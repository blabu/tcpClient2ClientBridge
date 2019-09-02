#ifndef MESSAGES_DTO
#define MESSAGES_DTO

#include <string>
#include <cstring> // for memcpy
#include <memory>

class message {
	const std::size_t sz;
	std::size_t current;
	std::uint8_t *const dat;
public:
	message(const std::size_t size) :sz(size), current(0), dat(new std::uint8_t[sz]) {}
	message(const message&& m) : sz(m.sz), current(m.current), dat(std::move(m.dat)) {}
	message(const message& m) : sz(m.current+(m.current>>1)), current(m.current), dat(new std::uint8_t[sz]) {
        memcpy((void*)dat, (const void*)m.dat, current);
	}
	message(const std::string& m) : sz(m.size()+(m.size()>>1)), current(m.size()), dat(new std::uint8_t[sz]) {
        memcpy(dat, m.c_str(), m.size());
	}
	message(const std::size_t size, std::uint8_t* const data) : sz(size+(size>>1)), current(size), dat(new std::uint8_t[sz]) {
        memcpy((void*)dat, (const void*)data, size);
	}
	~message() { if (dat != nullptr) delete[] dat; }
	std::uint8_t *const data() const { return dat; }
	const std::size_t size() const { return sz; }
	const std::size_t currentSize() const { return current; }
	void clear() { current = 0; }
	message& operator+=(std::uint8_t byte) {
		if (current >= sz) return *this;
		dat[current++] = byte;
		return *this;
	}
	void push_back(const std::uint8_t byte) {
		this->operator+=(byte);
	}
	void push_front(const std::uint8_t byte) {
		if ((current+1) >= sz) {
			return;
		}
		current++;
		for (std::size_t i = current; i > 0; i--) {
			dat[i] = dat[i-1];
		}
		dat[0] = byte;
	}
	std::string toString() const {
		return std::string((const char*const)dat, current);
	}
};

typedef std::shared_ptr<message> message_ptr;

#endif //MESSAGES_DTO
