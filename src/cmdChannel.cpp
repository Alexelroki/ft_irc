#include "cmdChannel.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
// #include "Message.hpp"
#include "utilsChannel.hpp"
#include <new>

static void reply(Server &server, Client &client, const std::string &code, const std::string &params, const std::string &trailing = "") // B revisar queue para encolar mensaje y getNick
{
	if (code == "324")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " " + params + "\r\n");
	else if (code == "331")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " " + params + " :No topic is set\r\n");
	else if (code == "332")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " " + params + " :" + trailing + "\r\n");
	else if (code == "341")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " " + params + " :Invitation sent\r\n");
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
	else if (code == "472")
		client.queue(":" + server.getName() + " " + code + " " + client.getNick() + " " + params + " :Unknown mode\r\n");
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

void	cmdInvite(Server &server, Client &client, const Message &msg)
{
	if (msg.params.size() < 2)
	{
		reply(server, client, "461", "INVITE");
		return;
	}
	if (!isValidName(msg.params[1])) // revisar clase Message
	{
		reply(server, client, "403", msg.params[1]);
		return;
	}

	Channel *channel;
	channel = server.findChannel(msg.params[1]); // A revisar metodo para encontrar un canal y clase Message
	if (channel == NULL)
	{
		reply(server, client, "403", msg.params[1]);
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
	target = server.findClient(msg.params[0]); // A revisar metodo para encontrar clientes
	if (target == NULL)
	{
		reply(server, client, "401", msg.params[0]);
		return;
	}
	if (!channel->insertGuest(target))
	{
		reply(server, client, "443", target->getNick() + " " + channel->getName());
		return;
	}

	reply(server, client, "341", target->getNick() + " " + channel->getName());
	target->queue(":" + client.prefix() + " INVITE " + target->getNick() + " " + channel->getName() + "\r\n");
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

void	cmdTopic(Server &server, Client &client, const Message &msg)
{
	if (msg.params.size() == 0)
	{
		reply(server, client, "461", "TOPIC");
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

	if (msg.params.size() == 1) // Solo Consulta
	{
		if (channel->getTopic() == "")
			reply(server, client, "331", channel->getName());
		else
			reply(server, client, "332", channel->getName(), channel->getTopic());
	}
	else						// Modificar Topic
	{
		if (channel->isTopicRestricted() && !channel->hasOperator(&client))
		{
			reply(server, client, "482", channel->getName());
			return;
		}
		channel->changeTopic(msg.params[1]);
		std::string line = ":" + client.prefix() + " TOPIC " + channel->getName() + " :" + channel->getTopic();
		line += "\r\n";
		channel->broadcast(line, NULL); // B método prefix() de Client (devuelve nick!user@host) revisar si lleva : y si devuelve un std::string
	}
}

static std::string	modeInvite(Channel &channel, bool enable)
{
	if (channel.isInviteOnly() == enable)
		return ("");
	channel.changeInviteOnly(enable);
	if (enable)
		return ("+i");
	else
		return ("-i");
}

static std::string	modeTopic(Channel &channel, bool enable)
{
	if (channel.isTopicRestricted() == enable)
		return ("");
	channel.changeTopicRestricted(enable);
	if (enable)
		return ("+t");
	else
		return ("-t");
}

static std::string	modeOperator(Server &server, Client &client, const Message &msg, Channel &channel, bool enable)
{
	if (msg.params.size() < 3)
	{
		reply(server, client, "461", "MODE");
		return ("");
	}

	Client *target;
	target = server.findClient(msg.params[2]);
	if (target == NULL)
	{
		reply(server, client, "401", msg.params[2]);
		return ("");
	}
	if (!channel.hasMember(target))
	{
		reply(server, client, "441", target->getNick() + " " + channel.getName());
		return ("");
	}
	if (channel.hasOperator(target) == enable)
		return ("");

	std::string mode;
	if (enable)
	{
		channel.insertOperator(target);
		mode = "+o";
		mode += " " + target->getNick();
	}
	else
	{
		channel.removeOperator(target);
		mode = "-o";
		mode += " " + target->getNick();
	}
	return (mode);
}

static std::string	modeKey(Server &server, Client &client, const Message &msg, Channel &channel, bool enable)
{
	std::string mode;
	if (enable)
	{
		if (msg.params.size() < 3)
		{
			reply(server, client, "461", "MODE");
			return ("");
		}
		if (channel.hasKey() && channel.acceptsKey(msg.params[2]))
			return ("");
		if (!channel.enableKey(msg.params[2]))
		{
			reply(server, client, "475", channel.getName());
			return ("");
		}		
		mode = "+k";
		mode += " " + msg.params[2];
	}
	else
	{
		if (channel.hasKey())
		{
			channel.disableKey();
			mode = "-k";
		}
	}
	return (mode);
}

static std::string	modeLimit(Server &server, Client &client, const Message &msg, Channel &channel, bool enable)
{
	std::string mode;

	if (enable)
	{
		if (msg.params.size() < 3)
		{
			reply(server, client, "461", "MODE");
			return ("");
		}
		size_t newLimit = 0;
		if (!allDigits(msg.params[2]) || !toSize_t(msg.params[2], newLimit))
		{
			reply(server, client, "400", channel.getName());
			return ("");
		}
		if (channel.hasLimit() && newLimit == channel.getLimit())
			return ("");
		if (!channel.enableLimit(newLimit))
		{
			reply(server, client, "400", channel.getName());
			return ("");
		}
		mode = "+l";
		mode += " " + toString(newLimit);
	}
	else
	{
		if (channel.hasLimit())
		{
			channel.disableLimit();
			mode = "-l";
		}
	}
	return (mode);
}
void	cmdMode(Server &server, Client &client, const Message &msg)
{
	if (msg.params.size() == 0)
	{
		reply(server, client, "461", "MODE");
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

	if (msg.params.size() == 1) // Solo Consulta
		reply(server, client, "324", channel->getName() + " " + channel->getModes());
	else						// Modificar Modos
	{
		if (!channel->hasOperator(&client))
		{
			reply(server, client, "482", channel->getName());
			return;
		}

		std::string mode;
		if (msg.params[1] == "+i" || msg.params[1] == "-i")
			mode = modeInvite(*channel, msg.params[1] == "+i");
		else if (msg.params[1] == "+t" || msg.params[1] == "-t")
			mode = modeTopic(*channel, msg.params[1] == "+t");
		else if (msg.params[1] == "+o" || msg.params[1] == "-o")
			mode = modeOperator(server, client, msg, *channel, msg.params[1] == "+o");
		else if (msg.params[1] == "+k" || msg.params[1] == "-k")
			mode = modeKey(server, client, msg, *channel, msg.params[1] == "+k");
		else if (msg.params[1] == "+l" || msg.params[1] == "-l")
			mode = modeLimit(server, client, msg, *channel, msg.params[1] == "+l");
		else
		{
			reply(server, client, "472", channel->getName());
			return;
		}

		if (!mode.empty())
		{
			std::string line = ":" + client.prefix() + " MODE " + channel->getName() + " " + mode;
			line += "\r\n";
			channel->broadcast(line, NULL);
		}
	}
}

// Message { prefix, command, params[] }