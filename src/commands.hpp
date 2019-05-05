#ifndef GRASS_COMMANDS_H
#define GRASS_COMMANDS_H

#define MAX_PATH_LEN 128
#define BFLNTH 150

#include <cstdlib>
#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <cstring>
#include <sstream>
#include <iterator>
#include <unistd.h>
#include <cstdint>
#include "User.hpp"
#include "grass.hpp"

using namespace std;

extern const string ACCESS_ERROR;
extern const string FILENAME_ERROR;
extern const string TRANSFER_ERROR;
extern const string AUTHENTICATION_FAIL;
extern const string ALREADY_LOGGED_IN;
extern const string THREAD_ERROR;

string escape(string cmd);

size_t split(vector<string> &res, const string &line, const char *delim);

int mkdir_cmd(string dir, User usr, string &out);

int cd_cmd(string dir, User &usr, string &out);

int ls_cmd(bool authenticated, string &out, User usr);

int get_cmd(string fileName, int getPort, User &usr, string &out);

int put_cmd(string fileName, string fileSize, int port, User &usr, string &out);

int login_cmd(string uname, map<string, string> allowedUsers, User &usr, string &out);

int logout_cmd(User &usr, string &out);

int pass_cmd(string psw, map<string, string> allowedUsers, User &usr, string &out);

int exec(const char *cmd, string &out, string usrLocation);

int ping_cmd(string host, string &out);

int rm_cmd(string fileName, User usr, string &out);

int whoami_cmd(User usr, string &out);

int w_cmd(User usr, string &out);

int date_cmd(bool authenticated, string &out);

int grep_cmd(string pattern, User usr, string &out);

int exit_cmd(User &usr);

long getFileSize(const char *fileName);

#endif //GRASS_COMMANDS_H
