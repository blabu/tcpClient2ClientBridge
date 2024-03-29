#include "MainProjectLoger.hpp"
#include "ComunicationService.hpp"
#include "Logger/Loger.hpp"

Loger globalLog("GLOBAL");

int main() {
	globalLog.SetAddr("./");
	globalLog.setShowLevel(3);

	boost::asio::io_service mainService;
	CommunicationService MainApp(&mainService, "./config.json");
	MainApp.run();
	Loger::snapShotLong();
	return 0;
}