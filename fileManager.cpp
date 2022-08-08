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
            str.push_back(file.get());
        }
    }
    file.close();

    readers_mutex->acquire();
    readers_counter--;
    if (readers_counter == 0)
    {
        reading_writing_semaphore->release();
    }
    readers_mutex->release();

    return str;
}

void FileManager::moveFile(const std::string &tmp_file, const std::string &filename) {
    std::string filepath = std::filesystem::relative(path);
    filepath += "/" + filename;

    readers_mutex->acquire();
    readers_counter++;
    if (readers_counter == 1)
    {
        reading_writing_semaphore->acquire();
    }
    readers_mutex->release();

    std::filesystem::rename(tmp_file, filepath);

    readers_mutex->acquire();
    readers_counter--;
    if (readers_counter == 0)
    {
        reading_writing_semaphore->release();
    }
    readers_mutex->release();
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

const std::filesystem::path &FileManager::getPath() const {
    return path;
}

void FileManager::copyFile(const std::string &file_orig, const std::string &file_dest) {
    readers_mutex->acquire();
    readers_counter++;
    if (readers_counter == 1)
    {
        reading_writing_semaphore->acquire();
    }
    readers_mutex->release();

    std::filesystem::copy_file(file_orig, file_dest);

    readers_mutex->acquire();
    readers_counter--;
    if (readers_counter == 0)
    {
        reading_writing_semaphore->release();
    }
    readers_mutex->release();
}



