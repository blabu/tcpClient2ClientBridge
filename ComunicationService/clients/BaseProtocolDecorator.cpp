#include "BaseProtocolDecorator.hpp"
#include "../MainProjectLoger.hpp"
#include "../Configuration.hpp"

#include "../picosha2.h"

#include <boost/beast/core/detail/base64.hpp>
#include <boost/scoped_array.hpp>
#include <boost/format.hpp>

#include <cstdlib>
#include <chrono>

const std::string BaseProtocolDecorator::initOkMessage("$V1;0;%s;6;7###INIT OK"); // local name
const std::string BaseProtocolDecorator::connectOkMessage("$V1;%s;%s;8;a###CONNECT OK"); // to name, local name

/*
https://www.e-reading.club/chapter.php/1002058/30/Mayers_-_Effektivnoe_ispolzovanie_CPP.html

Правило 9: Никогда не вызывайте виртуальные функции в конструкторе или деструкторе 

Начну с повторения: вы не должны вызывать виртуальные функции во время работы конструкторов или деструкторов, 
потому что эти вызовы будут делать не то, что вы думаете, и результатами их работы вы будете недовольны. 
Если вы – программист на Java или C#, то обратите на это правило особое внимание, потому что это в этом отношении C++ ведет себя иначе. 
Предположим, что имеется иерархия классов для моделирования биржевых транзакций, то есть поручений на покупку, на продажу и т. д.
Важно, чтобы эти транзакции было легко проверить, поэтому каждый раз, когда создается новый объект транзакции, 
в протокол аудита должна вноситься соответствующая запись. Следующий подход к решению данной проблемы выглядит разумным:

class Transaction { // базовый класс для всех 
public: // транзакций Transaction(); 
virtual void logTransaction() const = 0; // выполняет зависящую от типа // запись в протокол

... 

}; 

Transaction::Transaction() // реализация конструктора 
{ 
// базового класса 
... 
logTransaction(); 
} 

class BuyTransaction: public Transaction { // производный класс 
public: 

virtual void logTransaction() const = 0; // как протоколировать 
// транзакции данного типа 
... 
}; 

class SellTransaction: public Transaction { 
// производный класс public: 
virtual void logTransaction() const = 0; 
// как протоколировать 
// транзакции данного типа 

... 

};

Посмотрим, что произойдет при исполнении следующего кода: BuyTransaction b; 
Ясно, что будет вызван конструктор BuyTransaction, но сначала должен быть вызван конструктор Transaction,
потому что части объекта, принадлежащие базовому классу, конструируются прежде, чем части, принадлежащие 
производному классу. В последней строке конструктора Transaction вызывается виртуальная функция logTransaction, 
тут-то и начинаются сюрпризы. Здесь вызывается та версия logTransaction, которая определена в классе Transaction, 
а не в BuyTransaction, несмотря на то что тип создаваемого объекта – BuyTransaction.
Во время конструирования базового класса не вызываются виртуальные функции, определенные в производном классе. 
Объект ведет себя так, как будто он принадлежит базовому типу. Короче говоря, во время конструирования базового класса виртуальных функций не существует.
Есть веская причина для столь, казалось бы, неожиданного поведения. Поскольку конструкторы базовых классов вызываются раньше, чем конструкторы производных, 
то данные-члены производного класса еще не инициализированы во время работы конструктора базового класса. 
Это может стать причиной неопределенного поведения и близкого знакомства с отладчиком. 
Обращение к тем частям объекта, которые еще не были инициализированы, опасно, поэтому C++ не дает такой возможности. 
Есть даже более фундаментальные причины. Пока над созданием объекта производного класса трудится конструктор базового класса, типом объекта является базовый класс. 
Не только виртуальные функции считают его таковым, но и все прочие механизмы языка, использующие информацию о типе во время исполнения 
(например, описанный в правиле 27 оператор dynamic_cast и оператор typeid). 
В нашем примере, пока работает конструктор Transaction, инициализируя базовую часть объекта BuyTransaction, этот объект относится к типу Transaction. 
Именно так его воспринимают все части C++, и в этом есть смысл: части объекта, относящиеся к BuyTransaction, 
еще не инициализированы, поэтому безопаснее считать, что их не существует вовсе. 
Объект не является объектом производного класса до тех пор, пока не начнется исполнение конструктора последнего. 
То же относится и к деструкторам. Как только начинает исполнение деструктор производного класса, предполагается, что данные-члены, 
принадлежащие этому классу, не определены, поэтому C++ считает, что их больше не существует. 
При входе в деструктор базового класса наш объект становится объектом базового класса,
и все части C++ – виртуальные функции, оператор dynamic_cast и т. п. – воспринимают его именно так. 
*/

BaseProtocolDecorator::BaseProtocolDecorator(boost::asio::io_service *const srv, const std::string& host, const std::string& port, 
										   const std::string& deviceID, const std::string& answerIfOK, const std::string& answerIfError) : 
	answerOK(answerIfOK),
	answerError(answerIfError),
	srv(srv), 
	host(host), 
	port(port), 
	connectTo(deviceID) {
	auto conf = Configuration::getConfiguration("");
	localName = conf->getConfigString("user:name");
	localPass = localName + conf->getConfigString("user:pass");
	globalLog.addLog(Loger::L_DEBUG, "User name is ", localName);
	isConnected = false;
	// В конструкторе НИКОГДА НЕ ИСПОЛЬЗУЙ ВИРТУАЛЬНЫЕ ФУНКЦИИ (смотри комментарий выше)
}

//check INIT OK answer initOkMessage
void BaseProtocolDecorator::initHandler(const message_ptr m){
	globalLog.addLog(Loger::L_TRACE, "In init handler");
	std::string resultMessage(m->toString());
	std::string waitMessage((boost::format(initOkMessage) % localName).str());
	globalLog.addLog(Loger::L_TRACE, "Try compare receive message ", resultMessage, " and wait message ", waitMessage);
	if (resultMessage == waitMessage) { // Подключение и инициализация удалась
		globalLog.addLog(Loger::L_TRACE, "Try connect to ", connectTo);
		clientDelegate->receiveNewData.disconnect_all_slots();
		clientDelegate->receiveNewData.connect(boost::bind(&BaseProtocolDecorator::connectToDeviceHandler, this, _1));
		clientDelegate->sendNewData(message_ptr(new message(formMessage(connectTo, messageTypes::connectCMD, ""))));
		globalLog.addLog(Loger::L_INFO, "Init OK");
	}
	else {
		globalLog.addLog(Loger::L_WARNING, "Message not equal");
		close(); 
		open();
	}
}

// Check CONNECT OK answer connectOkMessage
void BaseProtocolDecorator::connectToDeviceHandler(const message_ptr m) {
	globalLog.addLog(Loger::L_TRACE, "In connect handler");
	std::string resultMessage(m->toString());
	std::string waitMessage ((boost::format(connectOkMessage)%connectTo%localName).str());
	globalLog.addLog(Loger::L_TRACE, "Try compare receive message ", resultMessage, " and wait message ", waitMessage);
	if (resultMessage == waitMessage) { // Подключение к клиенту удалось
		isConnected = true;
		clientDelegate->receiveNewData.disconnect_all_slots();
		clientDelegate->receiveNewData.connect(boost::bind(&BaseProtocolDecorator::parseMessage, this, _1));
		globalLog.addLog(Loger::L_INFO, "Connect OK");
		receiveNewData(message_ptr(new message(answerOK)));
		if (savedMsg != nullptr && savedMsg.get() != nullptr) {
			sendNewData(savedMsg);
		}
	}
	else {
		globalLog.addLog(Loger::L_ERROR, "Connection can not be establish ", resultMessage);
		receiveNewData(message_ptr(new message(answerError)));
	}
}

bool BaseProtocolDecorator::checkHeader(const header& h) {
	if (h.to != localName) {
		globalLog.addLog(Loger::L_WARNING, "Error localName ", localName, " not equal ", h.to);
	}
	if (h.msgType != messageTypes::dataCMD) {
		if (h.msgType == messageTypes::closeCMD) {
			isConnected = false;
			globalLog.addLog(Loger::L_WARNING, "Remote host close connection");
			return false;
		}
		else {
			globalLog.addLog(Loger::L_ERROR, "Undefined message type");
			return false;
		}
	}
	return true;
}

void BaseProtocolDecorator::sendNewData(const message_ptr & msg) {
	if (clientDelegate != nullptr && clientDelegate.get() != nullptr) {
		if (isConnected) {
			clientDelegate->sendNewData(message_ptr(new message(formMessage(connectTo, messageTypes::dataCMD, msg->currentSize(), msg->data()))));
		}
		else {
			if (savedMsg != nullptr && savedMsg.get() != nullptr) {
				globalLog.addLog(Loger::L_ERROR, "Error try send new message ", msg->toString(), " but already have saved message ", savedMsg->toString());
			}
			else {
				globalLog.addLog(Loger::L_WARNING, "Try send message ", msg->toString(), " but not connected yet. Save it");
			}
			savedMsg = msg;
		}
	}
	else {
		globalLog.addLog(Loger::L_ERROR, "Internal client (tcp client) is null");
	}
}

void BaseProtocolDecorator::close() noexcept {
	if (clientDelegate != nullptr && clientDelegate.get() != nullptr) {
		clientDelegate->close();
	}
}

void BaseProtocolDecorator::open() {
	std::vector<unsigned char> hashBin(picosha2::k_digest_size);
	picosha2::hash256(localPass.cbegin(), localPass.cend(), hashBin.begin(), hashBin.end());
	localPass = ProtocolUtils::base64_encode(hashBin.data(), hashBin.size());
	std::string salt(ProtocolUtils::randomString(16));
	globalLog.addLog(Loger::L_DEBUG, "Random salt is ", salt);
	auto signature = localName + salt + localPass;
	picosha2::hash256(signature.cbegin(), signature.cend(), hashBin.begin(), hashBin.end());
	signature = salt + ";" + ProtocolUtils::base64_encode(hashBin.data(), hashBin.size());
	globalLog.addLog(Loger::L_DEBUG, "Signature is ", signature);
	ConnectionProperties c;
	c.connectionString = formMessage("0", messageTypes::initCMD, signature);
	c.Host = host; c.Port = port;
	globalLog.addLog(Loger::L_INFO, "Form init string ", c.connectionString);
	clientDelegate = std::shared_ptr<TcpClient>(new TcpClient(srv, c));
	clientDelegate->finishSession.connect([this]() { this->receiveNewData(message_ptr(new message(this->answerError))); this->finishSession(); });
	clientDelegate->open();
}

