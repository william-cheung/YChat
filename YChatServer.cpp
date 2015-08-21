#include "ServerManager.h"

using namespace YChat;
using namespace std;

// -----------------------------------------------------------------------

#define terminate()  kill(getpid(), SIGKILL)

void (*my_signal(int signo, void (*func)(int)))(int) {
	struct sigaction act, oact;
	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
/*
#ifdef SA_INTERRUPT
	act.sa_flags |= SA_INTERRUPT;
#else
	printf("there is no defined SA_INTERRUPT on your platform," 
			"which may cause undefined behaviour !\n");
#endif
*/
	if (sigaction(signo, &act, &oact) < 0) 
		return SIG_ERR;
	return oact.sa_handler;
}


// -----------------------------------------------------------------------

static ServerManager* pmanager = NULL;

void on_exit() {
	//printf("\nexiting ...\n");
	if (pmanager != NULL) pmanager->releaseResources();
	pmanager = NULL;
}

void sig_handler(int sig) {
	kill(getpid(), SIGUSR1);  // according to thread.h (& thread.cpp)
}

int main() {
	my_signal(SIGINT, sig_handler); 
	my_signal(SIGTERM, sig_handler);
	atexit(on_exit);

	Conf::GetInstance("server.conf");

	ServerManager::OnProtocolProcessListener listener;
	ControlProtocol::setOnProtocolProcessListener(&listener);

	ServerManager manager("Server");
	pmanager = &manager;
	Protocol::Server(&manager);
	
	ThreadPool::AddTask(PollIO::Task::GetInstance());

	ThreadPool::Run();

	return 0;
}
