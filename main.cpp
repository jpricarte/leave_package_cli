#include <iostream>
#include "communication.h"
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <vector>
#include <sstream>

#define PORT 4000

using namespace std;

// Code from: https://www.javatpoint.com/how-to-split-strings-in-cpp
vector<string> split_str( std::string const &str, const char delim)
{
    // create a stream from the string
    std::stringstream s(str);

    std::string s2;
    std::vector <std::string> out{};
    while (std:: getline (s, s2, delim) )
    {
        out.push_back(s2); // store the string in s2
    }
    return out;
}

void communicationHandler(communication::Transmitter* transmitter)
{
    communication::Command last_command = communication::NOP;
    while(last_command != communication::EXIT)
    {
        string input{};
        cout << ">> ";
        cin >> input;
        cout << endl;
        auto args = split_str(input, ' ');
        if (args.size() > 2)
        {
            cout << "invalid command" << endl;
            continue;
        }
        last_command = communication::parseCommand(args[0]);
    }
}

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

    if (response == communication::OK)
    {
        communicationHandler(transmitter);
    }

    close(sockfd);
    return 0;
}
