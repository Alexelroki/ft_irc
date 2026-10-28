#ifndef CMDCHANNEL_HPP
# define CMDCHANNEL_HPP

class Server;
class Client;
class Message;

void	cmdJoin(Server &server, Client &client, const Message &msg);
void	cmdPart(Server &server, Client &client, const Message &msg);
void	cmdPrivmsg(Server &server, Client &client, const Message &msg);

#endif