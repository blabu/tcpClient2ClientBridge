#ifndef LOGER_H
#define LOGER_H

#include <string>
#include "ILogerWriter.h"
#include "FileLoger.hpp"
#include "StatusInterface.hpp"

/*
ФОРМАТ ЛОГОВ
"TIME"|"TAG"|"TEXT"
*/

typedef std::shared_ptr<ILogerWriter> LogerWriter_ptr;

class Loger : public StatusInterface
{
public:
    enum { L_ERROR, L_WARNING, L_UNIQ, L_INFO, L_TRACE, L_DEBUG, EMPTY };
private:
    const  std::string TAG;             // Строка с тэгом логов
    const std::string symb;         // Символ (строка) разделитель
    static const std::string level[EMPTY+1];
    static LogerWriter_ptr logerArray[EMPTY];
    static unsigned short saveLevel;	// Уровень логов, ниже которого не сбрасываются данные на диск (принимает значения  enum{L_ERROR,L_WARNING,L_INFO,L_TRACE,EMPTY};)
    static unsigned short showLevel;   // Уровень логов, ниже которого не отображается информация
    void snapShot(uint8_t logLevel);
public:
    Loger(const std::string &tag, const std::string &symb="|*|");
    static void SetAddr(const std::string& addr) {FileLoger::SetLogFilesAddr(addr);}
    static std::string getLogFileAdress() {return FileLoger::getFileAddrLog();}
    static void setSaveLevel(unsigned short lvl);
    static void setShowLevel(unsigned short lvl);
    static void setSaveLevel(const std::string& lvl);
    static void setShowLevel(const std::string& lvl);
    static void snapShot();
    static void snapShotLong();
    ~Loger();
	void addLog(std::uint8_t logLevel, const std::string &str);
    void addLog(std::uint8_t logLevel, const std::string &&str);
    void addLog(std::uint8_t logLevel, const std::string &str1, const std::string &str2);
    void addLog(std::uint8_t logLevel, const std::string &str1, const std::string &str2, const std::string &str3);
	void addLog(uint8_t logLevel, const std::string& str1, const std::string& str2, const std::string& str3, const std::string& str4);
    void addLog(std::uint8_t logLevel, const char *ch);

	std::string getStatus()const;

    Loger &operator <<(const std::string &str); // Для DEBUG
    Loger &operator <<(const char *ch);		    // Для DEBUG
};

#endif // LOGER_H
