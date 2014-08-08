
#include "daemonize.hpp"

#include <vector>
#include <exception>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <sstream>

#include <pthread.h>
#include <dirent.h>
#include <sys/wait.h>



std::string StrError (int errCode);
void* threadSigHandler (void *pvPtr);
void *workerThread (void *pvPtr);



class CProgramException: public std::exception {
public:
	const char* what () const noexcept {
		return "Have happened an error while initialization of SET_OF_PARAMS structure\n";
	}
};


class CTaskException: public std::exception {
private:
	std::string m_msg;
public:
	CTaskException (const std::string & str): m_msg (str) {}
	
	const char* what () const noexcept {
		return "Have happened an error in CTaskManager\n";
	}
	
	std::string WhatError () const noexcept {
		return "Have happened an error in CTaskManager: " + m_msg;
	}
};


class GLOBAL_CHILDS {
private:
	mutable pthread_mutex_t syncMut;
	std::unordered_map <pthread_t, pid_t> mapChild;
	
public:
	GLOBAL_CHILDS () {
		if (0 != pthread_mutex_init (&syncMut, NULL)) throw CProgramException ();
	}
	~ GLOBAL_CHILDS () {
		pthread_mutex_destroy (&syncMut);
	}
	
	
	void Insert (pthread_t thrId, pid_t childId) {
		pthread_mutex_lock (&syncMut);
		mapChild[thrId] = childId;
		pthread_mutex_unlock (&syncMut);
	}
	
	bool CheckPresence (pthread_t thrId) const {
		pthread_mutex_lock (&syncMut);
		bool ret = mapChild.end () == mapChild.find (thrId);
		pthread_mutex_unlock (&syncMut);
		return ret;
	}
	
	void Remove (pthread_t thrId) {
		pthread_mutex_lock (&syncMut);
		mapChild.erase (thrId);
		pthread_mutex_unlock (&syncMut);
	}
	
	pid_t GetPid (pthread_t thrId) {
		pthread_mutex_lock (&syncMut);
		pid_t pid = mapChild[thrId];
		pthread_mutex_unlock (&syncMut);
		return pid == 0 ? -1 : pid;
	}
	
	void Clear () {
		pthread_mutex_lock (&syncMut);
		mapChild.clear ();
		pthread_mutex_unlock (&syncMut);
	}
};


typedef struct _SET_OF_PARAMS {
	pthread_t thrSigHnd;
	
	pthread_mutex_t syncMut;
	pthread_cond_t syncCond;
	std::vector <pthread_t> pthArr;
	
	bool needFin, needReread;
	
	_SET_OF_PARAMS () throw (CProgramException):
		thrSigHnd (0), needFin (false), needReread (false) {
		if (pthread_mutex_init (&syncMut, NULL) != 0 ||
			pthread_cond_init (&syncCond, NULL)
		)
		{
			throw CProgramException ();
		}
	}
	
	~ _SET_OF_PARAMS () {
		pthread_mutex_destroy (&syncMut);
		pthread_cond_destroy (&syncCond);
	}
} SET_OF_PARAMS, *PSET_OF_PARAMS;


class CTaskManager {
private:
	static const int secSleep = 5;
	SET_OF_PARAMS m_dat;
	std::vector <std::string> m_pathArr;
	std::string m_pathConf;
	
public:
	CTaskManager (const std::string & pathPar): m_pathConf (pathPar) {}
	~ CTaskManager () {}
	
	void StartWork ();
	
private:
	void FinAllThreadsAndChilds (GLOBAL_CHILDS & chld);
	
	
	//
	friend void* threadSigHandler (void *pvPtr);
};








