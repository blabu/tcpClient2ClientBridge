// ComunicationService.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <iostream>
#include <deque>
#include <map>
#include <chrono>
#include <thread>
#include <ctime>
#include <functional>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/signals2.hpp>

#include "Logger/Loger.hpp"

#include "json.hpp"

Loger globalLog("GLOBAL");


struct ConnectionProperties {
	std::string Host;
	std::string Port;
	std::string apiKey;
	std::string deviceId;
};



//TcpClient - Реализует интерфейс базового клиента для TCP соединения
class TcpClient : public BaseClient {
	static unsigned int receiveDataTimeout;
	const ConnectionProperties connection; // Настройки соодинения (нужны для переподключения)
	std::chrono::seconds connectionTimeout;// Таймоут соединения
	std::chrono::seconds reConnectionTimeout;
	boost::asio::ip::tcp::socket sock;
	boost::asio::deadline_timer watchdog;
	boost::asio::deadline_timer timer;
	boost::asio::ip::tcp::resolver resolver;

	/*Обработчик таймоута (сессия закончена)*/
	void timeoutHandler(const boost::system::error_code& er) {
		if (!er) {
			globalLog.addLog(Loger::L_WARNING, "Timeout session");
			boost::system::error_code er;
			sock.cancel(er);
			if (!er) { globalLog.addLog(Loger::L_WARNING, "Error in timeout handler when try cancel all operation"); }
			if (sock.is_open()) { boost::system::error_code er; sock.close(er); }
		}
		else if(er != boost::asio::error::operation_aborted) {
			globalLog.addLog(Loger::L_ERROR, er.message(), " Error in timeout handler");
		}
	}

	void connectionHandler(boost::system::error_code er, boost::asio::ip::tcp::endpoint p) {
		if (!er) { // Подключение удалось
			globalLog.addLog(Loger::L_TRACE, "Connection fine to the remote host");
			updateTimer(watchdog, boost::bind(&TcpClient::timeoutHandler, this, _1), connectionTimeout);
			message_ptr msg(new message((connection.apiKey + connection.deviceId).size(),
				(std::uint8_t*)(connection.apiKey + connection.deviceId).c_str()));
			globalLog.addLog(Loger::L_TRACE, "Form message ", (char*)msg->data());
			MessagesQueue.push_back(msg);
			write();
			read();
		}
		else { // Если ошибка подключения пробуем еще раз через reConnectionTimeout времени
			globalLog.addLog(Loger::L_ERROR, "Error when try open connection to the server host ", er.message());
			updateTimer( timer, [this](const boost::system::error_code& er) {connect();}, reConnectionTimeout);
		}
	}
	
	void connect() {
		auto entryPoints = resolver.resolve(connection.Host, connection.Port);
		boost::system::error_code er;
		sock.cancel(er);
		if (er) {
			globalLog.addLog(Loger::L_WARNING, er.message().c_str());
		}
		globalLog.addLog(Loger::L_INFO, "All function has canceled before create new connection");
		boost::asio::async_connect(sock, entryPoints, boost::bind(&TcpClient::connectionHandler, this, _1, _2));
		// Если подключение успешное отправим регистрационные данные
	}

	void readHandler(boost::system::error_code er, std::size_t sz) {
		if (!er) {
			if (sz) {
				updateTimer(readTimer,
					[=](const boost::system::error_code& er) {
					if (!er) {
						this->service->post(boost::bind(&TcpClient::readHandler, this, er, 0));
					}
				},
					readTimeout);
				updateTimer(watchdog, boost::bind(&TcpClient::timeoutHandler, this, _1), connectionTimeout);
				readBuffer += *(readBuffer.data() + readBuffer.size() - 1);
				read(); // Читаем сново, таким образом ждем байты мы все время пока не произойдет ошибка чтения
			}
			else { // Сюда мы попадем, по таймоуту (значит в течении readTimeout времени не пришел ни один байт)
				message_ptr m(new message(readBuffer)); // копируем принятое сообщение
				globalLog.addLog(Loger::L_TRACE, "Message from socket received ", (char*)readBuffer.data());
				readBuffer.clear(); // Старый буфер очищаем
				receiveNewData(m);  // Генерируем сигнал
				// При этом ожидание данных с сокета остается
			}
		}
		else { // Ошибка чтения приведет к завершению операции чтения
			if (sock.is_open()) { boost::system::error_code er; sock.close(er); } // TODO Возможно ошибка чтения приводит к завершению соединения
			globalLog.addLog(Loger::L_ERROR, "Error when try read from remote connection");
			globalLog.addLog(Loger::L_ERROR, er.message().c_str());
		}
	}

	void read() {
		boost::asio::async_read(sock, boost::asio::buffer((readBuffer.data()+readBuffer.size()-1),1),
			boost::bind(&TcpClient::readHandler, this, _1, _2));
	}

	void write() {
		if (!MessagesQueue.empty()) {
			updateTimer(watchdog, boost::bind(&TcpClient::timeoutHandler, this, _1), connectionTimeout);
			message_ptr newMsg(MessagesQueue.back());
			boost::asio::async_write(sock, boost::asio::buffer(newMsg->data(), newMsg->currentSize()),
				[this](boost::system::error_code er, std::size_t length) {
				if (!er) {
					if (length == MessagesQueue.back()->currentSize()) { MessagesQueue.pop_back(); }
					if (MessagesQueue.size() > 0) { write(); }
				}
				else { // Пробуем переподключится
					globalLog.addLog(Loger::L_ERROR, "Error when try write to the server");
					connect();
				}
			});
		}
	}

	TcpClient() = delete;
	TcpClient(TcpClient&) = delete;
public:
	TcpClient(boost::asio::io_service *const srv, const ConnectionProperties& c) :
		BaseClient(srv, 4096, std::chrono::milliseconds(receiveDataTimeout)),
		connection(c),
		connectionTimeout(std::chrono::seconds(90)),
		reConnectionTimeout(std::chrono::seconds(30)),
		sock(*service),
		watchdog(*service),
		timer(*service),
		resolver(*service) {
			globalLog.addLog(Loger::L_INFO, "Try open connection");
		}
	void open() {
		connect();
	}
	
	void close() noexcept {
		if (sock.is_open()) { boost::system::error_code er; sock.cancel(er); sock.close(er); }
	}

	virtual ~TcpClient() { 
		globalLog.addLog(Loger::L_INFO, "Delete TCP client");
		globalLog.snapShot();
	}
	static void setReadTiemout(unsigned int timeout) {
		receiveDataTimeout = timeout;
	}
};
unsigned int TcpClient::receiveDataTimeout;

class SerialClient : public BaseClient {
	static unsigned int receiveDataTimeout;
	boost::asio::serial_port sp;
	const std::string deviceName;
	void readHandler(const boost::system::error_code& er, std::size_t byteTransfered) {
		if (!er) {
			if (byteTransfered) {
				updateTimer(readTimer,
					[=](const boost::system::error_code& er) {
					if (!er) {
						this->service->post(boost::bind(&SerialClient::readHandler, this, er, 0));
					}
				},
				readTimeout);
				readBuffer += *(readBuffer.data() + readBuffer.size() - 1);
				read();
				globalLog.addLog(Loger::L_TRACE, "Received message for now ", (char*)readBuffer.data());
			}
			else { // byteTransfered == 0 Значит сюда мы попали по таймоуту все сообщение передано
				message_ptr receivedMsg(new message(readBuffer));
				globalLog.addLog(Loger::L_TRACE, "Result message ", (char*)readBuffer.data());
				receiveNewData(receivedMsg);
				readBuffer.clear();
			}
		}
		else {
			globalLog.addLog(Loger::L_ERROR, "Error when try read from serial port");
			//TODO обработка ошибок
		}
	}

	void read() {
		globalLog.addLog(Loger::L_TRACE, "Try read some messages from serial port");
		sp.async_read_some(boost::asio::buffer((readBuffer.data() + readBuffer.size() - 1), 1),
			boost::bind(&SerialClient::readHandler, this, _1, _2));
	}

	void write() {
		if (!MessagesQueue.empty()) {
			message_ptr newMsg(MessagesQueue.back());
			sp.async_write_some(boost::asio::buffer(newMsg->data(), newMsg->currentSize()),
				[this](const boost::system::error_code& er, std::size_t length) {
				if (!er) {
					if (length == MessagesQueue.back()->currentSize()) { MessagesQueue.pop_back(); }
					if (!MessagesQueue.empty()) { write(); }
				}
				else {
					globalLog.addLog(Loger::L_ERROR, "Error when try write to serial port");
					// TODO Обработка ошибки
				}
			});
		}
	}
public:
	SerialClient(boost::asio::io_service* const srv, const std::string& device): 
		BaseClient(srv, 4096, std::chrono::milliseconds(receiveDataTimeout)),
		sp(*srv),
		deviceName(device){
	
	}

	void close() noexcept {
		if (sp.is_open()) { boost::system::error_code er; sp.cancel(er); sp.close(er); }
	}

	void open() {
		boost::system::error_code er;
		sp.open(deviceName, er);
		if (!er) {
			sp.set_option(boost::asio::serial_port_base::baud_rate(921600));
			read();
		}
		else {
			globalLog.addLog(Loger::L_ERROR, "Error when try open COM port ", std::string(deviceName));
		}
	}

	static void setReadTiemout(unsigned int timeout) {
		receiveDataTimeout = timeout;
	}

	virtual ~SerialClient() { close(); finishSession(); service->stop(); }
};
unsigned int SerialClient::receiveDataTimeout;

#include <fstream>
/*
Модем как обертка над другим интерфейсом
*/

class ModemClient: public IBaseClient {
	static std::string clientKey;
	static std::map<std::string, std::string> vocabulary;
	static std::string host;
	static std::string port;
	
	boost::asio::io_service* const srv;
	std::atomic<bool> isStarted;
	std::atomic<bool> isFirstMessage;
	
	std::shared_ptr<IBaseClient> clientDelegate;
	
	ModemClient() = delete;
	ModemClient(const ModemClient&) = delete;
	
	void emmitNewDataFromDelegate(const message_ptr m) {
		receiveNewData(m);
	}

	bool stopCommandHandler(const std::string& command) {
		isStarted.store(false);
		if (clientDelegate != nullptr && clientDelegate.get() != nullptr) {
			clientDelegate->receiveNewData.disconnect_all_slots();
			clientDelegate->finishSession.disconnect_all_slots();
			globalLog.addLog(Loger::L_INFO, "Close connection, delete client delegate");
			clientDelegate->close();
			srv->post([this]() {if(this->clientDelegate != nullptr) this->clientDelegate.reset(); });
		}
		finishSession();
		return true;
	}
	
	bool startCommandHandler(const std::string& command) {
		globalLog.addLog(Loger::L_TRACE, "Finded command is start");
		if (!isStarted.load()) {
			auto identifier = command.find('#');// TODO Подключаемся
			if (identifier != std::string::npos) {
				std::string device;
				while (command[++identifier] != '#') { device.push_back(command[identifier]); }
				ConnectionProperties c;
				c.Host = "localhost";
				c.Port = "7778";
				c.apiKey = "$V2000EID=" + clientKey + ";";
				c.deviceId = device;
				isStarted.store(true);
				isFirstMessage.store(true);
				clientDelegate = std::shared_ptr<IBaseClient>(new TcpClient(srv, c));
				clientDelegate->open();
				clientDelegate->receiveNewData.connect(boost::bind(&ModemClient::emmitNewDataFromDelegate, this, _1));
				return true;
			}
			return false;
		}
		return false;
	}

	std::string write(const std::string& command, const std::string& defaultAnswer = "OK\r\n") {
		globalLog.addLog(Loger::L_INFO, "Modem receive command ", command);
		try {
			auto answer(vocabulary.at(command)); // Ищем ответ по словарю
			auto startPos = answer.find('{');	 // Если в ответе присутствуют фигурные скобки, значит это не ответ а команда к действию
			auto stopPos = answer.find('}'); // 
			if (startPos != std::string::npos && stopPos != std::string::npos && startPos < stopPos) { // Значит это команда на исполнение, а не готовый ответ
				if (answer.find("start") != std::string::npos) {
					globalLog.addLog(Loger::L_INFO, "Finded start command");
					if (startCommandHandler(command)) return defaultAnswer;
					else return std::string();
				}
				else if (answer.find("stop") != std::string::npos) {
					globalLog.addLog(Loger::L_INFO, "Finded stop command");
					if (stopCommandHandler(command)) return defaultAnswer;
					else return std::string();
				}
				// Undefined command
				globalLog.addLog(Loger::L_WARNING, "Undefined answer " + answer, " for command: " + command, ", so modem form empty string answer");
				return std::string();
			}
			// Здесь мы если ответ был найден в словаре
			if (!isStarted.load()) {
				globalLog.addLog(Loger::L_INFO, "Modem form answer ", answer);
				answer.append("\r\n");
				return answer;
			}
			else {
				return std::string();
			}
		}
		catch (std::out_of_range) {
			if (isStarted.load()) {
				globalLog.addLog(Loger::L_INFO, "Command not found and session is started, so modem form empty string answer");
				return std::string();
			}
			globalLog.addLog(Loger::L_INFO, "Command not found, so modem form default answer ", defaultAnswer);
			return defaultAnswer;
		}
	}

public:
	/*
	В качестве параметра используется адрес файла со словарем
	словарь представляет из себя масив json объектов с минимуму двумя параметрами command и answer
	*/
	ModemClient(boost::asio::io_service* const service, const std::string& addrForWorlds) : srv(service){}
	
	void open() {}
	void close() noexcept {}

	void sendNewData(const message_ptr& msg) {   // Слот
		auto answer = write(std::string((char*)msg->data()));
		if (!isStarted.load()) {
			globalLog.addLog(Loger::L_TRACE, "TCP client is stoped form answer to serial: ", answer);
			receiveNewData(message_ptr(new message(answer)));
		}
		else if (!isFirstMessage.load()) {
				if (clientDelegate != nullptr && clientDelegate.get() != nullptr) {
					globalLog.addLog(Loger::L_TRACE, "Try send new message to delegate");
					clientDelegate->sendNewData(msg);
				}
				else {
					globalLog.addLog(Loger::L_ERROR, "Client delegete is null");
				}
		}
		else {
			emmitNewDataFromDelegate(message_ptr(new message(answer)));
			isFirstMessage.store(false);
		}
	}

	static void setHost(const std::string& h) { host = h; }
	static void setPort(const std::string& p) { port = p; }
	static void setClientKey(const std::string& key) { clientKey = key; }
	static void appendNewCommand(const std::string& command, const std::string& answer) {
		vocabulary.insert(std::pair<std::string, std::string>(command, answer));
	}
};
std::string ModemClient::clientKey;
std::map<std::string, std::string> ModemClient::vocabulary;
std::string ModemClient::host;
std::string ModemClient::port;


class CommunictionService {
	boost::asio::io_service* const srv;
	std::shared_ptr<ModemClient> modem;
	std::shared_ptr<SerialClient> serial;
	nlohmann::json configuration;
	void readConfigurationFile(const std::string& configFile) {
		globalLog.addLog(Loger::L_TRACE, "Try open AT command map file in json format: ", configFile);
		std::ifstream file(configFile);
		if (file.is_open()) { globalLog.addLog(Loger::L_TRACE, "File ", configFile, " opened"); }
		globalLog.addLog(Loger::L_TRACE, "Try parse json file");
		try {
			file >> configuration;
			file.close();
		}
		catch (...) {
			globalLog.addLog(Loger::L_ERROR, "Unhandled ecxeption whe try parse file ", configFile);
			globalLog.snapShotLong();
			std::this_thread::sleep_for(std::chrono::seconds(2));
			exit(1);
		}
	}
public:
	CommunictionService(boost::asio::io_service* s, const std::string& atCommandFile) : srv(s) {
		readConfigurationFile(atCommandFile);
		Loger::setShowLevel(configuration.at("log").at("showLogLevel").get<unsigned int>());
		Loger::setSaveLevel(configuration.at("log").at("saveLogLevel").get<unsigned int>());
		ModemClient::setHost(configuration.at("server").at("host").get<std::string>());
		ModemClient::setPort(configuration.at("server").at("port").get<std::string>());
		ModemClient::setClientKey(configuration.at("server").at("clientKey").get<std::string>());
		auto commands = configuration.at("commands");
		for (auto& el : commands.items()) {
			globalLog.addLog(Loger::L_DEBUG, "Key " + el.key(), " Value " + el.value().get<std::string>());
			ModemClient::appendNewCommand(el.key(), el.value().get<std::string>());
		}
		SerialClient::setReadTiemout(configuration.at("serial").at("timeout").get<unsigned int>());
		TcpClient::setReadTiemout(configuration.at("server").at("timeout").get<unsigned int>());
		std::string portName(configuration.at("serial").at("portName").get<std::string>());

		modem = std::shared_ptr<ModemClient>(new ModemClient(srv, atCommandFile));
		
		serial = std::shared_ptr<SerialClient>(new SerialClient(srv, portName));
		serial->open();
		modem->open();
		globalLog.addLog(Loger::L_TRACE, "Setup default behavior");
		
		modem->receiveNewData.connect(boost::bind(&SerialClient::sendNewData, serial.get(), _1));
		serial->receiveNewData.connect(boost::bind(&ModemClient::sendNewData, modem.get(), _1));
	}

	void run() {srv->run();}

	~CommunictionService() { modem->close(); serial->close(); }
};


int main() {
	globalLog.SetAddr("./");
	globalLog.setShowLevel(3);

	boost::asio::io_service mainService;

	CommunictionService MainApp(&mainService, "./configuration.json");
	MainApp.run();
	Loger::snapShotLong();
    return 0;
}