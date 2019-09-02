#include "FileLoger.hpp"
#include <boost/chrono.hpp>
#include <iostream>

std::string FileLoger::fileAddr("./");

FileLoger::FileLoger(const std::string& level):level(level){
	logData = new(std::vector<std::string>);
	if (logData != nullptr) {
		std::cerr << "Create logger for " << level<<std::endl;
		logData->reserve(100);
		logData->push_back("Start loging " + level);
	}
}

FileLoger::~FileLoger(){
    flushLong();
    if(file.is_open()) file.close();
	std::cerr << "File logger " << level << " destroyed\n";
}

void FileLoger::addLog(const std::string&& text){
	if (logData != nullptr) {
		if (vectorMtx.try_lock()) {
			logData->push_back(text);
			vectorMtx.unlock();
		}
	}
}

void FileLoger::saveToDisk(const std::string &addr, std::vector<std::string>* listData){
	try {
		boost::mutex::scoped_lock lk(fileMtx);
		if (listData != nullptr) {
			while (file.is_open()) {
				std::cerr << "ERROR!!! File log already openned";
				boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
			}
			file.open(addr, std::ios_base::app);
			if (file.is_open()) {
				for (auto it : *listData) {
					file << it << "\n";
				}
				file.close();
			}
			listData->clear();
			delete listData; // Чистим память
		}
	}
	catch (...) {
		addLog("WARN! UNHANDELED EXCEPTION WHEN TRY SAVE LOG");
	}
}

void FileLoger::saveToDisk(const std::string &addr) {
	try {
		boost::mutex::scoped_lock lk(fileMtx);
		if (logData != nullptr) {
			while(file.is_open()) {
				std::cerr << "ERROR!!! File log already openned";
				boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
			}
			file.open(addr, std::ios_base::app);
			if (file.is_open()) {
				boost::mutex::scoped_lock scope(vectorMtx);
				for (auto it : *logData) {
					file << it << "\n";
				}
				file.close();
				logData->clear();
			}
		}
	}
	catch (...) {
		addLog("WARN! UNHANDELED EXCEPTION WHEN TRY SAVE LOG");
	}
}

void FileLoger::flush() {
	if (logData != nullptr) {
		if (fileAddr.empty()) return;
		if (logData->empty()) return;
		std::string tempAddr(fileAddr + level + " " + getTimeName(day) + ".txt");
		if (th_ptr != nullptr && th_ptr.get() != nullptr) th_ptr->join();
		std::size_t size = logData->size();
		{
			boost::mutex::scoped_lock scope(vectorMtx);
			auto deleteAfterSavePointer = logData;
			th_ptr = thPtr_t(new boost::thread(boost::bind(&FileLoger::saveToDisk, this, tempAddr, deleteAfterSavePointer)));
			logData = new(std::vector<std::string>);
			logData->reserve(size);
		}
	}
}

void FileLoger::flushLong(){
	if (logData != nullptr) {
		if (fileAddr.empty()) return;
		if (logData->empty()) return;
		std::string tempAddr(fileAddr + level + " " + getTimeName(day) + ".txt");
		saveToDisk(tempAddr);
	}
}

void FileLoger::deleteAllLog(){
	if (logData != nullptr) {
		boost::mutex::scoped_lock scope(vectorMtx);
		logData->clear();
	}
}


#ifdef _MSC_BUILD
#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS
#endif
std::string FileLoger::getTimeName(timePoint_t point) {
	static const std::string mon[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	std::time_t t = std::time(NULL);
	std::tm* timed = std::localtime(&t);
	std::string str_time = std::to_string(timed->tm_mday) + " " +
		mon[timed->tm_mon];
	if (point == timePoint_t::day) {
		return str_time;
	}
	str_time += " " + std::to_string(timed->tm_hour); 
	if (point == timePoint_t::hour) {
		return str_time;
	}
	str_time += ":" + std::to_string(timed->tm_min);
	if (point == timePoint_t::minutes) {
		return str_time;
	}
	str_time += ":" + std::to_string(timed->tm_sec);
	if (point == timePoint_t::seconds) {
		return str_time;
	}
	str_time += "." + std::to_string(boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::system_clock::now().time_since_epoch()).count());
	
	return str_time;
}
