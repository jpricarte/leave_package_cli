//
// Created by jpricarte on 28/07/22.
//

#ifndef LEAVE_PACKAGE_COMMUNICATION_H
#define LEAVE_PACKAGE_COMMUNICATION_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <ostream>
#include <atomic>
#include <iostream>
#include <string>

#include "lp_exceptions.h"

namespace communication {

    enum Command {
        LOGIN=0,
        UPLOAD,
        DOWNLOAD,
        DELETE,
        GET_SYNC_DIR,
        LIST_SERVER,
        LIST_CLIENT,
        OK,
        EXIT,
        NOP
    };

    Command parseCommand(std::string s);

    struct Packet {
        Command command; // Which command
        unsigned int seqn; // the number in the sequence
        unsigned long int total_size; // Total size of the file
        unsigned int length; // payload size
        char* _payload; // The content itselfs
    };

    class Transmitter {
        struct sockaddr_in* client_addr;
        int socketfd;

    public:
        void sendPackage(const Packet& packet);

        Packet receivePackage();

        Transmitter(sockaddr_in *clientAddr, int socketfd);

        virtual ~Transmitter();
    };

    // USABLE CONSTANTS
    const Packet LOGIN_FAIL {EXIT, 1,
                             14,
                             (unsigned int) 14,
                             (char*) "INVALID LOGIN"};

    const Packet SUCCESS {OK, 1,
                          17,
                          (unsigned int) 17,
                          (char*) "FINE (UNTIL NOW)"};
} // communication

#endif //LEAVE_PACKAGE_COMMUNICATION_H
