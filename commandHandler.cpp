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
    keep_running = true;
    if (filesystem::exists("sync_dir")) {
        file_manager = new FileManager("sync_dir");
    } else {
        file_manager = nullptr;
    }

    should_start_cv = new condition_variable();
}


CommandHandler::~CommandHandler() {
    delete transmitter;
}

void CommandHandler::handle()
{
    if (file_manager != nullptr)
    {
        getSyncDir();
        deleteOldFiles();
    }

    should_start_cv->notify_all();

    communication::Command last_command = communication::NOP;
    while(last_command != communication::EXIT && keep_running)
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
    cout << "handleCommand out" << endl;
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

    ifstream file(filename, ios::binary);
    if (!file)
    {
        return;
    }

    string sendable_filename = filesystem::path(filename).filename();
    // primeiro, envia o nome e outros metadados (se precisar) do arquivo
    Packet metadata_packet {
            communication::UPLOAD,
            1,
            filesize,
            (unsigned int) strlen(sendable_filename.c_str()),
            (char*) sendable_filename.c_str()
    };
    try {
        transmitter->sendPacket(metadata_packet);
    } catch (SocketWriteError& e) {
        cerr << e.what() << endl;
    }

    size_t remaining_size = filesize;
    // depois, envia o arquivo em partes de 1024 Bytes
    const unsigned int BUF_SIZE = 1024;
    char buf[BUF_SIZE] = {};
    unsigned int i = 2;
    while(remaining_size > 0)
    {
        bzero(buf, BUF_SIZE);
        auto read_size = min((unsigned long)BUF_SIZE, remaining_size);
        remaining_size -= read_size;
        file.read(buf, (unsigned int) read_size);
        Packet data_packet {
                communication::UPLOAD,
                i,
                filesize,
                (unsigned int) read_size,
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
        cout << "The File Watcher disabled, you should restart your client!" << endl;
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
                getSyncFile(filename, packet.total_size);
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
        string server_files_raw{response._payload};
        auto server_files = split_str(server_files_raw, ',');

        auto user_files_raw = file_manager->listFiles();
        if (user_files_raw == "nothing to show")
            return;
        auto user_files = split_str(user_files_raw, ',');


        if (server_files_raw != "nothing to show")
        {
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
                    file_manager->deleteFile(file);
                }
            }
        }
        else {
            for (const auto& file : user_files)
            {
                file_manager->deleteFile(file);
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

void CommandHandler::getSyncFile(const string &filename, std::size_t file_size) {
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

void CommandHandler::saveDataFlow(std::ofstream &tmp_file, std::size_t file_size) {
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

void CommandHandler::watchFiles(std::future<void> exit_signal) {
    watcher_fd = inotify_init();

    { // Wait to start listen
        mutex m{}; // useless mutex (cv only for sync), but required to cv
        unique_lock lk(m);
        should_start_cv->wait(lk);
        lk.unlock();
    }

    if (file_manager != nullptr && inotify_add_watch(watcher_fd, file_manager->getPath().c_str(),
                          IN_CREATE | IN_CLOSE_WRITE | IN_DELETE) < 0)
    {
        cerr << "error watching files " << endl << "check if you already create sync_dir" << endl;
        return;
    }

    const size_t EVENT_SIZE = sizeof(struct inotify_event);
    const size_t BUF_SIZE = 512 * EVENT_SIZE;
    char buf[BUF_SIZE] = {};

    while(file_manager != nullptr)
    {
        size_t i=0, len;

        bzero(buf, BUF_SIZE);
        len = read(watcher_fd, buf, BUF_SIZE);

        if (exit_signal.wait_for(std::chrono::milliseconds(1)) != future_status::timeout)
        { // Break if got a exit signal
            break;
        }

        vector<string> need_upload{};
        vector<string> need_update{};
        vector<string> need_delete{};
        // For each readed event
        while(i < len)
        {
            auto* event = (struct inotify_event *)&buf[i];
            if (event->len > 0)
            {
                string filename{event->name};
                if (filename[0] == '.' || filename[filename.size()-1] == '~')
                    continue;
                if ((event->mask & IN_CREATE) && !(event->mask & IN_ISDIR)) {
                    need_upload.push_back(filename);
                }
                if ((event->mask & IN_CLOSE_WRITE) && !(event->mask & IN_ISDIR)) {
                    need_update.push_back(filename);
                }
                if ((event->mask & IN_DELETE) && !(event->mask & IN_ISDIR)) {
                    need_delete.push_back(filename);
                }
            }
            i += EVENT_SIZE + event->len;
        }

        for (const auto& filename : need_upload)
        {
            std::string sync_file_path = file_manager->getPath();
            sync_file_path += "/" + filename;

            in_use_semaphore->acquire();
            uploadFile(sync_file_path);
            in_use_semaphore->release();
        }

        for (const auto& filename : need_update)
        {
            std::string sync_file_path = file_manager->getPath();
            sync_file_path += "/" + filename;


            in_use_semaphore->acquire();
            deleteFile(filename);
            uploadFile(sync_file_path);
            in_use_semaphore->release();
        }

        for (const auto& filename : need_delete)
        {
            in_use_semaphore->acquire();
            deleteFile(filename);
            in_use_semaphore->release();
        }
    }

    close(watcher_fd);
    cout << "watcher out" << endl;
}



void CommandHandler::simpleSync(std::future<void> exit_signal) {
    { // Wait to start listen
        mutex m{}; // useless mutex (cv only for sync), but required to cv
        unique_lock lk(m);
        should_start_cv->wait(lk);
        lk.unlock();
    }
    while (exit_signal.wait_for(std::chrono::seconds (30)) == future_status::timeout) {
        in_use_semaphore ->acquire();
        deleteOldFiles();
        getSyncDir();
        in_use_semaphore->release();
    }
    cout << "simpleSync out" << endl;
}


/*
    Função não utilizada, servia para a replicação mais complexa 
    explicada no relatório.
*/
//void CommandHandler::SyncDevice(std::future<void> exit_signal) {
//    { // Wait to start listen
//        mutex m{}; // useless mutex (cv only for sync), but required to cv
//        unique_lock lk(m);
//        should_start_cv->wait(lk);
//        lk.unlock();
//    }
//
//    cout << "start " << endl;
//    while (exit_signal.wait_for(std::chrono::milliseconds(1)) != future_status::timeout) {
//        Packet sync_packet = transmitter->popSyncRequest();
//        in_use_semaphore->acquire();
//        switch (sync_packet.command) {
//            case communication::SYNC_UPLOAD:
//                cout << "get a new file" << endl;
//                getSyncFile(sync_packet._payload, );
//                break;
//            case communication::SYNC_DELETE:
//                cout << "deleting a file" << endl;
//                file_manager->deleteFile(sync_packet._payload);
//        }
//        in_use_semaphore->release();
//
//    }
//}

bool CommandHandler::shouldKeepRunning() const {
    return keep_running;
}

void CommandHandler::setKeepRunning(bool keepRunning) {
    keep_running = keepRunning;
}

FileManager *CommandHandler::getFileManager() const {
    return file_manager;
}

void CommandHandler::setFileManager(FileManager *fileManager) {
    file_manager = fileManager;
}

