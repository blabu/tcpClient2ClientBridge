#include "BinaryProtocolDecorator.hpp"
#include "ProtocolUtils.hpp"
#include <boost/format.hpp>
#include <string>

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

const std::string BinaryProtocolDecorator::headerTemplateBinary("$V1;%s;%s;%x;B;%x###"); // localName, toName, command, messageType (T-text,B-binary), messageSize, message

std::vector<std::uint8_t> BinaryProtocolDecorator::formMessage(const std::string & to, command cmd, std::size_t sz, const std::uint8_t * data) const {
	globalLog.addLog(Loger::L_TRACE, "BinaryProtocolDecorator::formMessage to ", to);
	auto header = (boost::format(headerTemplateBinary)%localName%to%cmd%sz).str();
	std::vector<std::uint8_t> res(header.cbegin(), header.cend());
	for (std::size_t i = 0; i < sz; i++) res.push_back(data[i]);
	globalLog.addLog(Loger::L_TRACE, "BinaryProtocolDecorator::formMessage fine");
	return res;
}

std::string BinaryProtocolDecorator::formMessage(const std::string & to, command cmd, const std::string & data) const {
	return (boost::format(headerTemplateBinary)%localName%to%cmd%data.size()).str().append(data);
}

void BinaryProtocolDecorator::parseMessage(const message_ptr m){
	header head;
	try {
		head = ProtocolUtils::parseHeader(m);
	}
	catch (std::invalid_argument er) {
		globalLog.addLog(Loger::L_ERROR, er.what());
		return;
	}
	if (!checkHeader(head)) return;
	globalLog.addLog(Loger::L_DEBUG, "Receive binary message size ", std::to_string(head.packetSize));
	globalLog.addLog(Loger::L_INFO, "Receive binary message header ", m->toString().substr(0, head.headerSize));
	receiveNewData(message_ptr(new message(head.packetSize, m->data()+head.headerSize)));
}

