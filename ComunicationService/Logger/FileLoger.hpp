#ifndef FILE_LOGER_H
#define FILE_LOGER_H

#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <fstream>
#include <vector>
#include "ILogerWriter.h"

class FileLoger: public ILogerWriter {
    typedef boost::shared_ptr<boost::thread> thPtr_t;
    FileLoger(const std::string &level, const std::string &symb)= delete;
    FileLoger(const FileLoger&)=delete;
    FileLoger& operator=(const FileLoger&)=delete;

    const std::string level;          // Уровень логов имя файла по умолчанию
    static std::string fileAddr;
    std::fstream file;                 // Файл слогами
    std::vector<std::string> *logData; // Контейнер логов

    void saveToDisk(const std::string& addr, std::vector<std::string>* listData);
	void saveToDisk(const std::string& addr);
    thPtr_t th_ptr;
    boost::mutex vectorMtx;               // Защищает контейнер логов
	boost::mutex fileMtx;			  // Защищает файл от открытия в двух потоках
public:
    static void SetLogFilesAddr(const std::string & addr){fileAddr=addr;}
    static std::string getFileAddrLog(){return fileAddr;}
    FileLoger(const std::string & level);
	FileLoger() = delete;
    virtual ~FileLoger();
	
    void addLog(const std::string&& text);
    void flush();
    void flushLong();
	void deleteAllLog();
	
	enum timePoint_t {
		timestamp = 1,
		seconds,
		minutes,
		hour,
		day
	};
	static std::string getTimeName(timePoint_t point);
};

#endif // FILE_LOGER_H
