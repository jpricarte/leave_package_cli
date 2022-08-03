//
// Created by jpricarte on 02/08/22.
//

#include <iostream>
#include <fstream>
#include "fileManager.h"

FileManager::FileManager(const std::string &folderName) {
    std::filesystem::create_directory(folderName);
    path = "./"+folderName;

    reading_writing_semaphore = new std::binary_semaphore(1);
    readers_mutex = new std::binary_semaphore(1);
    readers_counter = 0;
}

// TODO: arrumar o fim do arquivo
void FileManager::createFile(const std::string &filename, const std::string &content) {
    std::string filepath = std::filesystem::relative(path);
    filepath += "/" + filename;

    reading_writing_semaphore->acquire();
    std::ofstream file(filepath);
    if (file) {
        file << content;
        file.close();
    }
    reading_writing_semaphore->release();
}

void FileManager::deleteFile(const std::string& filename) {
    std::string filepath = std::filesystem::relative(path);
    filepath += "/" + filename;

    reading_writing_semaphore->acquire();
    std::filesystem::remove(filepath);
    reading_writing_semaphore->release();
}

// TODO: arrumar o fim do arquivo
std::string FileManager::readFile(const std::string &filename) {
    std::string filepath = std::filesystem::relative(path);
    filepath += "/" + filename;

    readers_mutex->acquire();
    readers_counter++;
    if (readers_counter == 1)
    {
        reading_writing_semaphore->acquire();
    }
    readers_mutex->release();

    std::ifstream file(filepath);
    std::string str{};

    if (file)
    {
        while(file) {
            str.push_back(file.get());
        }
        file.close();
    }
    str.push_back('\n');

    readers_mutex->acquire();
    readers_counter--;
    if (readers_counter == 0)
    {
        reading_writing_semaphore->release();
    }
    readers_mutex->release();

    return str;
}

std::string FileManager::listFiles() {
    readers_mutex->acquire();
    readers_counter++;
    if (readers_counter == 1)
    {
        reading_writing_semaphore->acquire();
    }
    readers_mutex->release();

    std::string str;
    for (const auto & file : std::filesystem::directory_iterator(path))
    {
        str += file.path();
        str.push_back(',');
    }

    readers_mutex->acquire();
    readers_counter--;
    if (readers_counter == 0)
    {
        reading_writing_semaphore->release();
    }
    readers_mutex->release();

    return str;
}

// TODO: arrumar o fim do arquivo
std::string FileManager::readUnwatchedFile(const std::string& filename) {
    std::ifstream file(filename);
    std::string str{};
    if (file)
    {
        while(file) {
            str.push_back(file.get());
        }
        str.push_back('\n');
        file.close();
    }
    return str;
}
