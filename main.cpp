#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <vector>
#include <sstream>
#include <thread>

#include "communication.h"
#include "commandHandler.h"

#define PORT 4001

using namespace std;
using namespace communication;


int main(int argc, char* argv[]) {
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 2) {
        fprintf(stderr,"usage %s hostname\n", argv[0]);
        exit(0);
    }

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        printf("ERROR opening socket\n");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
    bzero(&(serv_addr.sin_zero), 8);


    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
    {
        printf("ERROR connecting\n");
        return -1;
    }

    auto username = std::string("jordi\0");
    communication::Packet packet{
        communication::LOGIN,
        0,
        username.size(),
        (unsigned int) username.size(),
        const_cast<char *>(username.c_str())
    };
    cout << packet._payload << endl;
    auto* transmitter = new communication::Transmitter(&serv_addr, sockfd);
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

    auto th1 = thread(&CommandHandler::handle, command_handler);

    th1.join();

    delete command_handler;
    close(sockfd);
    return 0;
}
