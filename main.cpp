#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <vector>
#include <thread>

#include "communication.h"
#include "commandHandler.h"


using namespace std;
using namespace communication;


int main(int argc, char* argv[]) {

    if (argc < 4) {
        fprintf(stderr,"usage %s <username> <hostname> <port>\n", argv[0]);
        exit(0);
    }

    struct hostent *server = gethostbyname(argv[2]);
    if (server == nullptr) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        printf("ERROR opening socket\n");

    struct sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
    bzero(&(serv_addr.sin_zero), 8);


    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
    {
        printf("ERROR connecting\n");
        return -1;
    }
    auto* transmitter = new communication::Transmitter(&serv_addr, sockfd);

    std::string username = argv[1];
    communication::Packet packet{
        communication::LOGIN,
        0,
        username.size(),
        (unsigned int) username.size(),
        const_cast<char *>(username.c_str())
    };

    try {
        transmitter->sendPackage(packet);
    } catch (SocketWriteError& e) {
        std::cerr << e.what() << std::endl;
    }

    communication::Command response = communication::EXIT;
    try {
        auto result = transmitter->receivePackage();
        cout << result._payload << endl;
        response = result.command;
    } catch (SocketReadError& e) {
        std::cerr << e.what() << std::endl;
    }

    if (response != communication::OK)
    {
        cerr << "something went wrong in server" << endl;
        close(sockfd);
        return -2;
    }

    auto* command_handler = new CommandHandler(transmitter);

    auto sender = thread(&CommandHandler::handle, command_handler);
    auto watcher = thread(&CommandHandler::watchFiles, command_handler);
    auto listener = thread(&CommandHandler::listenCommands, command_handler);

    sender.join();
    watcher.join();
    listener.join();

    delete command_handler;
    close(sockfd);
    return 0;
}
