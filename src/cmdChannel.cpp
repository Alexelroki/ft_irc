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
		client.queue(":" + server.getName() + " 461 " + client.getNick() + " JOIN :Not enough parameters\r\n"); // B Revisar queue; Necesitamos un helper de numericos para aclarar el codigo.
		return;
	}
	if (!isValidName(msg.params[0])) // revisar clase Message
	{
		client.queue(":" + server.getName() + " 403 " + client.getNick() + " " + msg.params[0] + " :No such channel\r\n"); // B Revisar queue
		return;
	}
	
	Channel *channel;
	channel = server.findChannel(msg.params[0]); // A revisar metodo para encontrar un canal y clase Message

	if (channel == NULL)  // Si no existe
	{
		channel = new Channel(msg.params[0]); // B como recibo params?
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
	else // Canal ya existe
	{
		if (channel->hasMember(&client))
		{
			client.queue(":" + server.getName() + " 443 " + client.getNick() + " " + channel->getName() + " :Is already on channel\r\n"); // B Revisar queue y getNick
			return;
		}
		if (channel->isInviteOnly() && !channel->hasGuest(&client))
		{
			client.queue(":" + server.getName() + " 473 " + client.getNick() + " " + channel->getName() + " :Cannot join channel (+i)\r\n"); // B Revisar queue y getNick
			return;
		}
		if ((msg.params.size() == 1 && !channel->acceptsKey(""))
			|| (msg.params.size() > 1 && !channel->acceptsKey(msg.params[1]))) // B como recibo params?
		{
			client.queue(":" + server.getName() + " 475 " + client.getNick() + " " + channel->getName() + " :Incorrect password (+k)\r\n"); // B Revisar queue y getNick
			return;
		}
		if (channel->isFull())
		{
			client.queue(":" + server.getName() + " 471 " + client.getNick() + " " + channel->getName() + " :Channel is full (+l)\r\n"); // B Revisar queue y getNick
			return;
		}
		channel->insertClient(&client);
	}
	channel->broadcast(":" + client.prefix() + " JOIN " + channel->getName() + "\r\n", NULL); // B método prefix() de Client (devuelve nick!user@host) revisar si lleva : y si devuelve un std::string
	if (channel->getTopic() != "")
		client.queue(":" + server.getName() + " 332 " + client.getNick() + " " + channel->getName() + " :" + channel->getTopic() + "\r\n"); // B revisar getNick y queue
	else
		client.queue(":" + server.getName() + " 331 " + client.getNick() + " " + channel->getName() + " :No topic is set\r\n"); // B revisar getNick y queue
	client.queue(":" + server.getName() + " 353 " + client.getNick() + " = " + channel->getName() + " :" + channel->membersList() + "\r\n"); // B revisar getNick y queue
	client.queue(":" + server.getName() + " 366 " + client.getNick() + " " + channel->getName() + " :End of /NAMES list\r\n"); // B revisar getNick y queue
}

Message { prefix, command, params[] }