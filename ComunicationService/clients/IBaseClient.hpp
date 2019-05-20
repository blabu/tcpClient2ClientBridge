#ifndef IBASE_CLIENT
#define IBASE_CLIENT

#include "messagesDTO.hpp"
#include <boost/signals2.hpp>

//IBaseClient - Базовая сущность клиента (все клиенты приложения реализуют этот интерфейс)
class IBaseClient {
public:
	virtual void sendNewData(const message_ptr& msg) = 0;   // Слот
	virtual void close() noexcept = 0;
	virtual void open() = 0;
	boost::signals2::signal<void(const message_ptr)> receiveNewData; // Сигнал что пришли новые данные
	boost::signals2::signal<void(void)> finishSession;
	virtual ~IBaseClient() { receiveNewData.disconnect_all_slots(); finishSession.disconnect_all_slots(); }
};


#endif //IBASE_CLIENT