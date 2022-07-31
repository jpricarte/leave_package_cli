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

void uploadFile(const string& filename)
{
    cerr << "not implemented" << endl;
}

void downloadFile(const string& filename)
{
    cerr << "not implemented" << endl;
}

void deleteFile(const string& filename)
{
    cerr << "not implemented" << endl;
}

void getSyncDir()
{
    cerr << "not implemented" << endl;
}

void listServer()
{
    cerr << "not implemented" << endl;
}

void listClient()
{
    cerr << "not implemented" << endl;
}

void handleCommand(const Command& command, const vector<string>& args) {

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
        default:
            break;
    }
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

void commandHandler(communication::Transmitter* transmitter)
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
        handleCommand(last_command, args);
    }

    delete transmitter;
}
