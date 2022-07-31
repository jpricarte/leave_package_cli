//
// Created by jpricarte on 31/07/22.
//

#ifndef LEAVE_PACKAGE_CLI_COMMANDHANDLER_H
#define LEAVE_PACKAGE_CLI_COMMANDHANDLER_H

#include <iostream>
#include "communication.h"
#include <unistd.h>
#include <cstring>
#include <vector>
#include <sstream>

using namespace std;
using namespace communication;

Command parseCommand(std::string s);

vector<string> split_str( std::string const &str, const char delim);

void handleCommand(const Command& command, const vector<string>& args);

void communicationHandler(communication::Transmitter* transmitter);

#endif //LEAVE_PACKAGE_CLI_COMMANDHANDLER_H
