#ifndef GRASS_H
#define GRASS_H

#include "User.hpp"
#include <set>


void hijack_flow();

// Define some constants that are needed in multiple classes/files
extern std::set<User> connected_users;
extern std::string baseDirectory;

#endif
