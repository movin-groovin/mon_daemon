
// gcc -lstdc++ -std=c++11 -lpthread daemonize.cpp daemon.cpp -o daemon

#include "daemon.hpp"


GLOBAL_CHILDS g_chldData;
char *goodString;// = "xxxDEAD-BEAFxxx";



std::string StrError (int errCode) {
	const int strLen = 1024 + 1;
	std::string tmp (strLen, ' ');
	
	return std::string (strerror_r (errCode, &tmp[0], strLen));
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
	int ret;
	
	
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
		return 1;
	}
	
	if (argc < 3) {
#ifndef NDEBUG
		syslog (LOG_ERR, "Need first and second parameter where is a path to config file\n");
#endif
		return 2;
	}
	goodString = argv[2];
	
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
		syslog (LOG_ERR, "Error of pthread_sigmask at CTaskManager::StartWork: %s; errno: %d\n",
				StrError (ret).c_str (), ret);
		throw CTaskException ("Error of pthread_sigmask at CTaskManager::StartWork");
	}
	
	if (0 != (ret = pthread_create (&m_dat.thrSigHnd, NULL, threadSigHandler, this))) {
#ifndef NDEBUG
		syslog (LOG_ERR, "Error of pthread_create: %s; errno: %d\n", StrError (ret).c_str (), ret);
#endif
		throw CTaskException ("Can't create a thread for signals handling");
	}
	pthread_detach (m_dat.thrSigHnd);
	
	
	while (true) {
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
			FinAllThreadsAndChilds (g_chldData);
			g_chldData.Clear ();
			pthread_kill (m_dat.thrSigHnd, SIGTERM);
			usleep (secSleep * 1000 * 1000);
			if (!pthread_kill (m_dat.thrSigHnd, 0))
				pthread_kill (m_dat.thrSigHnd, SIGKILL);
			m_dat.needFin = false;
#ifndef NDEBUG
			syslog (LOG_WARNING, "Finit ala comedia\n");
#endif
			pthread_mutex_unlock (&m_dat.syncMut);
			break;
		} else if (m_dat.needReread)
		{
			FinAllThreadsAndChilds (g_chldData);
			g_chldData.Clear ();
			m_dat.needReread = false;
#ifndef NDEBUG
			syslog (LOG_WARNING, "Begining of reariding\n");
#endif
		} else
		{
#ifndef NDEBUG
			syslog (LOG_WARNING, "Unexcpected event\n");
#endif
		}
		
		pthread_mutex_unlock (&m_dat.syncMut);
	}
	
	
	return;
}


void CTaskManager::FinAllThreadsAndChilds (GLOBAL_CHILDS & chld) {
	for (auto & val : m_dat.pthArr)
		if (!pthread_kill (val, 0)) pthread_cancel (val);
		
	usleep (secSleep * 1000 * 1000);
	
	for (auto &val : m_dat.pthArr) {
		if (!pthread_kill (val, 0)) {
			pthread_kill (val, SIGKILL);
			kill (chld.GetPid (val), SIGKILL);
		}
	}
	
	return;
}


void* threadSigHandler (void *pvPtr) {
	CTaskManager *ctPtr = static_cast <CTaskManager *> (pvPtr);
	sigset_t sigSet;
	int ret, gotSig, chldStatus;
	pid_t pidNum;
	
	
	sigemptyset (&sigSet);
	sigaddset (&sigSet, SIGHUP);
	sigaddset (&sigSet, SIGTERM);
	sigaddset (&sigSet, SIGCHLD);
	while (true) {
		if (0 != (ret = sigwait (&sigSet, &gotSig))) {
#ifndef NDEBUG
		syslog (LOG_WARNING, "Error of sigwait in thread sig-handler: %s; errno: %d\n",
				StrError (ret).c_str (), ret);
#endif
			exit (101);
		}
		
		switch (gotSig) {
			case SIGHUP:
				pthread_mutex_lock (&ctPtr->m_dat.syncMut);
				ctPtr->m_dat.needReread = true;
				pthread_mutex_unlock (&ctPtr->m_dat.syncMut);
				pthread_cond_signal (&ctPtr->m_dat.syncCond);
				break;
			//
			case SIGTERM:
				pthread_mutex_lock (&ctPtr->m_dat.syncMut);
				ctPtr->m_dat.needFin = true;
				pthread_mutex_unlock (&ctPtr->m_dat.syncMut);
				pthread_cond_signal (&ctPtr->m_dat.syncCond);
				break;
			//
			case SIGCHLD:
				pidNum = waitpid (-1, &chldStatus, 0);
#ifndef NDEBUG
			syslog (LOG_WARNING, "Child process with pid: %d is dead\n", pidNum);
#endif
				break;
			//
			default:
#ifndef NDEBUG
			syslog (LOG_WARNING, "Have got unexcpected signal: %d\n", gotSig);
#endif
		}
	}
	
	
	return NULL;
}


bool IsAlive (const std::string & prPid) {
	DIR *dPtr;
	int ret, tmp = 0, fd;
	const int cmdLine = 2048 + 1;
	std::unique_ptr <char []> arrMem (new char [cmdLine]);
	

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


std::string MakeChildThread (const std::string & binPath) {
	std::string pidStr;
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
		syslog (LOG_WARNING, "Error of fork: %s; errno: %d\n",
				StrError (ret).c_str (), ret);
#endif
		return std::string ();
	} else if (pidVal == 0) {
		if (std::string::npos == (posInd = binPath.find_last_of ('/')))
		{
			argvArr [0] = const_cast <char*> (&binPath[0]);
		} else
		{
			posInd++;
			argvArr [0] = const_cast <char*> (&binPath[posInd]);
		}
		if ((ret = execv (std::string::npos == posInd ? argvArr[0] : binPath.c_str (),
						  argvArr)) == -1
		   )
		{
#ifndef NDEBUG
			ret = errno;
			syslog (LOG_WARNING, "Error of execv: %s; errno: %d\n",
					StrError (ret).c_str (), ret);
#endif
			pthread_exit ((void*)0x1);
			return std::string ();
		}
	}
	std::ostringstream ossCnv;
	ossCnv << pidVal;
	
	
	return ossCnv.str ();
}


void *workerThread (void *pvPtr) {
	std::string binPath = *static_cast <const std::string*> (pvPtr);
	const int secSlp = 2;
	int oldSt, newSt = PTHREAD_CANCEL_ENABLE;
	std::string prPid;
	std::istringstream issCnv;
	pid_t pidNum;
	
	
	pthread_setcancelstate (newSt, &oldSt);
	
	prPid = MakeChildThread (binPath);
	issCnv.str (prPid);
	issCnv >> pidNum;
	g_chldData.Insert (pthread_self (), pidNum);
	while (true) {
#ifndef NDEBUG
		syslog (LOG_WARNING, "Pid: %s - pid: %d\n", prPid.c_str (), pidNum);
#endif
		if (!IsAlive (prPid)) {
			prPid = MakeChildThread (binPath);
			issCnv.str (prPid);
			issCnv >> pidNum;
			g_chldData.Insert (pthread_self (), pidNum);
		}
#ifndef NDEBUG
		else 
			syslog (LOG_WARNING, "PProcess alive; id: %s - pid: %d\n", prPid.c_str (), pidNum);
#endif
		
		sleep (secSlp);
		pthread_testcancel ();
	} 
	
	
	return NULL;
}










