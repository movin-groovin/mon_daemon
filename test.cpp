
// g++ -std=c++11 -lpthread -o test daemonize.cpp test.cpp

#include "daemonize.hpp"

#include <iostream>

#include <pthread.h>



void* threadFunc (void *p) {
	std::cout << "Inside thread\n";
	//pause ();
	return (void*)0x1234;
}

class CPrint {
public:
	int print (int *val) {
		std::cout << "In file: " << __FILE__ << "; at line: " << __LINE__
			      << "; of function: " <<__PRETTY_FUNCTION__ <<std::endl;
		return 1;
	}
};


int main (int argc, char **argv) {
	sigset_t newMask, oldMask;
	int ret;
	void *pvPtr;
	
	/*
	if ((ret = daemonize ("Test programm")) != 0) {
		return ret;
	}
	
	sigfillset (&newMask);
	if (-1 == (ret = sigprocmask (SIG_SETMASK, &newMask, &oldMask))) {
#ifndef NDEBUG
		ret = errno;
		syslog (LOG_ERR, "Error of sigprocmask: %s, errno num: %d\n", strerror (ret), ret);
#endif
		return (5);
	}
	while (true) {
		syslog (LOG_INFO, "=============== INFO MESSAGE ===============\n");
		usleep (5 * 1000 * 1000);
	}
	*/
	CPrint ().print (NULL);
	
	pthread_t thrId;
	if (0 != pthread_create (&thrId, NULL, &threadFunc, NULL)) {
		std::cout << "Erro of pthread_create\n";
		return 1;
	}
	sleep (2);
	/*
	if (0 != pthread_detach (thrId)) {
		std::cout << "Error of pthread_detach\n";
		return 1;
	}
	*/
	sleep (2);
	std::cout << "Ret of pthread_kill: " << pthread_kill (thrId, 0) << std::endl;
	pthread_join (thrId, &pvPtr);
	std::cout << "Returned from thread: " << pvPtr << std::endl;
	
	std::cout << "Bye bye\n";
	
	
	return 0;
}























