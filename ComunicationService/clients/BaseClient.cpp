#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "BaseClient.hpp"
#include "../MainProjectLoger.hpp"


void BaseClient::updateTimer(boost::asio::deadline_timer & t, std::function<void(boost::system::error_code er)> handler, std::chrono::microseconds time) {
	boost::system::error_code er;
	t.expires_from_now(boost::posix_time::microseconds(time.count()), er);
	if (er.value() != 0) {
		globalLog.addLog(Loger::L_WARNING, "Can not update timer");
	}
	else {
		t.async_wait(handler); // Запускаем таймер
	}
}

BaseClient::BaseClient(boost::asio::io_service * const srv, std::size_t readBufferSize, std::chrono::milliseconds ReadTmeout) :
	IBaseClient(),
	service(srv),
	readTimer(*srv),
	readTimeout(ReadTmeout),
	readBuffer(readBufferSize) {
	if (readTimeout.count() == 0) readTimeout = std::chrono::milliseconds(15);
}

void BaseClient::sendNewData(const message_ptr & msg) {
	globalLog.addLog(Loger::L_TRACE, "SendNewData is called ", (char*)msg->data());
	if (MessagesQueue.empty()) {
		MessagesQueue.push_front(std::move(msg));
		write();
	}
	else {
		MessagesQueue.push_front(std::move(msg));
	}
}
