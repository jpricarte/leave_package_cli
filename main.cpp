#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <vector>
#include <thread>
#include <csignal>
#include <future>

#include "communication.h"
#include "commandHandler.h"
#include "UpdateHandler.h"


using namespace std;
using namespace communication;

Transmitter* command_transmitter = nullptr;
CommandHandler* command_handler = nullptr;

Transmitter* update_transmitter = nullptr;
UpdateHandler* update_handler = nullptr;

int sendGoodbye()
{
    if (command_transmitter != nullptr)
    {
        string end_connection = "goodbye my friend!";
        Packet finishConnectionPacket{
                EXIT,
                1,
                end_connection.size(),
                (unsigned int) strlen(end_connection.c_str()),
                (char*) end_connection.c_str()
        };
        command_transmitter->sendPacket(finishConnectionPacket);
    }
    command_handler->setKeepRunning(false);

    return 0;
}

int handleSignal()
{
    cout << "use exit" << endl;
    sendGoodbye();
    return 0;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, reinterpret_cast<__sighandler_t>(handleSignal));
    signal(SIGTERM, reinterpret_cast<__sighandler_t>(handleSignal));

    if (argc < 4) {
        fprintf(stderr,"usage %s <username> <hostname> <port>\n", argv[0]);
        exit(0);
    }

    struct hostent *server = gethostbyname(argv[2]);
    if (server == nullptr) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    int command_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (command_sockfd == -1)
        printf("ERROR opening socket\n");

    struct sockaddr_in command_addr{};
    command_addr.sin_family = AF_INET;
    command_addr.sin_port = htons(atoi(argv[3]));
    command_addr.sin_addr = *((struct in_addr *)server->h_addr);
    bzero(&(command_addr.sin_zero), 8);

    if (connect(command_sockfd, (struct sockaddr *) &command_addr, sizeof(command_addr)) < 0)
    {
        printf("ERROR connecting\n");
        return -1;
    }

    int update_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (update_sockfd == -1)
        printf("ERROR opening socket\n");

    struct sockaddr_in update_addr{};
    update_addr.sin_family = AF_INET;
    update_addr.sin_port = htons(atoi(argv[3]));
    update_addr.sin_addr = *((struct in_addr *)server->h_addr);
    bzero(&(update_addr.sin_zero), 8);

    if (connect(update_sockfd, (struct sockaddr *) &update_addr, sizeof(update_addr)) < 0)
    {
        printf("ERROR connecting\n");
        return -1;
    }

    command_transmitter = new communication::Transmitter(&command_addr, command_sockfd);

    std::string username = argv[1];
    communication::Packet packet{
            communication::LOGIN,
            0,
            username.size(),
            (unsigned int) username.size(),
            const_cast<char *>(username.c_str())
    };

    try {
        command_transmitter->sendPacket(packet);
    } catch (SocketWriteError& e) {
        std::cerr << e.what() << std::endl;
    }

    communication::Command response = communication::EXIT;
    try {
        auto result = command_transmitter->receivePacket();
        cout << result._payload << endl;
        response = result.command;
    } catch (SocketReadError& e) {
        std::cerr << e.what() << std::endl;
    }

    if (response != communication::OK)
    {
        cout << response << endl;
        cerr << "something went wrong in server" << endl;
        close(command_sockfd);
        close(update_sockfd);
        return -2;
    }
    command_handler = new CommandHandler(command_transmitter);

    update_transmitter = new Transmitter{&update_addr, update_sockfd};
    update_handler = new UpdateHandler{update_transmitter, command_handler->getFileManager()};

    std::promise<void> watcher_exit;
    std::future<void> watcher_sig = watcher_exit.get_future();
    std::promise<void> listener_exit;
    std::future<void> listener_sig = listener_exit.get_future();
    auto sender = thread(&CommandHandler::handle, command_handler);
    auto watcher = thread(&CommandHandler::watchFiles, command_handler, move(watcher_sig));
    auto listener = thread(&UpdateHandler::waitForFiles, update_handler);

    sender.join();
    sendGoodbye();

    watcher_exit.set_value();
    watcher.detach();

    delete command_handler;
    return 0;
}