
// g++ sleeper.cpp -o sleeper

#include <iostream>

#include <unistd.h>
#include <syslog.h>



int main () {
	syslog (LOG_NOTICE, "================ SLEEPER ================: %s - %d - %s\n",
			__FILE__, __LINE__, __PRETTY_FUNCTION__);
	
	while (true) pause ();
	
	return 0;
}
