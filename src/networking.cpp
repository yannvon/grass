#include "networking.hpp"

#define SOCKET int
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/*
 * Code inspired from:
 * https://stackoverflow.com/questions/25634483/send-binary-file-over-tcp-ip-connection
 * https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/
 */

using namespace std;


/**
 * Creates a TCP server to serve a file. Arguments are passed in a struct.
 *
 * @param ptr, ptr to a thread_args struct
 */
void *openFileServer(void *ptr) {
    struct thread_args *args = (struct thread_args *) ptr;
    FILE *f = fopen(args->fileName, "r");
    int port = args->port;
    struct sockaddr_in address;
    int newSocket, mainSocket;
    int opt = 1;
    int addressLength = sizeof(address);
    if ((mainSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        return (void *) 1;
    }
    if (setsockopt(mainSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt,
                   sizeof(opt)) < 0) {
        return (void *) 1;
    }

    // Binds
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(mainSocket, (struct sockaddr *) &address,
             sizeof(address)) < 0) {
        return (void *) 1;
    }
    if (listen(mainSocket, 1) < 0) {
        return (void *) 1;
    }
    if ((newSocket = accept(mainSocket, (struct sockaddr *) &address,
                            (socklen_t *) &addressLength)) < 0) {
        return (void *) 1;
    }
    if (!sendFile(newSocket, f)) {
        return (void *) 1;
    }

    fclose(f);
    close(newSocket);
    close(mainSocket);
    return (void *) 0;
}

/**
 * Creates a TCP client to retrieve a file from a TCP server. Arguments are passed in a struct.
 *
 * @param ptr, pointer to a thread_args struct
 * @return
 */
void *openFileClient(void *ptr) {
    struct thread_args *args = (struct thread_args *) ptr;

    char *serverIp = args->ip;
    int port = args->port;
    long fileSize = args->fileSize;

    int main_socket;

    sockaddr_in address;
    int attempts = 0;
    bool connected = false;

    // Attempts multiple times to connect to server. Waits 1s between each attempt.
    while (attempts < 10 and !connected) {
        if ((main_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
            return (void *) 1;
        }

        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_port = htons(port);

        if (inet_pton(AF_INET, serverIp, &address.sin_addr) <= 0) {
            printf("Address error\n");
            return (void *) 1;
        }

        if (connect(main_socket, (struct sockaddr *) &address, sizeof(address)) == 0) {
            connected = true;
        } else {
            sleep(1);
        }
    }
    if (!connected) {
        return (void *) 1;
    }
    FILE *f = fopen(args->fileName, "w");

    // Reads whole file from server
    readFile(main_socket, f, fileSize);
    fclose(f);
    close(main_socket);
    return (void *) 0;
}

/**
 * Sends data on given socket.
 *
 * @param sock
 * @param buffer
 * @param bufferLength
 * @return
 */
bool sendData(SOCKET sock, void *buffer, int bufferLength) {
    unsigned char *pbuf = (unsigned char *) buffer;

    while (bufferLength > 0) {
        int num = send(sock, pbuf, bufferLength, 0);
        if (num == -1) {
            return false;
        }

        pbuf += num;
        bufferLength -= num;
    }
    return true;
}

/**
 * Sends entire file on given socket.
 *
 * @param sock
 * @param file
 * @return
 */
bool sendFile(SOCKET sock, FILE *file) {
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);
    if (fileSize == EOF)
        return false;
    if (fileSize > 0) {
        char buffer[1024];
        do {
            size_t num = (size_t)MIN(fileSize, 1024);
            num = fread(buffer, 1, num, file);
            if (num < 1)
                return false;
            if (!sendData(sock, buffer, num))
                return false;
            fileSize -= num;
        } while (fileSize > 0);
    }
    return true;
}


/**
 * Reads data from given socket.
 *
 * @param sock
 * @param buffer
 * @param bufferLength
 * @return
 */
bool readData(SOCKET sock, void *buffer, int bufferLength) {
    unsigned char *pbuf = (unsigned char *) buffer;

    while (bufferLength > 0) {
        int num = recv(sock, pbuf, bufferLength, 0);
        if (num == -1 or num == 0)
            return false;

        pbuf += num;
        bufferLength -= num;
    }

    return true;
}


/**
 * Reads entire file form given socket.
 *
 * @param sock
 * @param file
 * @param fileSize
 * @return
 */
bool readFile(SOCKET sock, FILE *file, long fileSize) {
    if (fileSize > 0) {
        char buffer[1024];
        do {
            int num = (int)MIN(fileSize, 1024);
            if (!readData(sock, buffer, num))
                return false;
            int offset = 0;
            do {
                size_t written = fwrite(&buffer[offset], 1, num - offset, file);
                if (written < 1)
                    return false;
                offset += written;
            } while (offset < num);
            fileSize -= num;
        } while (fileSize > 0);
    }
    return true;
}