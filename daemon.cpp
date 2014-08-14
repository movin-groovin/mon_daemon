
// gcc -lstdc++ -std=c++11 -lpthread daemonize.cpp daemon.cpp -g -o daemon_cpp
// 
// gcc -lstdc++ -std=c++11 -lpthread daemonize.cpp daemon.cpp -DNDEBUG -o daemon_cpp

#include "daemon.hpp"



char *goodString;// = "xxxDEAD-BEAFxxx";
const char *chPidFile = "/home/yarr/src/srv/mon_daemon/xxxDEAD-BEAFxxx.pid";
//"/var/log/xxxDEAD-BEAFxxx.pid";



std::string StrError (int errCode) {
	const int strLen = 1024 + 1;
	std::string tmp (strLen, ' ');
	
	return std::string (strerror_r (errCode, &tmp[0], strLen));
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


//
// 1 param is a name of config file, where present pathes to binaries
// to monitoring them (like: /bin/cat). 0 param of course is the program's name
// Second parameter (2) - is our special string like "xxxDEAD-BEAFxxx"
//
// Honestly mutex here is excess, I will delete it in the future
//
int main (int argc, char *argv[]) {
	const char *defName = "monitoring";
	struct sigaction sigAct;
	int ret, fd;
	std::string strPid;
	size_t curLen, totLen = 0;
	
	
	if (argc < 3) {
#ifndef NDEBUG
		syslog (LOG_ERR, "Need first and second parameter where is a path to config file\n");
#endif
		return 1;
	}
	goodString = argv[2];
	
	
	if ((ret = daemonize (defName)) != 0) {
		return ret;
	}
	sigAct.sa_handler = SIG_DFL;
	sigAct.sa_flags = 0;
	sigemptyset (&sigAct.sa_mask);
	if (-1 == (ret = sigaction (SIGHUP, &sigAct, NULL))) {
		ret = errno;
#ifndef NDEBUG
		syslog (LOG_ERR, "Error of sigaction: %s; errno: %d\n", StrError (ret).c_str (), ret);
#endif
		return 3;
	}
	
	
	if (-1 == (ret = IsAlreadyRunning (goodString, chPidFile))) return 2;
	if (ret) {
#ifndef NDEBUG
		syslog (LOG_ERR, "One instance of this program is running\n");
#endif
		return 0;
	}
	
	// to create pid file
	if (-1 == (fd = open (chPidFile, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR))) {
#ifndef NDEBUG
		ret = errno;
		syslog (LOG_ERR, "Error of open file: %s; description: %s; errno: %d\n",
				chPidFile, StrError (ret).c_str (), ret);
#endif
		return 4;
	}
	std::ostringstream ossCnv;
	ossCnv << getpid ();
	strPid = ossCnv.str ();
	while ((curLen = write (fd, &strPid[0] + totLen, strPid.size () - totLen))) {
		if (curLen == -1 && errno == EINTR) continue;
		if (curLen == -1 && errno != EINTR) {
#ifndef NDEBUG
			ret = errno;
			syslog (LOG_ERR, "Error of writing to file: %s; description: %s; errno: %d\n",
					chPidFile, StrError (ret).c_str (), ret);
#endif
			return 5;
		}
		
		if (strPid.size () == totLen) break;
		totLen += curLen;
	}
	close (fd);
#ifndef NDEBUG
	syslog (LOG_ERR, "Have written, pid: %s - %d\n", strPid.c_str (), getpid ());
#endif
	
	
	try {
		CTaskManager ctMgr (argv[1]);
		
		ctMgr.StartWork ();
		
		ret = 0;
	} catch (CTaskException & Exc) {
#ifndef NDEBUG
		syslog (LOG_ERR, (std::string ("Have caught an exception: ") + Exc.WhatError ()).c_str ());
#endif
		ret = 3;
	} catch (std::exception & Exc) {
#ifndef NDEBUG
		syslog (LOG_ERR, (std::string ("Have caught an exception: ") + Exc.what ()).c_str ());
#endif
		ret = 4;
	}
	
	
	return ret;
}


void CTaskManager::StartWork () {
	int ret;
	sigset_t sigMask;
	
	
	sigfillset (&sigMask);
	if (0 != (ret = pthread_sigmask (SIG_SETMASK, &sigMask, NULL))) {
#ifndef NDEBUG
		syslog (LOG_ERR, "Error of pthread_sigmask at CTaskManager::StartWork: %s; errno: %d\n",
				StrError (ret).c_str (), ret);
#endif
		throw CTaskException ("Error of pthread_sigmask at CTaskManager::StartWork");
	}
	
	
	while (true) {
		if (0 != (ret = pthread_create (&m_dat.thrSigHnd, NULL, threadSigHandler, this))) {
#ifndef NDEBUG
			syslog (LOG_ERR, "Error of pthread_create: %s; errno: %d\n", StrError (ret).c_str (), ret);
#endif
			throw CTaskException ("Can't create a thread for signals handling");
		}
		pthread_detach (m_dat.thrSigHnd);
		
		
		std::ifstream ifsDat (m_pathConf);
		if (!ifsDat) {
#ifndef NDEBUG
			ret = errno;
			syslog (LOG_ERR, "Can't open conf file: %s; Description: %s; errno: %d\n", 
					m_pathConf.c_str (), StrError (ret).c_str (), ret);
#endif
			throw CTaskException (("Can't open conf file" + m_pathConf).c_str ());
		}
		
		m_pathArr.clear ();
		std::string strBuf;
		while (ifsDat) {
			std::getline (ifsDat, strBuf);
			if (strBuf[0] == '#' || strBuf.length () == 0) {
				strBuf.clear ();
				continue;
			}
			m_pathArr.push_back (strBuf);
			strBuf.clear ();
		}
		if (m_pathArr.empty ()) {
#ifndef NDEBUG
			syslog (LOG_ERR, "Config file: \"%s\" is empty\n", m_pathConf.c_str ());
#endif
			throw CTaskException ("Config file: \"" + m_pathConf + "\" is empty\n");
		}
		
		std::vector <std::string>::iterator itWork = m_pathArr.begin ();
		auto itEnd = m_pathArr.end ();
		while (itWork != itEnd) {
			pthread_t thrId;
			if ((ret = pthread_create (&thrId, NULL, workerThread, &(*itWork))) != 0) {
#ifndef NDEBUG
				syslog (LOG_ERR, "Error of pthread_create: %s; errno: %d\n", StrError (ret).c_str (), ret);
#endif
			}
			m_dat.pthArr.push_back (thrId);
			
			++itWork;
		}
		
		//
		// Wait and handle info from signals
		//
		pthread_mutex_lock (&m_dat.syncMut);
		pthread_cond_wait (&m_dat.syncCond, &m_dat.syncMut);
		
		if (m_dat.needFin)
		{
			pthread_kill (m_dat.thrSigHnd, SIGTERM);
			FinAllWorkerThreads ();
			//usleep (secSleep * 1000 * 1000);
			//if (!pthread_kill (m_dat.thrSigHnd, 0))
			//	pthread_kill (m_dat.thrSigHnd, SIGKILL);
			m_dat.needFin = false;
#ifndef NDEBUG
			syslog (LOG_WARNING, "Finit ala comedia\n");
#endif
			pthread_mutex_unlock (&m_dat.syncMut);
			break;
		} else // if (m_dat.needReread)
		{
			pthread_kill (m_dat.thrSigHnd, SIGTERM);
			FinAllWorkerThreads ();
			//usleep (secSleep * 1000 * 1000);
			//if (!pthread_kill (m_dat.thrSigHnd, 0))
			//	pthread_kill (m_dat.thrSigHnd, SIGKILL);
			m_dat.needReread = false;
#ifndef NDEBUG
			syslog (LOG_WARNING, "Begining of reariding\n");
#endif
			pthread_mutex_unlock (&m_dat.syncMut);
		}
	}
	
	
	return;
}


void CTaskManager::FinAllWorkerThreads () {
	std::vector <pid_t> thrChlds;
	size_t shadPtr;
	
	
	for (auto & val : m_dat.pthArr)
		if (!pthread_kill (val, 0)) pthread_kill (val, SIGUSR1);
		
	usleep (secSleep * 1000 * 1000);
	
	for (auto &val : m_dat.pthArr) {
		if (!pthread_kill (val, 0))
			pthread_kill (val, SIGKILL);
		if (!pthread_join (val, reinterpret_cast <void**> (&shadPtr))) {
			thrChlds.push_back (static_cast <pid_t> (shadPtr));
		}
	}
	m_dat.pthArr.clear ();
	
	for (auto & pid : thrChlds) {
#ifndef NDEBUG
		syslog (LOG_WARNING, "Waiting for process with pid: %d\n", pid);
#endif
		waitpid (pid, NULL, 0);
	}
	
	
	return;
}


void* threadSigHandler (void *pvPtr) {
	CTaskManager *ctPtr = static_cast <CTaskManager *> (pvPtr);
	sigset_t sigSet;
	int ret, gotSig;
	
	
	sigemptyset (&sigSet);
	sigaddset (&sigSet, SIGHUP);
	sigaddset (&sigSet, SIGTERM);
	while (true) {
		if (0 != (ret = sigwait (&sigSet, &gotSig))) {
#ifndef NDEBUG
		syslog (LOG_WARNING, "Error of sigwait in thread sig-handler: %s; errno: %d\n",
				StrError (ret).c_str (), ret);
#endif
			//exit (101);
			usleep ((CTaskManager::secSleep * 1000  *1000) / 2);
			continue;
		}
		
		switch (gotSig) {
			case SIGHUP:
#ifndef NDEBUG
				syslog (LOG_WARNING, "Have got SIGHUP\n");
#endif
				pthread_mutex_lock (&ctPtr->m_dat.syncMut);
				ctPtr->m_dat.needReread = true;
				pthread_mutex_unlock (&ctPtr->m_dat.syncMut);
				pthread_cond_signal (&ctPtr->m_dat.syncCond);
				pthread_exit (NULL);
				break;
			//
			case SIGTERM:
#ifndef NDEBUG
				syslog (LOG_WARNING, "Have got SIGTERM\n");
#endif
				pthread_mutex_lock (&ctPtr->m_dat.syncMut);
				ctPtr->m_dat.needFin = true;
				pthread_mutex_unlock (&ctPtr->m_dat.syncMut);
				pthread_cond_signal (&ctPtr->m_dat.syncCond);
				pthread_exit (NULL);
				break;
			//
			default:
#ifndef NDEBUG
			syslog (LOG_WARNING, "Have got unexcpected signal: %d\n", gotSig);
#endif
			;
		}
	}
	
	
	return NULL;
}


bool IsAlive (pid_t pidNum) {
	DIR *dPtr;
	int ret, tmp = 0, fd;
	const int cmdLine = 2048 + 1;
	std::unique_ptr <char []> arrMem (new char [cmdLine]);
	std::string prPid;
	std::ostringstream ossCnv;
	
	
	ossCnv << pidNum;
	prPid = ossCnv.str ();

	if (NULL == (dPtr = opendir (("/proc/" + prPid).c_str ()))) {
#ifndef NDEBUG
		ret = errno;
		syslog (LOG_WARNING, "Can't find need process; pid: %s; error description: %s; errno: %d\n",
				prPid.c_str (), StrError (ret).c_str (), ret);
#endif
		return false;
	}
	
	if (-1 == (fd = open (("/proc/" + prPid + "/cmdline").c_str (), O_RDONLY))) {
#ifndef NDEBUG
		ret = errno;
		syslog (LOG_WARNING, "Error of opening: %s; error description: %s; errno: %d\n",
				("/proc/" + prPid + "/cmdline").c_str (), StrError (ret).c_str (), ret);
#endif
		return false;
	}
	
	while (ret = read (fd, &arrMem[0] + tmp, cmdLine - 1 - tmp)) {
		if (ret == -1) {
			if (errno != EINTR) {
#ifndef NDEBUG
				ret = errno;
				syslog (LOG_WARNING, "Error of reading from: %s; error description: %s; errno: %d\n",
						("/proc/" + prPid + "/cmdline").c_str (), StrError (ret).c_str (), ret);
#endif
				break;
			}
			continue;
		}
		
		if (tmp == cmdLine - 1) break;
		tmp += ret;
	}
	arrMem [tmp] = '\0';
	close (fd);
	
	for (int i = 0; i < tmp; ++i)
		if (arrMem[i] == '\0') arrMem[i] = '_';

#ifndef NDEBUG	
	syslog (LOG_WARNING, "Check of process: %s", strstr (arrMem.get (), goodString) ? "TRUE" : "FALSE");
#endif

	return strstr (arrMem.get (), goodString);
}


pid_t MakeChildThread (const std::string & binPath) {
	std::string pidStr;
	sigset_t sigMsk;
	pid_t pidVal;
	int ret;
	char *argvArr [] = {NULL, goodString, NULL};
	typename std::string::size_type posInd;
	

#ifndef NDEBUG
	syslog (LOG_INFO, "Bin path: %s\n", binPath.c_str ());
#endif
	if ((pidVal = fork ()) == -1) {
#ifndef NDEBUG
		ret = errno;
		syslog (LOG_ERR, "Error of fork: %s; errno: %d\n",
				StrError (ret).c_str (), ret);
#endif
		return -1;
	} else if (pidVal == 0) {
		//
		// This thread cretaed by main thread with full filled mask (all signals are blocked)
		// So in forked process we will unblock all signals, restoring signal's mask
		//
		sigemptyset (&sigMsk);
		if (0 != (ret = pthread_sigmask (SIG_SETMASK, &sigMsk, NULL))) {
#ifndef NDEBUG
			syslog (LOG_ERR, "Error of pthread_sigmask : %s; errno: %d in: %s at line: of function: %s\n",
					StrError (ret).c_str (), ret, __FILE__, __LINE__, __func__);
#endif
			return -1;
		}
		
		if (std::string::npos == (posInd = binPath.find_last_of ('/')))
		{
			argvArr [0] = const_cast <char*> (&binPath[0]);
		} else
		{
			posInd++;
			argvArr [0] = const_cast <char*> (&binPath[posInd]);
		}
		// if there are slashes - the path is absolute, otherwise is relative
		if ((ret = execv (std::string::npos == posInd ? argvArr[0] : binPath.c_str (),
						  argvArr)) == -1
		   )
		{
#ifndef NDEBUG
			ret = errno;
			syslog (LOG_ERR, "Error of execv: %s; errno: %d\n",
					StrError (ret).c_str (), ret);
#endif
			pthread_exit ((void*)0x1);
			return -1;
		}
	}
	
	
	return pidVal;
}


void *workerThread (void *pvPtr) {
	std::string binPath = *static_cast <const std::string*> (pvPtr);
	std::string prPid;
	sigset_t sigMask;
	pid_t pidNum;
	int ret, sigNum, sigRetWait, stsVal;
	
	
	sigemptyset (&sigMask);
	sigaddset (&sigMask, SIGCHLD);
	sigaddset (&sigMask, SIGUSR1);
	
	pidNum = MakeChildThread (binPath);
	while (true) {
		if (!IsAlive (pidNum)) {
			usleep ((CTaskManager::secSleep * 1000 * 1000) / 2);
			pidNum = MakeChildThread (binPath);
			continue;
		}
#ifndef NDEBUG
		else 
			syslog (LOG_WARNING, "Process alive, pid: %d\n", pidNum);
#endif
		
		if (0 != (ret = sigwait (&sigMask, &sigNum))) {
		#ifndef NDEBUG
			syslog (LOG_ERR, "Error of sigwait: %s; errno: %d\n",
					StrError (ret).c_str (), ret);
		#endif
			// if we can't wait for signals we check zombie childs
			waitpid (pidNum, NULL, WNOHANG);
			continue;
		}
		
		switch (sigNum) {
			case SIGUSR1:
				kill (pidNum, SIGTERM);
				pthread_exit (reinterpret_cast <void*> (pidNum));
				break;
			//
			case SIGCHLD:
				while (true) {
					if ((sigRetWait = waitpid (-1, &stsVal, WNOHANG)) == -1 && errno == EINTR) continue;
					else break;
				}
#ifndef NDEBUG
				if (sigRetWait != -1 && sigRetWait &&
						(WIFEXITED (stsVal) ||
						 WIFSIGNALED (stsVal)
						)
				   )
				{
					syslog (LOG_WARNING, "Child process with pid has finished: %d\n", sigRetWait);
				}
				else if (sigRetWait == -1) {
					ret = errno;
					syslog (LOG_ERR, "Error of waitpid: %s; errno: %d\n", StrError (ret).c_str (), ret);
				}
#endif
				break;
			//
			default:
#ifndef NDEBUG
			syslog (LOG_ERR, "Unexcpected signal, number: %d\n", sigNum);
#endif
			;
		}
	} 
	
	
	return NULL;
}










