
// g++ sleeper.cpp -o sleeper

#include <iostream>

#include <unistd.h>
#include <syslog.h>



int main () {
	syslog (LOG_NOTICE, "================ SLEEPER ================\n");
	
	while (true) pause ();
	
	return 0;
}
