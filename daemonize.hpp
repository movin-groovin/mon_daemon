
#ifndef DAEMON_HEADER
#define DAEMON_HEADER

//#define NDEBUG


#include <iostream>
#include <string>
#include <vector>

#include <cerrno>
#include <cstring>
#include <cstdlib>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <syslog.h>
#include <signal.h>



int daemonize (const char *chStr);
int IsAlreadyRunning (const std::string & cmdLinePar, const std::string & pidFile);



#endif // DAEMON_HEADER

