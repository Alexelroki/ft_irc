#include "Server.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: ./ircserv <port> <password>\n";
        return (1);
    }

    // Set signal handling for clean exit on Ctrl+C (SIGINT) or Ctrl+\ (SIGQUIT)
    signal(SIGINT, Server::signalHandler);
    signal(SIGQUIT, Server::signalHandler);

    try
    {
        Server	server(argv[1], argv[2]);
        server.start();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return (1);
    }

    return (0);
}
