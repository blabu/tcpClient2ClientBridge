#include "Loger.hpp"
#include "FileLoger.hpp"
#include <string>
#include <ctime>
#include <boost/bind.hpp>

#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS

#include <iostream>

const std::string Loger::level[]={
    "ERROR",
    "WARNING",
	"UNIQ",
    "INFO",
	"TRACE",
	"DEBUG",
    "EMPTY",
};

unsigned short Loger::saveLevel = L_DEBUG; //  По умолчанию все логи доступны для сохранения
unsigned short Loger::showLevel = L_DEBUG; //  По умолчанию все логи доступны для отображения

LogerWriter_ptr Loger::logerArray[] = { 
				LogerWriter_ptr(new FileLoger(Loger::level[Loger::L_ERROR])),
				LogerWriter_ptr(new FileLoger(Loger::level[Loger::L_WARNING])),
				LogerWriter_ptr(new FileLoger(Loger::level[Loger::L_UNIQ])),
				LogerWriter_ptr(new FileLoger(Loger::level[Loger::L_INFO])),
				LogerWriter_ptr(new FileLoger(Loger::level[Loger::L_TRACE])),
				LogerWriter_ptr(new FileLoger(Loger::level[Loger::L_DEBUG]))
};

void Loger::snapShot(uint8_t logLevel) {
	if (logLevel < EMPTY) {
		std::cout << "====---Save logs " << level[logLevel] << " level to disk---====\n";
		if (logLevel <= saveLevel) logerArray[logLevel]->flush();
		logerArray[logLevel]->deleteAllLog();
	}
}

void Loger::snapShot(){
	int i = 0;
	for (; i <= saveLevel; i++) {
		std::cout << "====---Save logs "<<level[i]<<" level to disk---====\n";
		std::flush(std::cout);

		try {
			logerArray[i]->flush();
		}
		catch (...) {
			logerArray[L_ERROR]->addLog("Unhandled exception when try save log data to disc");
			std::cerr << "\nERROR!!! Unhandled exception when try save log data to disc\n";
			return;
		}
	}
	for (; i < EMPTY; i++) {
		logerArray[i]->deleteAllLog();
	}
}

void Loger::snapShotLong(){
	int i = 0;
	for (; i <= saveLevel; i++) {
		try {
			if (logerArray[i] != nullptr && logerArray[i].get() != nullptr) {
				std::cout << "====---Save logs long " << level[i] << " level to disk---====\n";
				logerArray[i]->flushLong();
			}
		}
		catch (...) {
			if(logerArray[L_ERROR] != nullptr && logerArray[L_ERROR].get() != nullptr) logerArray[L_ERROR]->addLog("Unhandled exception when try save log data in long function to disc");
			std::cerr << "\nERROR!!! Unhandled exception when try save log data in long function to disc\n";
			return;
		}
	}
	for (; i < EMPTY; i++) {
		if (logerArray[i] != nullptr && logerArray[i].get() != nullptr) logerArray[i]->deleteAllLog();
	}
}

Loger::~Loger(){}

void Loger::setSaveLevel(unsigned short lvl) {
	if (lvl >= EMPTY) lvl = L_DEBUG;
	saveLevel = lvl;
}
void Loger::setShowLevel(unsigned short lvl) {
	if (lvl >= EMPTY) lvl = L_DEBUG;
	showLevel = lvl;
}

void Loger::setSaveLevel(const std::string& lvl) {
	unsigned short poz = 0;
	for (; poz < L_DEBUG; poz++) {
		if (lvl == level[poz]) break;
	}
	saveLevel = poz;
}
void Loger::setShowLevel(const std::string& lvl) {
	unsigned short poz = 0;
	for (; poz < L_DEBUG; poz++) {
		if (lvl == level[poz]) break;
	}
	showLevel = poz;
}

Loger::Loger(const std::string &tag, const std::string &symb): TAG(tag), symb(symb){
	bool isError = false;
	for (unsigned short i = 0; i < EMPTY; i++) {
		if (logerArray[i] == nullptr || logerArray[i].get() == nullptr) {
			isError = true;
		}
	}
	if (!isError) {
		std::string data = "Start loging " + tag;
		addLog(L_DEBUG, std::move(data));
	}
	else {
		std::cerr << "---======ERROR!!! Backend loggers for "<<tag<<" is null======---\n";
	}
}

/*
ФОРМАТ ЛОГОВ
"TIME"|"TAG"|"TEXT"
*/
void Loger::addLog(uint8_t logLevel, const std::string&& str) {
    if(logLevel >= EMPTY) return;
	if ((logLevel > saveLevel) &&
		(logLevel > showLevel)) return; // Нечего здесь делать
    std::string time = FileLoger::getTimeName(FileLoger::seconds);
    std::string data(symb+time+symb+"<"+TAG+">"+symb+str+symb+"\n");
	if (logLevel <= showLevel) {
		std::cout << data << std::endl;
	}
	logerArray[logLevel]->addLog(std::move(data));
}

void Loger::addLog(uint8_t logLevel, const std::string& str) {
	if (logLevel >= EMPTY) return;
	if ((logLevel > saveLevel) &&
		(logLevel > showLevel)) return; // Нечего здесь делать
	std::string time = FileLoger::getTimeName(FileLoger::seconds);
	std::string data(symb + time + symb + "<" + TAG + ">" + symb + str + symb + "\n");
	if (logLevel <= showLevel) {
		std::cout << data << std::endl;
	}
	logerArray[logLevel]->addLog(std::move(data));
}

void Loger::addLog(uint8_t logLevel, const std::string& str1, const std::string& str2) {
	if (logLevel >= EMPTY) return;
	if ((logLevel > saveLevel) &&
		(logLevel > showLevel)) return; // Нечего здесь делать
	std::string logStr(str1 + str2);
	addLog(logLevel, std::move(logStr));
}

void Loger::addLog(uint8_t logLevel, const std::string& str1, const std::string& str2, const std::string& str3) {
	if (logLevel >= EMPTY) return;
	if ((logLevel > saveLevel) &&
		(logLevel > showLevel)) return; // Нечего здесь делать
	std::string logStr(str1 + str2 + str3);
	addLog(logLevel, std::move(logStr));
}

void Loger::addLog(uint8_t logLevel, const std::string& str1, const std::string& str2, const std::string& str3, const std::string& str4) {
	if (logLevel >= EMPTY) return;
	if ((logLevel > saveLevel) &&
		(logLevel > showLevel)) return; // Нечего здесь делать
	std::string logStr(str1 + str2 + str3 + str4);
	addLog(logLevel, std::move(logStr));
}

void Loger::addLog(uint8_t logLevel, const char *ch){
    std::string str_mess(ch);
    return addLog(logLevel, std::move(str_mess));
}

std::string Loger::getStatus() const {
	std::string res(  "\nLogger information\nCurrent show level is: " + Loger::level[Loger::showLevel] + "\n" + 
					  "Current save level is: " + Loger::level[Loger::saveLevel] + "\n" + 
					  "Current fileAddr is: "   + FileLoger::getFileAddrLog() + "\n");
	return res;
}

Loger& Loger::operator<<(const std::string& str){
    addLog(L_DEBUG, std::move(str));
    return *this;
}

Loger& Loger::operator<<(const char *ch){
    addLog(L_DEBUG,ch);
    return *this;
}

