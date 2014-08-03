#ifndef _COMMON_H
#define _COMMON_H


#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>

#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>

using std::string;
using std::cout;
using std::endl;
using std::vector;
using std::map;
using std::pair;


#define MAX_BUF_SIZE 1024
#define DNS_BUF_SIZE 8192


#endif // common.h