
// g++ -std=c++11 -o test1 test1.cpp

#include <iostream>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <string>

#include <unistd.h>
#include <signal.h>
#include <ucontext.h>



void printUContext (ucontext_t *pCnt) {
	printf ("uc_link: %p\n", (void*)pCnt->uc_link);
	printf ("Rip: %p\n", (void*)pCnt->uc_mcontext.gregs [REG_RIP]);
	printf ("Rsp: %p\n", (void*)pCnt->uc_mcontext.gregs [REG_RSP]);
	printf ("CS_FS_GS: %p\n", (void*)pCnt->uc_mcontext.gregs [REG_CSGSFS]);
	
	return;
}


void sigActionSIGHUP (int sigNum, siginfo_t *sigInfo, void *pvContext) {
	ucontext_t *cntPtr = (ucontext_t*)pvContext;
	
	std::cout << "SIGNAUM: " << sigNum << std::endl;
	std::cout << "Func ptr1: " << (void*)&sigActionSIGHUP << "\n\n";
	printUContext (cntPtr);
	printf ("\n");
	
	
	return;
}



int main () {
	sigset_t sigMsk;
	struct sigaction saDat;
	ucontext_t cntDat;
	
	
	sigemptyset (&saDat.sa_mask);
	saDat.sa_flags = SA_SIGINFO;
	saDat.sa_sigaction = &sigActionSIGHUP;
	if (-1 == sigaction (SIGHUP, &saDat, NULL)) {
		std::cout << "Error of sigaction\n";
		return 1;
	}
	
	std::cout << "Before\n";
	std::cout << "Func ptr2: " << (void*)&sigActionSIGHUP << std::endl;
	printUContext (&cntDat);
	printf ("\n");
	raise (SIGHUP);
	std::cout << "After\n";
	printUContext (&cntDat);
	printf ("\n");
	sleep (5);
	
	
	std::string ss = "12345";
	std::istringstream iss;
	iss.str (ss);
	ss [0] = '9';
	ss [1] = '9';
	int val;
	iss >> val;
	
	std::cout << "Result: " << val << std::endl;
	
	
	return 0;
}























