#ifndef IBASE_CLIENT
#define IBASE_CLIENT

#include "messagesDTO.hpp"
#include <boost/signals2.hpp>

/* 
IBaseClient - Базовый интерфейс клиента в системе. 
Определяет набор boost сигналов и методов (слотов) необходимых для работы программы
И взаимодействия клиентов между собой
Все клиенты приложения реализуют этот интерфейс.
*/
class IBaseClient {
public:
	virtual void sendNewData(const message_ptr& msg) = 0;   // Слот который занимается отправкой данных полученных в процессе работы программы
	virtual void close() noexcept = 0;
	virtual void open() = 0;
	boost::signals2::signal<void(const message_ptr)> receiveNewData; // Сигнал об новых пришедших данных
	boost::signals2::signal<void(void)> finishSession;				 // Сигнал о завершении сессии
	virtual ~IBaseClient() { receiveNewData.disconnect_all_slots(); finishSession.disconnect_all_slots(); }
};


#endif //IBASE_CLIENT