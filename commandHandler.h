//
// Created by jpricarte on 31/07/22.
//

#ifndef LEAVE_PACKAGE_CLI_COMMANDHANDLER_H
#define LEAVE_PACKAGE_CLI_COMMANDHANDLER_H

#include <iostream>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <sstream>

#include "communication.h"
#include "lp_exceptions.h"


void commandHandler(communication::Transmitter* transmitter);

#endif //LEAVE_PACKAGE_CLI_COMMANDHANDLER_H
