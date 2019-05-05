#ifndef GRASS_USER_H
#define GRASS_USER_H

#include <cstdint>
#include <string>

using namespace std;

class User {
public:
    explicit User(int socket, string ip, string base);

    bool isAuthenticated();

    int getSocket() const;

    string getUname() const;

    string getLocation();

    string getIp();

    void setUname(const string &uname);

    void resetUname();

    void setLocation(const string &location);

    void setAuthenticated(bool auth);

    bool operator<(const User &other) const;

    pthread_t putThread;
    pthread_t getThread;

private:
    int socket;
    bool authenticated;
    string uname;
    string location;
    string ip;
};

#endif //GRASS_USER_H
