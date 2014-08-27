
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


int IsAlreadyRunning (
	const std::string & cmdLinePar,
	const std::string & pidFile
)
//Ret values: 0 - not running; 1 - is running; -1 - an error have happened
{
	std::string strTmp;
	int fd, ret;
	size_t rdLen = 0, curLen;
	int bufLen = 128;
	std::vector <char> chBuf (bufLen + 1);
	
	
	if (-1 == (fd = open (pidFile.c_str (), O_RDWR))) {
#ifndef NDEBUG
		ret = errno;
		syslog (LOG_ERR, "Error of open file: %s; description: %s; errno: %d\n",
				pidFile.c_str (), StrError (ret).c_str (), ret);
#endif
		return 0;
	}
	while ((curLen = read (fd, &chBuf[0] + rdLen, bufLen - rdLen)) != 0) {
		if (curLen == -1 && errno == EINTR) continue;
		if (curLen == -1 && errno != EINTR) {
#ifndef NDEBUG
			ret = errno;
			syslog (LOG_ERR, "Error of read file: %s; description: %s; errno: %d\n",
					pidFile.c_str (), StrError (ret).c_str (), ret);
#endif
			close (fd);
			return -1;
		}
		
		if (rdLen == bufLen) break;
		rdLen += curLen;
	}
	close (fd);
	chBuf[rdLen] = '\0';
	
	
	strTmp = "/proc/" + std::string (&chBuf[0]) + "/cmdline";
	if (-1 == (fd = open (strTmp.c_str (), O_RDONLY))) {
#ifndef NDEBUG
		ret = errno;
		syslog (LOG_ERR, "Error of open file: %s; description: %s; errno: %d\n",
				strTmp.c_str (), StrError (ret).c_str (), ret);
#endif
		return ENOENT == errno ? 0 : -1;
	}
	bufLen *= 10;
	chBuf.resize (bufLen + 1);
	while ((curLen = read (fd, &chBuf[0] + rdLen, bufLen - rdLen)) != 0) {
		if (curLen == -1 && errno == EINTR) continue;
		if (curLen == -1 && errno != EINTR) {
#ifndef NDEBUG
			ret = errno;
			syslog (LOG_ERR, "Error of read file: %s; description: %s; errno: %d\n",
					pidFile.c_str (), StrError (ret).c_str (), ret);
#endif
			close (fd);
			return -1;
		}
		
		if (rdLen == bufLen) break;
		rdLen += curLen;
	}
	close (fd);
	chBuf[rdLen] = '\0';
	for (int i = 0; i < rdLen; ++i) if (chBuf[i] == '\0') chBuf[i] = '_';
	
	
	return std::string::npos == std::string (&chBuf[0]).find (cmdLinePar) ? 0 : 1;
}














