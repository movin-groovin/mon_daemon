
#include "daemonize.hpp"

#include <vector>
#include <exception>
#include <fstream>
#include <memory>

#include <pthread.h>



std::string StrError (int errCode);


class ProgramException: public std::exception {
public:
	const char* what () const noexcept {
		return "Have happened an error while initialization of SET_OF_PARAMS structure\n";
	}
};


class TaskException: public std::exception {
public:
	const char* what () const noexcept {
		return "Have happened an error in CTaskManager\n";
	}
	
	std::string WhatError () const noexcept {
		return XXXX;
	}
};


typedef struct _SET_OF_PARAMS {
	pthread_t thrSigHnd;
	
	pthread_mutex_t syncArr;
	std::vector <pthread_t> pthArr;
	
	pthread_spinlock_t syncNeedChk;
	bool needFin;
	
	_SET_OF_PARAMS (pthread_t thrVal) throw (ProgramException): thrSigHnd (thrVal) {
		if (pthread_mutex_init (&syncArr, NULL) != 0 ||
			pthread_spin_init (&syncNeedChk, 0) != 0
		)
		{
			throw ProgramException ();
		}
	}
	
	~ _SET_OF_PARAMS () {
		pthread_mutex_destroy (&syncArr);
		pthread_spin_destroy (&syncNeedChk);
	}
} SET_OF_PARAMS, *PSET_OF_PARAMS;


class CTaskManager {
private:
	SET_OF_PARAMS m_dat;
	std::vector <std::string> m_path;
	
public:
	CTaskManager (SET_OF_PARAMS par1, std::vector <std::string> par2):
			m_dat(par1), m_path(par2) {}
	~ CTaskManager () {}
	
	void StartWork ();
};








