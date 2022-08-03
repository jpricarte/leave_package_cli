//
// Created by jpricarte on 02/08/22.
//

#ifndef LEAVE_PACKAGE_FILEMANAGER_H
#define LEAVE_PACKAGE_FILEMANAGER_H


#include <filesystem>
#include <string>
#include <semaphore>

/* DECISÃO DE PROJETO
 * Foi decidido usar o FileManager como um problema de leitores/escritores
 * De forma que, quando um cliente for fazer download/listagem de arquivos,
 * ele será tratado como um leitor, mas quando for fazer upload/deletar, ele
 * será um escritor.
 * Apesar de ser possível fazer implementações mais eficientes, essa facilita
 * uma implementação preguiçosa, pois assim só precisamos de 4 funções para
 * fazer todas as operações que devem ser implementadas
 */
class FileManager {

    std::binary_semaphore* reading_writing_semaphore;
    std::binary_semaphore* readers_mutex;
    int readers_counter;

    std::filesystem::path path;

public:

    FileManager(const std::string &folderName);

    void createFile(const std::string& filename, const std::string& content);

    void deleteFile(const std::string& filename);

    std::string readFile(const std::string& filename);

    std::string listFiles();

    // CAUTION: NOT THREAD SAFE (read from outside the get_sync_dir
    // filename must be the absolute or relative path from the running dir, not from
    // get_sync_dir
    static std::string readUnwatchedFile(const std::string& filename);
};


#endif //LEAVE_PACKAGE_FILEMANAGER_H
