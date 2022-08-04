//
// Created by jpricarte on 31/07/22.
//

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
    }

    string end_connection = "goodbye my friend!";
    Packet finishConnectionPacket{
            EXIT,
            1,
            end_connection.size(),
            (unsigned int) end_connection.size(),
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
    filesystem::path fpath(file_path);
    string filename = fpath.filename();
    string content = FileManager::readUnwatchedFile(file_path);
    string payload = filename + "\n" + content;

    // primeiro, envia o nome e outros metadados (se precisar) do arquivo
//    Packet metadata_packet {
//            communication::UPLOAD,
//            1,
//            filename.size(),
//            (unsigned int) filename.size(),
//            (char*) filename.c_str()
//    };
//
//    // depois, envia o arquivo
//    Packet data_packet {
//            communication::UPLOAD,
//            2,
//            content.size(),
//            (unsigned int) content.size(),
//            (char*) content.c_str()
//    };

    Packet data_packet {
            communication::UPLOAD,
            1,
            payload.size(),
            (unsigned int) payload.size(),
            (char*) payload.c_str()
    };

    try {
        transmitter->sendPackage(data_packet);
//        transmitter->sendPackage(metadata_packet);
//        transmitter->sendPackage(data_packet);
    } catch (SocketWriteError& e) {
        cerr << e.what() << endl;
    }

}

void CommandHandler::downloadFile(const string& filename)
{
    Packet packet {
        communication::DOWNLOAD,
        1,
        filename.size(),
        (unsigned int) filename.size(),
        (char*) filename.c_str()
    };
    try {
        transmitter->sendPackage(packet);
    } catch (SocketWriteError& e) {
        cerr << e.what() << endl;
    }

    //TODO: receber arquivo (outra thread?)
}

void CommandHandler::deleteFile(const string& filename)
{
    Packet packet {
            communication::DELETE,
            1,
            filename.size(),
            (unsigned int) filename.size(),
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
    string message = "begging for files.";
    Packet packet {
            communication::GET_SYNC_DIR,
            1,
            message.size(),
            (unsigned int) message.size(),
            (char*) message.c_str()
    };
    try {
        transmitter->sendPackage(packet);
    } catch (SocketWriteError& e) {
        cerr << e.what() << endl;
    }
    //TODO: receber os arquivos em ordem
}

void CommandHandler::listServer()
{
    string message = "show me your files.";
    Packet packet {
            communication::LIST_SERVER,
            1,
            message.size(),
            (unsigned int) message.size(),
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
