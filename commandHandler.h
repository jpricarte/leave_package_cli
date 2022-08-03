//
// Created by jpricarte on 31/07/22.
//

#ifndef LEAVE_PACKAGE_CLI_COMMANDHANDLER_H
#define LEAVE_PACKAGE_CLI_COMMANDHANDLER_H

#include <iostream>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <sstream>

#include "communication.h"
#include "lp_exceptions.h"
#include "fileManager.h"

class CommandHandler {
    FileManager* file_manager;
    communication::Transmitter* transmitter;

    void handleCommand(const communication::Command& command, const std::vector<std::string>& args);
    void uploadFile(const std::string& file_path);
    void downloadFile(const std::string& filename);
    void deleteFile(const std::string& filename);
    void getSyncDir();
    void listServer();
    void listClient();

public:
    explicit CommandHandler(communication::Transmitter *transmitter);

    virtual ~CommandHandler();

    void handle();
};

#endif //LEAVE_PACKAGE_CLI_COMMANDHANDLER_H
