//
// Created by jpricarte on 28/07/22.
//


#include <vector>
#include "communication.h"

namespace communication {

    void showPacket(const Packet& packet)
    {
        std::cout << "{" << std::endl;
        std::cout << "seqn: " << packet.seqn << std::endl;
        std::cout << "total size: " << packet.total_size << std::endl;
        std::cout << "length: " << packet.length << std::endl;
        std::cout << "payload: " << std::endl;
        std::cout << packet._payload << std::endl;
        std::cout << "}" << std::endl;

    }

    Transmitter::Transmitter(sockaddr_in *clientAddr, int socketfd) : client_addr(clientAddr), socketfd(socketfd) {
        socket_semaphore = new std::counting_semaphore<1>(1);
        sync_request_semaphore = new std::counting_semaphore<10>(0);
        sync_requests = {};
    }

    Transmitter::~Transmitter() {
        close(this->socketfd);
    }

    void Transmitter::sendPacket(const Packet &packet) {
        auto* sendable_packet = new Packet{
                packet.command,
                htons(packet.seqn),
                htonl(packet.total_size),
                htons(packet.length),
                nullptr
        };

        // send command
        socket_semaphore->acquire();
        auto res = write(socketfd, sendable_packet, sizeof(Packet));
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

    Packet Transmitter::receivePacket() {
        Packet packet{};
        socket_semaphore->acquire();
        bool finish = false;
        do {
            size_t pckt_size = 0;
            while (pckt_size < sizeof(Packet)) {
                auto res = read(socketfd, &packet, sizeof(Packet));
                if (res < 0) {
                    socket_semaphore->release();
                    std::cerr << "Error in header" << std::endl;
                    throw SocketReadError();
                }
                pckt_size += res;
            }

            packet.seqn = ntohs(packet.seqn);
            packet.total_size = ntohl(packet.total_size);
            packet.length = ntohs(packet.length);
            packet._payload = new char[packet.length + 1];
            bzero(packet._payload, packet.length + 1);

            pckt_size = 0;
            while (pckt_size < packet.length) {
                auto res = read(socketfd, packet._payload, packet.length);
                if (res < 0) {
                    socket_semaphore->release();
                    std::cerr << "Error in payload" << std::endl;
                    throw SocketReadError();
                }
                pckt_size += res;
            }
            if (packet.command == SYNC_UPLOAD || packet.command == SYNC_DELETE) {
                sync_requests.push_back(packet);
                sync_request_semaphore->release();
                std::cout << "got sync command" << std::endl;
            } else finish = true;
        } while (!finish);
        socket_semaphore->release();

        return packet;
    }

    Packet Transmitter::popSyncRequest() {
        sync_request_semaphore->acquire();
        std::cout << "popping" << std::endl;
        Packet p = sync_requests.front();
        sync_requests.erase(sync_requests.begin());
        return p;
    }
} // communication