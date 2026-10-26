#include "cmdChannel.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
// #include "Message.hpp"
#include <new>


void	cmdJoin(Server &server, Client &client, const Message& msg)
{
	if (msg.params.size() == 0)
	{
		client.queue("461"); // B revisar este metodo de Client
		return ;
	}
	Channel *channel = new Channel(msg.params[0]); // B como recibo params?
	try
	{
		channel->insertClient(&client);
		channel->insertOperator(&client);
		server.insertMap(channel);			// A como registro el canal en map de server
	}
	catch(const std::exception& e)
	{
		delete channel;
		throw;
	}
}

Message { prefix, command, params[] }