//
// Created by jpricarte on 31/07/22.
//

#include <fstream>
#include "commandHandler.h"

using namespace std;
using namespace communication;


// Code from: https://www.javatpoint.com/how-to-split-strings-in-cpp
vector<string> split_str( std::string const &str, const char delim)
{
    // create a stream from the string
    std::stringstream stream(str);

    std::string buf;
    std::vector <std::string> out{};
    while (std::getline(stream, buf, delim))
    {
        out.push_back(buf); // store the string in s2
    }
    return out;
}

Command parseCommand(const std::string& s) {
    if (s == "exit")
        return EXIT;
    if (s == "list_client")
        return LIST_CLIENT;
    if (s == "list_server")
        return LIST_SERVER;
    if (s == "get_sync_dir")
        return GET_SYNC_DIR;
    if (s == "upload")
        return UPLOAD;
    if (s == "download")
        return DOWNLOAD;
    if (s == "delete")
        return DELETE;
    else
        return NOP;
}

CommandHandler::CommandHandler(Transmitter *transmitter) : transmitter(transmitter) {
    if (filesystem::exists("sync_dir")) {
        file_manager = new FileManager("sync_dir");
    } else {
        file_manager = nullptr;
    }
}


CommandHandler::~CommandHandler() {
    delete transmitter;
}

void CommandHandler::handle()
{
    communication::Command last_command = communication::NOP;
    while(last_command != communication::EXIT)
    {
        string input{};
        cout << ">> ";
        getline(cin, input);
        if (input.empty())
        {
            continue;
        }
        auto args = split_str(input, ' ');
        if (args.size() > 2)
        {
            cout << "invalid command" << endl;
            continue;
        }
        last_command = parseCommand(args[0]);

        in_use_semaphore->acquire();
        try {
            handleCommand(last_command, args);
        } catch(InvalidNumOfArgs& e) {
            cout << "Invalid number of arguments, received " << args.size()
                 << " intead of 2. Received:" << endl;
            for (auto arg : args)
            {
                cout << "- " << arg << endl;
            }
        }
        in_use_semaphore->release();
    }
    string end_connection = "goodbye my friend!";
    Packet finishConnectionPacket{
            EXIT,
            1,
            end_connection.size(),
            (unsigned int) strlen(end_connection.c_str()),
            (char*) end_connection.c_str()
    };
    transmitter->sendPackage(finishConnectionPacket);
}

void CommandHandler::handleCommand(const Command& command, const vector<string>& args) {

    if ( (command == UPLOAD || command == DOWNLOAD || command == DELETE) && args.size() != 2 )
        throw InvalidNumOfArgs();

    if (file_manager == nullptr &&
        command != GET_SYNC_DIR && command != LIST_SERVER && command != NOP && command != EXIT)
    {
        cout << "You should use get_sync_dir first" << endl;
        return;
    }

    switch (command) {
        case UPLOAD:
            uploadFile(args[1]);
            break;
        case DOWNLOAD:
            downloadFile(args[1]);
            break;
        case DELETE:
            deleteFile(args[1]);
            break;
        case GET_SYNC_DIR:
            getSyncDir();
            break;
        case LIST_SERVER:
            listServer();
            break;
        case LIST_CLIENT:
            listClient();
            break;
        case NOP:
            cerr << "That's not a command" << endl;
        default:
            break;
    }
}

void CommandHandler::uploadFile(const string& file_path)
{
    string filename{};
    size_t filesize;
    try {
        filesystem::path fpath(file_path);
        filename = fpath.filename();
        filesize = file_size(fpath);
    } catch (std::filesystem::__cxx11::filesystem_error& e) {
        std::cerr << "File not found" << std::endl;
    }
    ifstream file(filename, ios::binary);
    if (!file)
    {
        return;
    }

    // primeiro, envia o nome e outros metadados (se precisar) do arquivo
    Packet metadata_packet {
            communication::UPLOAD,
            1,
            filesize,
            (unsigned int) strlen(filename.c_str()),
            (char*) filename.c_str()
    };

    try {
        transmitter->sendPackage(metadata_packet);
    } catch (SocketWriteError& e) {
        cerr << e.what() << endl;
    }

    // depois, envia o arquivo em partes de 255 Bytes
    const unsigned int BUF_SIZE = 255;
    char buf[BUF_SIZE] = {};
    unsigned int i = 2;
    while(!file.eof())
    {
        bzero(buf, BUF_SIZE);
        file.read((char*) buf, BUF_SIZE);
        Packet data_packet {
                communication::UPLOAD,
                i,
                filesize,
                (unsigned int) strlen(buf),
                buf
        };

        try {
            transmitter->sendPackage(data_packet);
        } catch (SocketWriteError& e) {
            cerr << e.what() << endl;
            break;
        }

        try {
            communication::Packet packet = transmitter->receivePackage();
            if (packet.command == communication::OK) {
                i++;
            } else {
                break;
            }
        } catch (SocketReadError& e) {
            cerr << e.what() << endl;
            break;
        }
    }

    try {
        transmitter->sendPackage(communication::SUCCESS);
    } catch (SocketWriteError& e) {
        cerr << e.what() << endl;
    }

}

void CommandHandler::downloadFile(const string& filename)
{
    std::string sync_file_path = file_manager->getPath();
    sync_file_path += "/" + filename;
    try {
        filesystem::path p(sync_file_path);
    } catch (filesystem::filesystem_error& e) {
        cerr << "File not found" << endl;
    }
}

void CommandHandler::deleteFile(const string& filename)
{
    Packet packet {
            communication::DELETE,
            1,
            filename.size(),
            (unsigned int) strlen(filename.c_str()),
            (char*) filename.c_str()
    };
    try {
        transmitter->sendPackage(packet);
    } catch (SocketWriteError& e) {
        cerr << e.what() << endl;
    }
    //TODO: deletar arquivo da pasta local
}

void CommandHandler::getSyncDir()
{
    if (file_manager == nullptr)
    {
        file_manager = new FileManager("sync_dir");
    }

    Packet packet{
        communication::GET_SYNC_DIR,
        1,
        2,
        2,
        (char*) "."
    };
    try {
        transmitter->sendPackage(packet);
    } catch (SocketWriteError& e) {
        cerr << e.what() << endl;
    }

    do {
        try {
            packet = transmitter->receivePackage();
//            cout << packet._payload << endl;
            if (packet.command == communication::DOWNLOAD)
                getSyncFile(packet._payload);
        } catch (SocketReadError& e) {
            cout << e.what() << endl;
        }
    } while(packet.command != communication::OK);
}

void CommandHandler::listServer()
{
    string message = "show me your files.";
    Packet packet {
            communication::LIST_SERVER,
            1,
            message.size(),
            (unsigned int) strlen(message.c_str()),
            (char*) message.c_str()
    };

    try {
        transmitter->sendPackage(packet);
    } catch (SocketWriteError& e) {
        cerr << e.what() << endl;
    }

    try {
        auto response = transmitter->receivePackage();
        string files_list_raw{response._payload};
        auto files_list = split_str(files_list_raw, ',');
        for (const auto& str : files_list)
        {
            cout << str << endl;
        }
    } catch (SocketReadError& e) {
        cerr << e.what() << endl;
    }
}

void CommandHandler::listClient()
{
    auto files_list_raw = file_manager->listFiles();
    auto files_list = split_str(files_list_raw, ',');
    for (const auto& str : files_list)
    {
        cout << str << endl;
    }
}

void CommandHandler::getSyncFile(const string &filename) {
    std::string tmp_name = '.' + filename + ".tmp";
    std::ofstream tmp_file{tmp_name, std::ofstream::binary};

    if (tmp_file)
    {
        saveDataFlow(tmp_file);
        tmp_file.close();
        file_manager->moveFile(tmp_name, filename);
    }
}

void CommandHandler::saveDataFlow(std::ofstream &tmp_file) {
    auto last_command = communication::DOWNLOAD;
    while(last_command != communication::OK)
    {
        try {
            auto packet = transmitter->receivePackage();
            last_command = packet.command;
            if (last_command == communication::DOWNLOAD)
            {
                tmp_file.write((char*)packet._payload, packet.length);
            }
        } catch (SocketReadError& e) {
            std::cerr << e.what() << std::endl;
            break;
        } catch (...) {
            try {
                transmitter->sendPackage(communication::ERROR);
            } catch (SocketWriteError& e2) {
                std::cerr << e2.what() << std::endl;
                break;
            }
        }

        if (last_command != communication::OK)
        {
            try {
                transmitter->sendPackage(communication::SUCCESS);
            } catch (SocketWriteError& e) {
                std::cerr << e.what() << std::endl;
                break;
            }
        }
    }
}

void CommandHandler::watchFiles() {
    const uint16_t MAX_EVENTS = 512;
    const uint16_t LEN_NAME = 32;  /* Assuming that the length of the filename won't exceed 16 bytes*/
    const size_t EVENT_SIZE  = (sizeof (struct inotify_event)); /*size of one event*/
    const size_t BUF_LEN = ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME ));

    watchfd = inotify_init();
    if (fcntl(watchfd, F_SETFL, O_NONBLOCK) < 0)  // error checking for fcntl
        cerr << "cannot watch for some error" << endl;

    int watch_descriptor = inotify_add_watch(watchfd, "sync_dir",
                                             IN_CREATE | IN_MODIFY | IN_DELETE);


    if (watch_descriptor < 0)
    {
        cerr << "could not watch sync_dir, will not sync" << endl
             << "Try use get_sync_dir and restart your client" << endl;
        return;
    }

    while (true)
    {
        int i=0,length;
        char buffer[BUF_LEN];

        /* Step 3. Read buffer*/
        length = read(watchfd,buffer,BUF_LEN);

        while (i<length)
        {
            auto *event = (struct inotify_event *) &buffer[i];
            if(event->len){
                if ( event->mask & IN_CREATE ) {
                    if ( event->mask & IN_ISDIR ) {
                        printf( "The directory %s was created. Will not sync\n", event->name );
                    }
                    else {
                        string file_path = file_manager->getPath();
                        file_path += "/";
                        file_path += (char*) event->name;

                        in_use_semaphore->acquire();
                        uploadFile(file_path);
                        in_use_semaphore->release();
                    }
                }
                else if ( event->mask & IN_DELETE ) {
                    if ( event->mask & IN_ISDIR ) {
                        printf( "The directory %s was deleted. Will not sync\n", event->name );
                    }
                    else {
                        in_use_semaphore->acquire();
                        deleteFile(event->name);
                        in_use_semaphore->release();
                    }
                }
                else if ( event->mask & IN_MODIFY ) {
                    if ( event->mask & IN_ISDIR ) {
                        printf( "The directory %s was modified. Will not sync\n", event->name );
                    }
                    else {
                        string file_path = file_manager->getPath();
                        file_path += "/";
                        file_path += (char*) event->name;

                        in_use_semaphore->acquire();
                        uploadFile(file_path);
                        in_use_semaphore->release();
                    }
                }
            }
            i += EVENT_SIZE + event->len;
        }

    }
}

void CommandHandler::listenCommands() {
    Packet command_packet = transmitter->receivePackage();
    if (command_packet.command == communication::SYNC_DOWN)
    {
        in_use_semaphore->acquire();
        getSyncFile(command_packet._payload);
        in_use_semaphore->release();
    }
    if (command_packet.command == communication::SYNC_DELETE)
    {
        in_use_semaphore->acquire();
        file_manager->deleteFile(command_packet._payload);
        in_use_semaphore->release();
    }
}
