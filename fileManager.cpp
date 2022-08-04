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
        file << content << std::endl;
        file.close();
    }
    else {
        std::cout << "could not create file" << std::endl;
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
            file >> str;
        }
        file.close();
    }
    std::cout << str << std::endl;
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

    std::string str{};
    for (const auto & file : std::filesystem::directory_iterator(path))
    {
        str += file.path().filename();
        str.push_back(',');
    }
    if (str.empty())
    {
        str = "nothing to show";
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
            std::string buf{};
            getline(file, buf);
            str += buf + "\n";
        }
        file.close();
    }

    return str;
}
