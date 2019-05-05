#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "Parser.hpp"
#include "networking.hpp"
#include "commands.hpp"

using namespace std;

/*
 * -------------------------------- Constants --------------------------------------------------------------------------
 */

#define DEFAULT_MODE_ARGC 3
#define AUTO_MODE_ARGC 5
#define IP_SIZE 32
#define RESPONSE_MAX_SIZE 2000000
const string TRANSFER_ERROR = "Error: file transfer failed.\n";
const string THREAD_ERROR = "Error: Unable to create new thread.\n";

int runClient(char *serverIp, uint16_t serverPort, istream &infile, ostream &outfile, bool automated_mode);


/*
 * -------------------------------- Client Main ------------------------------------------------------------------------
 */

/**
 * Main function for GRASS client.
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, const char *argv[]) {
    bool automated_mode = (argc == AUTO_MODE_ARGC);

    if (argc != DEFAULT_MODE_ARGC and !automated_mode) {
        cerr << "Expected command: ./client server-ip server-port [infile outfile]\n";
        return -1;
    };

    // parsing server IP
    char serverIp[1024];
    strncpy(serverIp, argv[1], IP_SIZE);

    // parsing server port
    int tmp = stoi(argv[2]);
    uint16_t serverPort(0);
    if (tmp <= static_cast<int>(UINT16_MAX) && tmp >= 0) {
        serverPort = static_cast<uint16_t>(tmp);
    } else {
        cerr << "Error: server port cannot be cast to uint16\n";
        return 1;
    }

    // parsing input/output files
    istream &infile = automated_mode ? *(new ifstream(argv[3])) : cin;
    ostream &outfile = automated_mode ? *(new ofstream(argv[4])) : cout;

    int res = runClient(serverIp, serverPort, infile, outfile, automated_mode);

    return res;
}


/*
 * -------------------------------- Helper functions -------------------------------------------------------------------
 */

/**
 * Helper function in charge of doing all the heavy lifting.
 * 
 * @param serverIp
 * @param serverPort
 * @param infile
 * @param outfile
 * @param automated_mode
 * @return
 */
int runClient(char *serverIp, uint16_t serverPort, istream &infile, ostream &outfile, bool automated_mode) {

    //Local variables setup
    int mainSocket = 0;
    struct sockaddr_in serverAddress;

    char buffer[RESPONSE_MAX_SIZE] = {0};
    char copiedBuffer[RESPONSE_MAX_SIZE] = {0};
    char cmdCopy[1024];
    char *token;
    string cmd;

    pthread_t getThread;
    pthread_t putThread;

    char fileName[1024];
    long long fileSize = 0;
    int port = 0;

    int attempts = 0;
    bool connected = false;
    bool exit = false;

    //Attempts multiple times to connect to server. Waits 1s between each attempt.
    while (attempts < 10 and !connected) {

        // Network setup
        if ((mainSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            cerr << "Socket creation error" << endl;
            return 1;
        }

        memset(&serverAddress, 0, sizeof(serverAddress));
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(serverPort);

        if (inet_pton(AF_INET, serverIp, &serverAddress.sin_addr) <= 0) {
            return 1;
        }

        if (connect(mainSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == 0) {
            // Successful connection
            connected = true;
        } else {
            // Unsuccessful connection, waits before attempting again
            sleep(1);
        }
    }

    if (!connected) {
        return 1;
    }

    while (true) {

        // Reads line from user input
        getline(infile, cmd);

        // Check if end of file reached, or exit command sent
        if (infile.eof()) {
            // closing connection
            close(mainSocket);
            break;
        }

        // This line avoids sending an empty line and waiting indefinitely
        if (cmd.empty()) {
            continue;
        }


        // Extract file name and file size from get/put command if applicable.
        strncpy(cmdCopy, cmd.c_str(), RESPONSE_MAX_SIZE);
        token = strtok(cmdCopy, " ");
        if (token != nullptr and strcmp(token, "get") == 0) {
            memset(fileName, 0, 1024);
            token = strtok(nullptr, " ");
            if (token != nullptr) {
                strncpy(fileName, token, 1024);
            }
        } else if (token != nullptr and strcmp(token, "put") == 0) {
            memset(fileName, 0, 1024);
            token = strtok(nullptr, " ");
            if (token != nullptr) {
                strncpy(fileName, token, 1024);
            }
            token = strtok(nullptr, " ");
            if (token != nullptr) {
                fileSize = stoll(token);
            }
        } else if (token != nullptr and strcmp(token, "exit") == 0) {
            exit = true;
        }

        // sends command to the server
        send(mainSocket, cmd.c_str(), strlen(cmd.c_str()), 0);
        if(exit){
            break;
        }

        // wait for server to respond to the command sent
        ssize_t  valread = read(mainSocket, buffer, RESPONSE_MAX_SIZE-1);
        if(valread >= 0){
            buffer[valread] = '\0';
        }
        strncpy(copiedBuffer, buffer, RESPONSE_MAX_SIZE);

        string bufString = string(buffer);

        // trim string, since we send empty spaces when command returns nothing.
        bufString.erase(0, bufString.find_first_not_of(' '));

        memset(buffer, 0, RESPONSE_MAX_SIZE);
        token = strtok(copiedBuffer, " ");

        /*
         * When the response received by the client corresponds to a get command response, a thread
         * is created to receive the requested file in parallel
         */
        if (token and strcmp(token, "get") == 0 and strcmp(strtok(nullptr, " "), "port:") == 0) {

            // Extract port and fileSize from received command string
            port = atoi(strtok(nullptr, " "));
            strtok(nullptr, " ");
            fileSize = stol(strtok(nullptr, " "));

            // Prepare struct containing arguments to function in parallel thread
            struct thread_args *args = (struct thread_args *) malloc(sizeof(struct thread_args));
            strncpy(args->fileName, fileName, 1024);
            args->port = port;
            args->fileSize = fileSize;
            strncpy(args->ip, serverIp, 1024);

            // Kill any stale thread
            pthread_cancel(getThread);

            // Create new thread
            int rc = pthread_create(&getThread, nullptr, openFileClient, (void *) args);
            if (rc != 0) {
                cerr << THREAD_ERROR;
            }

            /*
            * When the response received by the client corresponds to a put command response, a thread
            * is created to send the requested file in parallel
            */
        } else if (token and strcmp(token, "put") == 0 and strcmp(strtok(nullptr, " "), "port:") == 0) {
            // Extract port from received command string
            port = atoi(strtok(nullptr, " "));
            // Prepare struct containing arguments to function in parallel thread
            struct thread_args *args = (struct thread_args *) malloc(sizeof(struct thread_args));
            strncpy(args->fileName, fileName, 1024);
            args->port = port;
            args->fileSize = fileSize;

            // Kill stale thread
            pthread_cancel(putThread);

            // Create new thread
            if (access(fileName, F_OK) == -1) {
                outfile << TRANSFER_ERROR;
            } else {
                FILE *f = fopen(args->fileName, "r");
                fseek(f, 0, SEEK_END);
                long actualFileSize = ftell(f);
                rewind(f);
                fclose(f);
                if (fileSize != actualFileSize) {
                    outfile << TRANSFER_ERROR;
                } else {
                    int rc = pthread_create(&putThread, nullptr, openFileServer, (void *) args);
                    if (rc != 0) {
                        cerr << THREAD_ERROR;
                    }
                }
            }
        } else {
            outfile << bufString;
        }
    }

    if(exit){
        close(mainSocket);
    }
    if (automated_mode) {
        delete (&infile);
        delete (&outfile);
    }
    return 0;
}