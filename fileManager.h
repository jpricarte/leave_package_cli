//
// Created by jpricarte on 02/08/22.
//

#ifndef LEAVE_PACKAGE_FILEMANAGER_H
#define LEAVE_PACKAGE_FILEMANAGER_H


#include <filesystem>
#include <string>
#include <semaphore>
#include <condition_variable>

/* DECISÃO DE PROJETO
 * Foi decidido usar o FileManager como um problema de leitores/escritores
 * De forma que, quando um cliente for fazer download/mover/listagem de arquivos,
 * ele será tratado como um leitor, mas quando for fazer upload/deletar, ele
 * será um escritor.
 * Apesar de ser possível fazer implementações mais eficientes, essa facilita
 * uma implementação preguiçosa, pois assim só precisamos de 4 funções para
 * fazer todas as operações que devem ser implementadas
 */
class FileManager {

    std::binary_semaphore *reading_writing_semaphore;
    std::binary_semaphore *readers_mutex;
    int readers_counter;

    std::filesystem::path path;

public:

    FileManager(const std::string &folderName);

    void createFile(const std::string &filename, const std::string &content);

    void moveFile(const std::string &tmp_file, const std::string &filename);

    void copyFile(const std::string &file_orig, const std::string &file_dest);

    void deleteFile(const std::string &filename);

    std::string readFile(const std::string &filename);

    std::string listFiles();

    const std::filesystem::path &getPath() const;

};



#endif //LEAVE_PACKAGE_FILEMANAGER_H
