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
    in_use_semaphore = new counting_semaphore<1>(1);
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
            for (const auto& arg : args)
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
    transmitter->sendPacket(finishConnectionPacket);
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


void CommandHandler::uploadFile(const string &filename)
{

    size_t filesize;
    try {
        filesize = file_size(filesystem::path(filename));
    } catch (std::filesystem::__cxx11::filesystem_error& e) {
        std::cerr << "File not found" << std::endl;
    }
    std::chrono::time_point<std::filesystem::__file_clock> lmod;
    try {
        lmod = std::filesystem::last_write_time(filename);
    } catch (std::filesystem::filesystem_error& e) {
        std::cerr << e.what() << std::endl;
        return;
    }

    ifstream file(filename, ios::binary);
    if (!file)
    {
        return;
    }

    cout << filename << endl;
    // primeiro, envia o nome e outros metadados (se precisar) do arquivo
    Packet metadata_packet {
            communication::UPLOAD,
            1,
            filesize,
            (unsigned int) strlen(filename.c_str()),
            (char*) filename.c_str()
    };
    try {
        transmitter->sendPacket(metadata_packet);
    } catch (SocketWriteError& e) {
        cerr << e.what() << endl;
    }

    communication::Packet metadata_packet2 {
            communication::DOWNLOAD,
            2,
            filesize,
            sizeof(std::chrono::time_point<std::filesystem::__file_clock>),
            (char*) &lmod
    };
    try {
        transmitter->sendPacket(metadata_packet2);
    } catch (SocketWriteError& e) {
        std::cerr << e.what() << std::endl;
    }

    // depois, envia o arquivo em partes de 1023 Bytes
    const unsigned int BUF_SIZE = 1023;
    char buf[BUF_SIZE] = {};
    unsigned int i = 2;
    while(!file.eof())
    {
        bzero(buf, BUF_SIZE);
        file.read(buf, BUF_SIZE);
        Packet data_packet {
                communication::UPLOAD,
                i,
                filesize,
                (unsigned int) BUF_SIZE,
                buf
        };

        try {
            transmitter->sendPacket(data_packet);
        } catch (SocketWriteError& e) {
            cerr << e.what() << endl;
            break;
        }

        i++;
    }

    try {
        transmitter->sendPacket(communication::SUCCESS);
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
        file_manager->copyFile(sync_file_path, filename);
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
        transmitter->sendPacket(packet);
    } catch (SocketWriteError& e) {
        cerr << e.what() << endl;
    }
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
        transmitter->sendPacket(packet);
    } catch (SocketWriteError& e) {
        cerr << e.what() << endl;
    }

    while(packet.command != communication::OK) {
        try {
            packet = transmitter->receivePacket();
            string filename = packet._payload;

            if (packet.command == communication::DOWNLOAD)
            {
                packet = transmitter->receivePacket();
                auto* last_write = (std::chrono::time_point<std::filesystem::__file_clock>*) packet._payload;

                getSyncFile(filename);
            }
        } catch (SocketReadError& e) {
            cout << e.what() << endl;
        }
    }
}

void CommandHandler::deleteOldFiles() {
    // Get server files
    string message = "show me your files.";
    Packet packet {
            communication::LIST_SERVER,
            1,
            message.size(),
            (unsigned int) strlen(message.c_str()),
            (char*) message.c_str()
    };

    try {
        transmitter->sendPacket(packet);
    } catch (SocketWriteError& e) {
        cerr << e.what() << endl;
    }

    try {
        auto response = transmitter->receivePacket();
        string files_list_raw{response._payload};
        auto server_files = split_str(files_list_raw, ',');

        auto user_files_raw = file_manager->listFiles();
        auto user_files = split_str(files_list_raw, ',');

        for (const auto& file : user_files)
        {
            bool in_server = false;
            for (const auto& sync_file : server_files)
            {
                if (file == sync_file)
                {
                    in_server = true;
                    break;
                }
            }
            if (!in_server)
            {
                deleteFile(file);
            }
        }
    } catch (SocketReadError& e) {
        cerr << e.what() << endl;
    }
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
        transmitter->sendPacket(packet);
    } catch (SocketWriteError& e) {
        cerr << e.what() << endl;
    }

    try {
        auto response = transmitter->receivePacket();
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
            auto packet = transmitter->receivePacket();
            last_command = packet.command;
            if (last_command == communication::DOWNLOAD)
            {
                tmp_file.write((char*)packet._payload, packet.length);
            }
        } catch (SocketReadError& e) {
            std::cerr << e.what() << std::endl;
            break;
        }
    }
}

void CommandHandler::watchFiles() {
    const uint16_t MAX_EVENTS = 512;
    const uint16_t LEN_NAME = 32;  /* Assuming that the length of the filename won't exceed 16 bytes*/
    const size_t EVENT_SIZE  = (sizeof (struct inotify_event)); /*size of one event*/
    const size_t BUF_LEN = ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME ));

    watchfd = inotify_init();

    int watch_descriptor = inotify_add_watch(watchfd, "sync_dir",
                                             IN_CREATE | IN_MODIFY | IN_MOVED_TO | IN_DELETE);

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
            if(event->len) {
                if (event->mask & IN_CREATE || event->mask & IN_MOVED_TO) {
                    if (event->mask & IN_ISDIR) {
                        printf("The directory %s was created/moved/modified. Will not sync\n", event->name);
                    } else {
                        string file_path = file_manager->getPath();
                        file_path += "/";
                        file_path.append(event->name);

                        if (file_path[file_path.size() - 1] == '~')
                            continue;
                        printf("File %s was created/moved/modified. sync\n", file_path.c_str());

                        in_use_semaphore->acquire();
                        uploadFile(file_path.c_str());
                        in_use_semaphore->release();
                    }
                } else if (event->mask & IN_DELETE) {
                    if (event->mask & IN_ISDIR) {
                        printf("The directory %s was deleted. Will not sync\n", event->name);
                    } else {
                        string filename = event->name;
                        if (filename[filename.size() - 1] == '~')
                            continue;
                        printf("%s was deleted. sync\n", event->name);
                        in_use_semaphore->acquire();
                        deleteFile(filename.c_str());
                        in_use_semaphore->release();
                    }
                }
                i += EVENT_SIZE + event->len;
            }
        }
    }
}

void CommandHandler::listenCommands() {
    while(true) {
        sleep(30);
        if (file_manager != nullptr ) {
            in_use_semaphore->acquire();
            getSyncDir();
            deleteOldFiles();
            in_use_semaphore->release();
        }
    }
}
