#ifndef NETWORK_H
#define NETWORK_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <windows.h>

#define close closesocket
//#define usleep(x) Sleep((x)/1000)
#define DIR_SEPARATOR "\\"
#pragma comment(lib, "ws2_32.lib")


#elif defined(__linux__)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/select.h>

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not available on your platform

#endif

static void initNetwork(void)
{
#ifdef WIN32
    WSADATA wsa;
    int err = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (err < 0)
    {
        printf("WSAStartup failed !\n");
        exit(EXIT_FAILURE);
    }
#endif
}

static void cleanNetwork(void)
{
#ifdef WIN32
    WSACleanup();
#endif
}
#endif
