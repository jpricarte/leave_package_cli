//
// Created by jpricarte on 28/07/22.
//


#include "communication.h"

namespace communication {
    void Transmitter::sendPackage(const Packet &packet) {
        // send command
        socket_semaphore->acquire();
        auto res = write(socketfd, (const void *) &packet, sizeof(Packet));
        if (res < 0)
        {
            socket_semaphore->release();
            throw SocketWriteError();
        }
        res = write(socketfd, (const void *) packet._payload, packet.length);
        if (res < 0)
        {
            socket_semaphore->release();
            throw SocketWriteError();
        }
        socket_semaphore->release();

    }

    Transmitter::~Transmitter() {
        close(this->socketfd);
    }

    Transmitter::Transmitter(sockaddr_in *clientAddr, int socketfd) : client_addr(clientAddr), socketfd(socketfd) {
        socket_semaphore = new std::counting_semaphore<1>(1);
    }

    Packet Transmitter::receivePackage() {
        Packet packet{};
        socket_semaphore->acquire();
        int res = read(socketfd, (void*) &packet, sizeof(Packet));
        if (res < 0) {
            socket_semaphore->release();
            std::cerr << "Error in header" << std::endl;
            throw SocketReadError();
        }
        char buf[packet.length];
        bzero(buf, packet.length);
        res = read(socketfd, (void*) buf, packet.length);
        if (res < 0) {
            socket_semaphore->release();
            std::cerr << "Error in payload" << std::endl;
            throw SocketReadError();
        }
        socket_semaphore->release();

        packet._payload = new char[packet.length];
        strcpy(packet._payload, buf);
        return packet;
    }

} // communication