#ifndef ILOGER_WRITER_H
#define ILOGER_WRITER_H

#include <string>

/*
������� ��������� ��� ������ ������
� �������
2 ��� 2019 ������ ��� ��������� ������ FileLoger
*/

class ILogerWriter {
public:
	virtual void addLog(const std::string&&) = 0;
	virtual void flush() = 0;
	virtual void flushLong() = 0;
	virtual void deleteAllLog() = 0;
	virtual ~ILogerWriter() {}
};

#endif ILOGER_WRITER_H