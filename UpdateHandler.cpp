//
// Created by jpricarte on 07/09/22.
//

#include "UpdateHandler.h"

UpdateHandler::UpdateHandler(communication::Transmitter *transmitter, FileManager* file_manager) :
transmitter(transmitter), file_manager(file_manager) {}

void UpdateHandler::waitForFiles() {
    communication::Command last_command = communication::NOP;
    while(last_command != communication::EXIT)
    {
        try {
            auto packet = transmitter->receivePacket();
            last_command = packet.command;
            std::string filename = packet._payload;
            std::size_t file_size = packet.total_size;
            if (last_command == communication::SYNC_UPLOAD)
            {
                getSyncFile(filename, file_size);
            } else if (last_command == communication::SYNC_DELETE)
            {
                deleteFile(filename);
            }
        } catch (SocketReadError& e) {
            std::cerr << e.what() << std::endl;
        }
    }
}

void UpdateHandler::getSyncFile(const std::string &filename, std::size_t file_size) {
    std::string tmp_name = '.' + filename + ".tmp";
    std::ofstream tmp_file{tmp_name, std::ofstream::binary};

    if (tmp_file)
    {
        saveDataFlow(tmp_file, file_size);
        tmp_file.close();
        file_manager->moveFile(tmp_name, filename);
    }
    else {
        std::cerr << filename << ": can't open" << std::endl;
    }
}

void UpdateHandler::saveDataFlow(std::ofstream &tmp_file, std::size_t file_size) {
    auto last_command = communication::DOWNLOAD;
    communication::Packet packet{};
    std::size_t current_size = 0;
    while(last_command != communication::OK)
    {
        try {
            packet = transmitter->receivePacket();
            last_command = packet.command;
            current_size += packet.length;

            if (last_command == communication::DOWNLOAD)
            {
                tmp_file.write(packet._payload, packet.length);
            }
        } catch (SocketReadError& e) {
            std::cerr << e.what() << std::endl;
            break;
        }
    }
}

void UpdateHandler::deleteFile(const std::string &filename) {
    file_manager->deleteFile(filename);
}
