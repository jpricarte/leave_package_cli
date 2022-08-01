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
    std::stringstream s(str);

    std::string s2;
    std::vector <std::string> out{};
    while (std:: getline (s, s2, delim) )
    {
        out.push_back(s2); // store the string in s2
    }
    return out;
}

Command parseCommand(std::string s) {
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


CommandHandler::CommandHandler(Transmitter *transmitter) : transmitter(transmitter) {}

void CommandHandler::uploadFile(const string& filename)
{
    // TODO: abrir arquivo, se abrir bonitinho, manda pro servidor,
    string mock_filename = "fakefile.txt";
    string mock_file = "I'm a file, please download me!";

    // primeiro, envia o nome e outros metadados (se precisar) do arquivo
    Packet metadata_packet {
            communication::UPLOAD,
            1,
            mock_filename.size(),
            (unsigned int) mock_filename.size(),
            (char*) mock_filename.c_str()
    };

    // depois, envia o arquivo
    Packet data_packet {
            communication::UPLOAD,
            2,
            mock_file.size(),
            (unsigned int) mock_file.size(),
            (char*) mock_file.c_str()
    };

    try {
        transmitter->sendPackage(metadata_packet);
        transmitter->sendPackage(data_packet);
    } catch (SocketWriteError& e) {
        cerr << e.what() << endl;
    }
    cerr << "not implemented" << endl;
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
    //TODO: receber lista de arquivos (talvez em csv?)
}

void CommandHandler::listClient()
{
    // TODO: listar arquivos associados a pasta sync_dir
    cerr << "not implemented" << endl;
}

void CommandHandler::handleCommand(const Command& command, const vector<string>& args) {

    if ( (command == UPLOAD || command == DOWNLOAD || command == DELETE) && args.size() != 2 )
        throw InvalidNumOfArgs();

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

void CommandHandler::handle()
{
    communication::Command last_command = communication::NOP;
    while(last_command != communication::EXIT)
    {
        string input{};
        cout << ">> ";
        cin >> input;
        cout << endl;
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

CommandHandler::~CommandHandler() {
    delete transmitter;
}
