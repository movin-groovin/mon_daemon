
// gcc -lstdc++ -std=c++11 -lpthread daemonize.cpp daemon.cpp -o daemon

#include "daemon.hpp"


GLOBAL_CHILDS g_chldData;



std::string StrError (int errCode) {
	const int strLen = 1024 + 1;
	std::string tmp (strLen, ' ');
	
	strerror_r (errCode, &tmp[0], strLen);
	
	return tmp;
}


//
// 1 param is a name of config file, where present pathes to binaries
// to monitoring them (like: /bin/cat). 0 param of course is the program's name
//
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
	
	if (argc < 2) {
#ifndef NDEBUG
		syslog (LOG_ERR, "Need first parameter where is a path to config file\n");
#endif
		return 2;
	}
	
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
	
	if (0 != (ret = pthread_create (&m_dat.thrSigHnd, NULL, threadSigHandler, &m_dat))) {
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
			syslog (LOG_ERR, "Can't open conf file: %s; Description: %s; errno: %d\n", 
					m_pathConf.c_str (), StrError (ret).c_str (), ret);
#endif
			throw CTaskException (("Can't open conf file" + m_pathConf).c_str ());
		}
		
		m_pathArr.clear ();
		std::string strBuf;
		while (ifsDat) {
			std::getline (ifsDat, strBuf);
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
			if ((ret = pthread_create (&thrId, NULL, workerThread, NULL)) != 0) {
#ifndef NDEBUG
				syslog (LOG_ERR, "Error of pthread_create: %s; errno: %d\n", StrError (ret).c_str (), ret);
#endif
			}
			m_dat.pthArr.push_back (thrId);
			
			++itWork;
		}
		
		//
		// Wait and to handle info from signals
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
	sigset_t sigSet;
	int ret, gotSig;
	
	
	sigemptyset (&sigSet);
	sigaddset (&sigSet, SIGHUP);
	sigaddset (&sigSet, SIGTERM);
	if (0 != (ret = sigwait (&sigSet, &gotSig))) {
#ifndef NDEBUG
			syslog (LOG_WARNING, "Error of sigwait in thread sig-handler: %s; errno: %d\n",
					StrError (ret).c_str (), ret);
#endif
		exit (101);
	}
	
	switch (gotSig) {
		case SIGHUP:
			
			break;
		//
		case SIGTERM:
			
			break;
		//
		default:
			syslog (LOG_WARNING, "Have got unexcpected signal: %d\n", gotSig);
	}
	
	
	return NULL;
}


void *workerThread (void *pvPtr) {
	
	
	
	return NULL;
}










