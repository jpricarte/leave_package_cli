//
// Created by jpricarte on 07/09/22.
//

#ifndef LEAVE_PACKAGE_CLI_UPDATEHANDLER_H
#define LEAVE_PACKAGE_CLI_UPDATEHANDLER_H

#include <fstream>
#include "communication.h"
#include "fileManager.h"

class UpdateHandler {
    communication::Transmitter* transmitter;
    FileManager* file_manager;

public:
    explicit UpdateHandler(communication::Transmitter *transmitter, FileManager* file_manager);

    void waitForFiles();

    void getSyncFile(const std::string &filename, std::size_t file_size);

    void saveDataFlow(std::ofstream &tmp_file, std::size_t file_size);

    void deleteFile(const std::string &filename);
};


#endif //LEAVE_PACKAGE_CLI_UPDATEHANDLER_H
