
// gcc -lstdc++ -std=c++11 -lpthread daemonize.cpp daemon.cpp -o daemon

#include "daemon.hpp"



std::string StrError (int errCode) {
	const int strLen = 1024 + 1;
	std::string tmp (' ', strLen);
	
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
	std::vector <std::string> chldArr;
	int ret;
	
	
	if ((ret = daemonize (defName)) != 0) {
		return ret;
	}
	
	if (argc < 2) {
#ifndef NDEBUG
		syslog (LOG_ERR, "Need first parameter where is a path to config file\n");
#endif
		return 1;
	}
	
	
	std::ifstream ifsDat (argv[1]);
	std::string strBuf;
	while (ifsDat) {
		std::getline (ifsDat, strBuf);
		chldArr.push_back (strBuf);
		strBuf.clear ();
	}
	
	
	try {
		SET_OF_PARAMS parDat (thrHandler);
		CTaskManager ctMgr (parDat, chldArr);
		
		ctMgr.StartWork ();
		
		ret = 0;
	} catch (std::exception & Exc) {
		syslog (LOG_ERR, (std::string ("Have caught an exception: ") + Exc.what ()).c_str ());
		ret = 2;
	}
	
	
	return ret;
}


void CTaskManager::StartWork () {
	std::vector <std::string>::iterator itWork = m_path.begin ();
	auto itEnd = m_path.end ();
	int ret;
	
	
	if (0 != (ret = pthread_create (&m_dat.thrSigHnd, NULL, &signalHandler, &m_dat))) {
		syslog (LOG_ERR, "Error of pthread_create: %s; errno: %d\n", StrError (ret).c_str (), ret);
		throw std::exception ("Can't create a thread for signals handling");
	}
	pthread_detach (thrHandler);
	
	
	while (itWork != itEnd) {
		if ((ret = pthread_create ()) != 0) {
			syslog (LOG_ERR, "Error of pthread_create: %s; errno: %d\n", StrError (ret).c_str (), ret);
		}
		
		++itWork;
	}
	
	
	return;
}



















