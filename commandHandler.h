//
// Created by jpricarte on 31/07/22.
//

#ifndef LEAVE_PACKAGE_CLI_COMMANDHANDLER_H
#define LEAVE_PACKAGE_CLI_COMMANDHANDLER_H

#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <sstream>
#include <sys/inotify.h>
#include <sys/fcntl.h>
#include <condition_variable>
#include <future>

#include "communication.h"
#include "lp_exceptions.h"
#include "fileManager.h"

class CommandHandler {

    std::binary_semaphore* in_use_semaphore;
    FileManager* file_manager;
    communication::Transmitter* transmitter;
    int watcher_fd;
    bool keep_running;

    std::condition_variable* should_start_cv;

    void handleCommand(const communication::Command& command, const std::vector<std::string>& args);
    void uploadFile(const std::string& filename);
    void downloadFile(const std::string& filename);
    void deleteFile(const std::string& filename);
    void getSyncDir();
    void deleteOldFiles();
    void listServer();
    void listClient();
    void getSyncFile(const std::string& filename, std::size_t file_size);
    void saveDataFlow(std::ofstream &tmp_file, std::size_t file_size);

public:
    explicit CommandHandler(communication::Transmitter *transmitter);

    bool shouldKeepRunning() const;

    void setKeepRunning(bool keepRunning);

    virtual ~CommandHandler();

    void handle();
    void watchFiles(std::future<void> exit_signal);
    void simpleSync(std::future<void> exit_signal);
    void SyncDevice(std::future<void> exit_signal);

    FileManager *getFileManager() const;

    void setFileManager(FileManager *fileManager);

};

#endif //LEAVE_PACKAGE_CLI_COMMANDHANDLER_H