//
// Created by jpricarte on 28/07/22.
//

#include <sstream>
#include <cstring>
#include "communication.h"

namespace communication {
    void Transmitter::sendPackage(const Packet &packet) {
        // send command
        auto res = write(socketfd, (const void *) &packet, sizeof(Packet));
        if (res < 0)
        {
            throw SocketWriteError();
        }
        res = write(socketfd, (const void *) packet._payload, packet.length);
        if (res < 0)
        {
            throw SocketWriteError();
        }
    }


    Transmitter::~Transmitter() {
        close(this->socketfd);
    }

    Transmitter::Transmitter(sockaddr_in *clientAddr, int socketfd) : client_addr(clientAddr), socketfd(socketfd) {}

    Packet Transmitter::receivePackage() {
        Packet packet{};
        int res = read(socketfd, (void*) &packet, sizeof(Packet));
        if (res < 0) {
            std::cerr << "Error in header" << std::endl;
            throw SocketReadError();
        }
        char buf[packet.length];
        bzero(buf, packet.length);
        res = read(socketfd, (void*) buf, packet.length);
        if (res < 0) {
            std::cerr << "Error in payload" << std::endl;
            throw SocketReadError();
        }
        std::cout << buf << std::endl;
        packet._payload = new char[packet.length];
        strcpy(packet._payload, buf);
        return packet;
    }

} // communication