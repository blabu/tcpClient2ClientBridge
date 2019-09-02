#include <boost/bind.hpp>
#include <regex>
#include "ModemClient.hpp"
#include "../MainProjectLoger.hpp"

std::map<std::string, std::string> ModemClient::vocabulary;
std::string ModemClient::host;
std::string ModemClient::port;

bool ModemClient::stopCommandHandler(const std::string & command) {
	isStarted.store(false);
	if (clientDelegate != nullptr && clientDelegate.get() != nullptr) {
		clientDelegate->receiveNewData.disconnect_all_slots();
		clientDelegate->finishSession.disconnect_all_slots();
		globalLog.addLog(Loger::L_INFO, "Close connection, delete client delegate");
		clientDelegate->close();
		srv->post([this]() {if (this->clientDelegate != nullptr) this->clientDelegate.reset(); });
	}
	finishSession();
	return true;
}

bool ModemClient::startCommandHandler(const std::string & command) {
	globalLog.addLog(Loger::L_TRACE, "Finded command is start");
	if (!isStarted.load()) {
		auto identifier = command.find('+');// TODO Подключаемся
		if (identifier == std::string::npos) { // Не нашли
			globalLog.addLog(Loger::L_TRACE, "Not find start device number (symb #) in command ", command, ". Try find + ");
			identifier = command.find('#');
		}
		if (identifier != std::string::npos) {
			globalLog.addLog(Loger::L_TRACE, "Start device number (symb # or +) finded in command ", command, " in position " + std::to_string(identifier));
			std::string device;
			try {
				const std::size_t sz = command.length();
				for (++identifier; identifier < sz; ++identifier) {
					if (command.at(identifier) != '\r' &&
						command.at(identifier) != '\n' &&
						command.at(identifier) != '#'  &&
						command.at(identifier) != '+') {
						device.push_back(command[identifier]);
						continue;
					}
					break;
				}
			}
			catch (std::out_of_range) {
				globalLog.addLog(Loger::L_ERROR, "Error! Not find end of phone number");
				return false;
			}
			isStarted.store(true);
			isFirstMessage.store(true);
			clientDelegate = std::shared_ptr<IBaseClient>(new ProtocolDecorator(srv, host, port, device));
			clientDelegate->open();
			clientDelegate->receiveNewData.connect(boost::bind(&ModemClient::emmitNewDataFromDelegate, this, _1));
			return true;
		}
		return false;
	}
	return false;
}

std::string ModemClient::write(const std::string & command, const std::string & defaultAnswer) {
	globalLog.addLog(Loger::L_INFO, "Modem receive command ", command);
	try {
		auto answer(vocabulary.at(command)); // Ищем ответ по словарю
		auto startPos = command.find('{');	 // Если в ответе присутствуют фигурные скобки, значит это не ответ а команда к действию
		auto stopPos = command.find('}'); // 
		if (startPos != std::string::npos && stopPos != std::string::npos && startPos < stopPos) { // Значит это команда на исполнение, а не готовый ответ
			if (command.find("start") != std::string::npos) { // {start}
				globalLog.addLog(Loger::L_INFO, "Finded start command");
				if (startCommandHandler(command)) return defaultAnswer;
				else return std::string();
			}
			else if (command.find("stop") != std::string::npos) {
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
	catch (std::out_of_range) { // Не найдена команда
        // Если команда не найдена в словаре, и сессия не запущена пробуем сравнить наш запрос со стартом сессии или её стопом
		try {
			if (!isStarted.load()) {
				globalLog.addLog(Loger::L_TRACE, "Try find regular expresion for command ", command + " ", vocabulary.at("{start}"));
				const std::regex regularExpressionStart(vocabulary.at("{start}"));
				if (std::regex_match(command, regularExpressionStart)) { // Найдено совпадение (достаем номер устройства для связи)
					globalLog.addLog(Loger::L_TRACE, "Regular expresion finded ");
					if (startCommandHandler(command)) return defaultAnswer;
					else return std::string();
				}
			}
			globalLog.addLog(Loger::L_TRACE, "Try find regular expresion for command ", command + " ", vocabulary.at("{stop}"));
			const std::regex regularExpressionStart(vocabulary.at("{stop}"));
			if (std::regex_match(command, regularExpressionStart)) { // Найдено совпадение (достаем номер устройства для связи)
				globalLog.addLog(Loger::L_TRACE, "Regular expresion finded ");
				if (stopCommandHandler(command)) return defaultAnswer;
				else return std::string();
			}
		}
		catch (std::out_of_range) { // Не определены такие команды как {start} и {stop}
			return "ERROR. Please define {start} and {stop} command";
		}
		if (isStarted.load()) {
			globalLog.addLog(Loger::L_INFO, "Command not found and session is started, so modem form empty string answer");
			return std::string();
		}
		globalLog.addLog(Loger::L_INFO, "Command not found, so modem form default answer ", defaultAnswer);
		return defaultAnswer;
	}
}

void ModemClient::sendNewData(const message_ptr & msg) {   // Пришли новые данные с последовательного порта
	auto answer = write(msg->toString()); // Проверяем команда ли это?
	if (!isStarted.load()) { // Если модуль не запущен
		globalLog.addLog(Loger::L_TRACE, "TCP client is stoped and form answer to serial: ", answer);
		receiveNewData(message_ptr(new message(answer))); // Формируем ответ сгенерированный имитатором модема
	}
	else if (!isFirstMessage.load()) { // Если это не первое сообщение
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
