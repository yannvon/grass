#include "User.hpp"

bool User::isAuthenticated() { return this->authenticated; }

User::User(int socket, string ip, string base) {
    this->socket = socket;
    this->authenticated = false;
    this->uname = "";
    this->ip = ip;
    this->location = base;
    this->putThread = 0;
    this->getThread = 0;
}

int User::getSocket() const {
    return this->socket;
}

void User::setUname(const string &uname) {
    this->uname = uname;
}

void User::resetUname() {
    this->setUname("");
}

void User::setLocation(const string &location) {
    this->location = location;
}

void User::setAuthenticated(bool auth) {
    this->authenticated = auth;
}

string User::getUname() const {
    return this->uname;
}

string User::getLocation() {
    return this->location;
}

string User::getIp() {
    return this->ip;
}

bool User::operator<(const User &other) const {
    return this->socket > other.socket;
}

