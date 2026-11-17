#ifndef COMMANDHANDLER_HPP
# define COMMANDHANDLER_HPP

class	Server;
class	Client;
struct	Message;

void	executeCommand(Server& server, Client& client, const Message& msg);

#endif