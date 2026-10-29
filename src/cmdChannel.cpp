#include "cmdChannel.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
// #include "Message.hpp"
#include <new>

static void reply(Server &server, Client &client, const std::string &code, const std::string &params, const std::string &trailing = "") // B revisar queue para encolar mensaje y getNick
{
	if (code == "331")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " " + params + " :No topic is set\r\n");
	else if (code == "332")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " " + params + " :" + trailing + "\r\n");
	else if (code == "353")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " = " + params + " :" + trailing + "\r\n");
	else if (code == "366")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " " + params + " :End of /NAMES list\r\n");
	else if (code == "401")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " " + params + " :No such target\r\n");
	else if (code == "403")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " " + params + " :No such channel\r\n");
	else if (code == "404")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " " + params + " :Cannot send to channel\r\n");
	else if (code == "411")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " :No recipient given (PRIVMSG)\r\n");
	else if (code == "412")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " :No text to send\r\n");
	else if (code == "441")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " " + params + " :target is not on channel\r\n");
	else if (code == "442")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " " + params + " :Is not on channel\r\n");
	else if (code == "443")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " " + params + " :Is already on channel\r\n");
	else if (code == "461")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " " + params + " :Not enough parameters\r\n");
	else if (code == "471")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " " + params + " :Channel is full (+l)\r\n");
	else if (code == "473")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " " + params + " :Cannot join channel (+i)\r\n");
	else if (code == "475")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " " + params + " :Incorrect password (+k)\r\n");
	else if (code == "482")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " " + params + " :Is not a channel operator\r\n");
	else
		client.queue(":" + server.getName() + " 400 " + client.getNick() + " :Unknown error\r\n");
}

void	cmdJoin(Server &server, Client &client, const Message& msg)
{
	if (msg.params.size() == 0)
	{
		reply(server, client, "461", "JOIN");
		return;
	}
	if (!isValidName(msg.params[0])) // revisar clase Message
	{
		reply(server, client, "403", msg.params[0]);
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
			reply(server, client, "443", channel->getName());
			return;
		}
		if (channel->isInviteOnly() && !channel->hasGuest(&client))
		{
			reply(server, client, "473", channel->getName());
			return;
		}
		if ((msg.params.size() == 1 && !channel->acceptsKey(""))
			|| (msg.params.size() > 1 && !channel->acceptsKey(msg.params[1]))) // B como recibo params?
		{
			reply(server, client, "475", channel->getName());
			return;
		}
		if (channel->isFull())
		{
			reply(server, client, "471", channel->getName());
			return;
		}
		channel->insertClient(&client);
	}
	channel->broadcast(":" + client.prefix() + " JOIN " + channel->getName() + "\r\n", NULL); // B método prefix() de Client (devuelve nick!user@host) revisar si lleva : y si devuelve un std::string
	if (channel->getTopic() != "")
		reply(server, client, "332", channel->getName(), channel->getTopic());
	else
		reply(server, client, "331", channel->getName());
	reply(server, client, "353", channel->getName(), channel->membersList());
	reply(server, client, "366", channel->getName());
}

void	cmdPart(Server &server, Client &client, const Message &msg)
{
	if (msg.params.size() == 0)
	{
		reply(server, client, "461", "PART");
		return;
	}
	if (!isValidName(msg.params[0])) // revisar clase Message
	{
		reply(server, client, "403", msg.params[0]);
		return;
	}

	Channel *channel;
	channel = server.findChannel(msg.params[0]); // A revisar metodo para encontrar un canal y clase Message
	if (channel == NULL)
	{
		reply(server, client, "403", msg.params[0]);
		return;
	}
	if (!channel->hasMember(&client))
	{
		reply(server, client, "442", channel->getName());
		return;
	}

	std::string line = ":" + client.prefix() + " PART " + channel->getName();
	if (msg.params.size() > 1)
		line += " :" + msg.params[1];
	line += "\r\n";
	channel->broadcast(line, NULL); // B método prefix() de Client (devuelve nick!user@host) revisar si lleva : y si devuelve un std::string
	
	channel->removeClient(&client);
	if (channel->isEmpty())
		server.requestDeleteChannel(channel->getName()); // A revisar que metodo implementa el server para la solicitud de borrado.
}

void	cmdKick(Server &server, Client &client, const Message &msg)
{
	if (msg.params.size() < 2)
	{
		reply(server, client, "461", "KICK");
		return;
	}
	if (!isValidName(msg.params[0])) // revisar clase Message
	{
		reply(server, client, "403", msg.params[0]);
		return;
	}

	Channel *channel;
	channel = server.findChannel(msg.params[0]); // A revisar metodo para encontrar un canal y clase Message
	if (channel == NULL)
	{
		reply(server, client, "403", msg.params[0]);
		return;
	}
	if (!channel->hasMember(&client))
	{
		reply(server, client, "442", channel->getName());
		return;
	}
	if (!channel->hasOperator(&client))
	{
		reply(server, client, "482", channel->getName());
		return;
	}

	Client *target;
	target = server.findClient(msg.params[1]); // A revisar metodo para encontrar clientes
	if (target == NULL)
	{
		reply(server, client, "401", msg.params[1]);
		return;
	}
	if (!channel->hasMember(target))
	{
		reply(server, client, "441", target->getNick() + " " + channel->getName());
		return;
	}

	std::string line = ":" + client.prefix() + " KICK " + channel->getName() + " " + target->getNick();
	if (msg.params.size() > 2)
		line += " :" + msg.params[2];
	line += "\r\n";
	channel->broadcast(line, NULL); // B método prefix() de Client (devuelve nick!user@host) revisar si lleva : y si devuelve un std::string
	
	channel->removeClient(target);
	if (channel->isEmpty())
		server.requestDeleteChannel(channel->getName()); // A revisar que metodo implementa el server para la solicitud de borrado.
}


void	cmdPrivmsg(Server &server, Client &client, const Message &msg)
{
	if (msg.params.size() < 2)
	{
		if (msg.params.size() == 0)
			reply(server, client, "411", "");
		else
			reply(server, client, "412", "");
		return;
	}
	if (!isValidName(msg.params[0])) // revisar clase Message
	{
		reply(server, client, "403", msg.params[0]);
		return;
	}

	Channel *channel;
	channel = server.findChannel(msg.params[0]); // A revisar metodo para encontrar un canal y clase Message
	if (channel == NULL)
	{
		reply(server, client, "403", msg.params[0]);
		return;
	}
	if (!channel->hasMember(&client))
	{
		reply(server, client, "404", channel->getName());
		return;
	}

	std::string line = ":" + client.prefix() + " PRIVMSG " + channel->getName(); // B método prefix() de Client
	line += " :" + msg.params[1];
	line += "\r\n";
	channel->broadcast(line, &client);
}

Message { prefix, command, params[] }