#ifndef GRASS_NETWORKING_H
#define GRASS_NETWORKING_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <cstdio>
#include <algorithm>
#include<iostream>
#include <string.h>
#include <unistd.h>

#define SOCKET int

/*
 * Code inspired from
 * https://stackoverflow.com/questions/25634483/send-binary-file-over-tcp-ip-connection
 * https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/
 */
struct thread_args {
    int port;
    char ip[1024];
    char fileName[1024];
    long fileSize;
};

using namespace std;

void *openFileServer(void *ptr);

void *openFileClient(void *ptr);

bool sendData(SOCKET sock, void *buffer, int bufferLength);

bool sendFile(SOCKET sock, FILE *file);

bool readData(SOCKET sock, void *buffer, int bufferLength);

bool readFile(SOCKET sock, FILE *file, long fileSize);

#endif //GRASS_NETWORKING_H