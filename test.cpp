
// g++ -std=c++11 -o test daemonize.cpp test.cpp

#include "daemonize.hpp"



int main (int argc, char **argv) {
	sigset_t newMask, oldMask;
	int ret;
	
	
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
	
	
	return 0;
}























