//
// Created by jpricarte on 28/07/22.
//


#include "communication.h"

namespace communication {

    Transmitter::Transmitter(sockaddr_in *clientAddr, int socketfd) : client_addr(clientAddr), socketfd(socketfd) {
        socket_semaphore = new std::counting_semaphore<1>(1);
    }

    Transmitter::~Transmitter() {
        close(this->socketfd);
    }

    void Transmitter::sendPackage(const Packet &packet) {
        auto* sendable_packet = new Packet{
                packet.command,
                htons(packet.seqn),
                htonl(packet.total_size),
                htons(packet.length),
                nullptr
        };

        // send command
        socket_semaphore->acquire();
        auto res = write(socketfd, (const void *) sendable_packet, sizeof(Packet));
        if (res < 0)
        {
            socket_semaphore->release();
            throw SocketWriteError();
        }
        res = write(socketfd,  packet._payload, packet.length);
        if (res < 0)
        {
            socket_semaphore->release();
            throw SocketWriteError();
        }
        socket_semaphore->release();
    }

    Packet Transmitter::receivePackage() {
        Packet packet{};
        socket_semaphore->acquire();
        auto res = read(socketfd, (void*) &packet, sizeof(Packet));
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

        packet.seqn = ntohs(packet.seqn);
        packet.total_size = ntohl(packet.total_size);
        packet.length = ntohs(packet.length);
        packet._payload = new char[packet.length];
        strcpy(packet._payload, buf);
        return packet;
    }
} // communication