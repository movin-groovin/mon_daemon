
#include "daemonize.hpp"



int daemonize (const char *chStr) {
#ifndef NDEBUG
	int errVal;
#endif
	int fd0, fd1, fd2;
	pid_t prPid;
	struct rlimit limDat;
	struct sigaction saDat;
	
#ifndef NDEBUG
	openlog (chStr, LOG_PID, 0);
#endif
	
	umask (0);
	
	if (-1 == getrlimit (RLIMIT_NOFILE, &limDat)) {
		limDat.rlim_max = 1024;
	}
	
	if (-1 == (prPid = fork ())) {
#ifndef NDEBUG
		errVal = errno;
		syslog (LOG_ERR, "Error of fork: %s, errno num: %d\n", strerror (errVal), errVal);
#endif
		return 1;
	} else if (prPid != 0) {
		exit (1);
	}
#ifndef NDEBUG
	closelog ();
	openlog (chStr, LOG_PID, 0);
#endif
	
	setsid ();
	saDat.sa_handler = SIG_IGN;
	saDat.sa_flags = 0;
	sigemptyset (&saDat.sa_mask);
	if (-1 ==  sigaction (SIGHUP, &saDat, NULL)) {
#ifndef NDEBUG
		errVal = errno;
		syslog (LOG_ERR, "Error of sigaction: %s, errno num: %d\n", strerror (errVal), errVal);
#endif
		return 2;
	}
	
	if (-1 == chdir ("/")) {
#ifndef NDEBUG
		errVal = errno;
		syslog (LOG_ERR, "Error of chdir: %s, errno num: %d\n", strerror (errVal), errVal);
#endif
		return 3;
	}
	
	for (int i = 0; i < limDat.rlim_max; ++i) {
		close (i);
	}
	
	fd0 = open ("/dev/null", O_RDWR);
	fd1 = dup (fd0);
	fd2 = dup (fd1);
	
	
	return 0;
}

















