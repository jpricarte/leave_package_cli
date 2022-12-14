//
// Created by jpricarte on 28/07/22.
//

#ifndef LEAVE_PACKAGE_COMMUNICATION_H
#define LEAVE_PACKAGE_COMMUNICATION_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <ostream>
#include <semaphore>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <atomic>

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
        SYNC_UPLOAD,
        SYNC_DELETE,
        NOP
    };

    struct Packet {
        Command command; // Which command
        unsigned int seqn; // the number in the sequence
        unsigned long int total_size; // Total size of the file
        unsigned int length; // payload size
        char* _payload; // The content itselfs
    };

    void showPacket(const Packet& packet);

    class Transmitter {
        std::binary_semaphore* socket_semaphore;
        struct sockaddr_in* client_addr;
        int socketfd;

    public:
        void sendPacket(const Packet& packet);

        Packet receivePacket();

        Transmitter(sockaddr_in *clientAddr, int socketfd);

        virtual ~Transmitter();


    };

    const Packet SUCCESS {OK, 1,
                          17,
                          (unsigned int) 17,
                          (char*) "FINE (UNTIL NOW)"};

    const Packet ERROR {NOP, 1,
                        6,
                        (unsigned int) 6,
                        (char*) "OH OH"};

} // communication

#endif //LEAVE_PACKAGE_COMMUNICATION_H