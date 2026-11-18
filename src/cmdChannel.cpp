#include "cmdChannel.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "utils.hpp"
#include "replies.hpp"


void	cmdJoin(Server &server, Client &client, const Message& msg)
{
	if (msg.params.size() == 0)
	{
		sendNumeric(server, client, ERR_NEEDMOREPARAMS, "JOIN");
		return;
	}
	if (!server.isValidName(msg.params[0]))
	{
		sendNumeric(server, client, ERR_NOSUCHCHANNEL, msg.params[0]);
		return;
	}
	
	Channel *channel;
	channel = server.findChannel(msg.params[0]);

	if (channel == NULL)
	{
		channel = new Channel(msg.params[0]);
		try
		{
			channel->insertClient(&client);
			channel->insertOperator(&client);
			server.insertMap(channel);
		}
		catch(...)
		{
			delete channel;
			throw;
		}
	}
	else
	{
		if (channel->hasMember(&client))
		{
			sendNumeric(server, client, ERR_USERONCHANNEL, channel->getName());
			return;
		}
		if (channel->isInviteOnly() && !channel->hasGuest(&client))
		{
			sendNumeric(server, client, ERR_INVITEONLYCHAN, channel->getName());
			return;
		}
		if ((msg.params.size() == 1 && !channel->acceptsKey(""))
			|| (msg.params.size() > 1 && !channel->acceptsKey(msg.params[1])))
		{
			sendNumeric(server, client, ERR_BADCHANNELKEY, channel->getName());
			return;
		}
		if (channel->isFull())
		{
			sendNumeric(server, client, ERR_CHANNELISFULL, channel->getName());
			return;
		}
		channel->insertClient(&client);
	}
	channel->broadcast(":" + client.prefix() + " JOIN " + channel->getName() + "\r\n", NULL);
	if (channel->getTopic() != "")
		sendNumeric(server, client, RPL_TOPIC, channel->getName(), channel->getTopic());
	else
		sendNumeric(server, client, RPL_NOTOPIC, channel->getName());
	sendNumeric(server, client, RPL_NAMREPLY, "= " + channel->getName(), channel->membersList());
	sendNumeric(server, client, RPL_ENDOFNAMES, channel->getName());
}

void	cmdPart(Server &server, Client &client, const Message &msg)
{
	if (msg.params.size() == 0)
	{
		sendNumeric(server, client, ERR_NEEDMOREPARAMS, "PART");
		return;
	}

	Channel *channel;
	channel = server.findChannel(msg.params[0]);
	if (channel == NULL)
	{
		sendNumeric(server, client, ERR_NOSUCHCHANNEL, msg.params[0]);
		return;
	}
	if (!channel->hasMember(&client))
	{
		sendNumeric(server, client, ERR_NOTONCHANNEL, channel->getName());
		return;
	}

	std::string line = ":" + client.prefix() + " PART " + channel->getName();
	if (msg.params.size() > 1)
		line += " :" + msg.params[1];
	line += "\r\n";
	channel->broadcast(line, NULL);
	
	channel->removeClient(&client);
	if (channel->isEmpty())
		server.requestDeleteChannel(channel->getName());
	else
		channel->electOperator(server.getName());
}

void	cmdKick(Server &server, Client &client, const Message &msg)
{
	if (msg.params.size() < 2)
	{
		sendNumeric(server, client, ERR_NEEDMOREPARAMS, "KICK");
		return;
	}

	Channel *channel;
	channel = server.findChannel(msg.params[0]);
	if (channel == NULL)
	{
		sendNumeric(server, client, ERR_NOSUCHCHANNEL, msg.params[0]);
		return;
	}
	if (!channel->hasMember(&client))
	{
		sendNumeric(server, client, ERR_NOTONCHANNEL, channel->getName());
		return;
	}
	if (!channel->hasOperator(&client))
	{
		sendNumeric(server, client, ERR_CHANOPRIVSNEEDED, channel->getName());
		return;
	}

	Client *target;
	target = server.findClient(msg.params[1]);
	if (target == NULL)
	{
		sendNumeric(server, client, ERR_NOSUCHNICK, msg.params[1]);
		return;
	}
	if (!channel->hasMember(target))
	{
		sendNumeric(server, client, ERR_USERNOTINCHANNEL, target->getNick() + " " + channel->getName());
		return;
	}

	std::string line = ":" + client.prefix() + " KICK " + channel->getName() + " " + target->getNick();
	if (msg.params.size() > 2)
		line += " :" + msg.params[2];
	line += "\r\n";
	channel->broadcast(line, NULL);
	
	channel->removeClient(target);
	if (channel->isEmpty())
		server.requestDeleteChannel(channel->getName());
	else
		channel->electOperator(server.getName());
}

void	cmdInvite(Server &server, Client &client, const Message &msg)
{
	if (msg.params.size() < 2)
	{
		sendNumeric(server, client, ERR_NEEDMOREPARAMS, "INVITE");
		return;
	}

	Channel *channel;
	channel = server.findChannel(msg.params[1]);
	if (channel == NULL)
	{
		sendNumeric(server, client, ERR_NOSUCHCHANNEL, msg.params[1]);
		return;
	}
	if (!channel->hasMember(&client))
	{
		sendNumeric(server, client, ERR_NOTONCHANNEL, channel->getName());
		return;
	}
	if (!channel->hasOperator(&client))
	{
		sendNumeric(server, client, ERR_CHANOPRIVSNEEDED, channel->getName());
		return;
	}

	Client *target;
	target = server.findClient(msg.params[0]);
	if (target == NULL)
	{
		sendNumeric(server, client, ERR_NOSUCHNICK, msg.params[0]);
		return;
	}
	if (!channel->insertGuest(target))
	{
		sendNumeric(server, client, ERR_USERONCHANNEL, target->getNick() + " " + channel->getName());
		return;
	}

	sendNumeric(server, client, RPL_INVITING, target->getNick() + " " + channel->getName());
	target->queue(":" + client.prefix() + " INVITE " + target->getNick() + " " + channel->getName() + "\r\n");
}

// cmdPrivmsg takes two parameters because handlePrivmsg validates them beforehand
void	cmdPrivmsg(Server &server, Client &client, const Message &msg)
{
	Channel *channel;
	channel = server.findChannel(msg.params[0]);
	if (channel == NULL)
	{
		sendNumeric(server, client, ERR_NOSUCHCHANNEL, msg.params[0]);
		return;
	}
	if (!channel->hasMember(&client))
	{
		sendNumeric(server, client, ERR_CANNOTSENDTOCHAN, channel->getName());
		return;
	}

	std::string line = ":" + client.prefix() + " PRIVMSG " + channel->getName();
	line += " :" + msg.params[1];
	line += "\r\n";
	channel->broadcast(line, &client);
}

void	cmdTopic(Server &server, Client &client, const Message &msg)
{
	if (msg.params.size() == 0)
	{
		sendNumeric(server, client, ERR_NEEDMOREPARAMS, "TOPIC");
		return;
	}

	Channel *channel;
	channel = server.findChannel(msg.params[0]);
	if (channel == NULL)
	{
		sendNumeric(server, client, ERR_NOSUCHCHANNEL, msg.params[0]);
		return;
	}
	if (!channel->hasMember(&client))
	{
		sendNumeric(server, client, ERR_NOTONCHANNEL, channel->getName());
		return;
	}

	// if -> only check ; else -> change topic
	if (msg.params.size() == 1)
	{
		if (channel->getTopic() == "")
			sendNumeric(server, client, RPL_NOTOPIC, channel->getName());
		else
			sendNumeric(server, client, RPL_TOPIC, channel->getName(), channel->getTopic());
	}
	else
	{
		if (channel->isTopicRestricted() && !channel->hasOperator(&client))
		{
			sendNumeric(server, client, ERR_CHANOPRIVSNEEDED, channel->getName());
			return;
		}
		channel->changeTopic(msg.params[1]);
		std::string line = ":" + client.prefix() + " TOPIC " + channel->getName() + " :" + channel->getTopic();
		line += "\r\n";
		channel->broadcast(line, NULL);
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
		sendNumeric(server, client, ERR_NEEDMOREPARAMS, "MODE");
		return ("");
	}

	Client *target;
	target = server.findClient(msg.params[2]);
	if (target == NULL)
	{
		sendNumeric(server, client, ERR_NOSUCHNICK, msg.params[2]);
		return ("");
	}
	if (!channel.hasMember(target))
	{
		sendNumeric(server, client, ERR_USERNOTINCHANNEL, target->getNick() + " " + channel.getName());
		return ("");
	}
	if (channel.hasOperator(target) == enable)
		return ("");

	std::string mode;
	if (enable)
		mode = "+o";
	else
		mode = "-o";
	mode += " " + target->getNick();

	if (enable)
		channel.insertOperator(target);
	else
		channel.removeOperator(target);
	return (mode);
}

static std::string	modeKey(Server &server, Client &client, const Message &msg, Channel &channel, bool enable)
{
	std::string mode;
	if (enable)
	{
		if (msg.params.size() < 3)
		{
			sendNumeric(server, client, ERR_NEEDMOREPARAMS, "MODE");
			return ("");
		}
		if (channel.hasKey() && channel.acceptsKey(msg.params[2]))
			return ("");
		if (!channel.enableKey(msg.params[2]))
		{
			sendNumeric(server, client, ERR_INVALIDMODEPARAM, channel.getName());
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
			sendNumeric(server, client, ERR_NEEDMOREPARAMS, "MODE");
			return ("");
		}
		size_t newLimit = 0;
		if (!allDigits(msg.params[2]) || !toSize_t(msg.params[2], newLimit))
		{
			sendNumeric(server, client, ERR_INVALIDMODEPARAM, channel.getName());
			return ("");
		}
		if (channel.hasLimit() && newLimit == channel.getLimit())
			return ("");
		if (!channel.enableLimit(newLimit))
		{
			sendNumeric(server, client, ERR_INVALIDMODEPARAM, channel.getName());
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
		sendNumeric(server, client, ERR_NEEDMOREPARAMS, "MODE");
		return;
	}

	Channel *channel;
	channel = server.findChannel(msg.params[0]);
	if (channel == NULL)
	{
		sendNumeric(server, client, ERR_NOSUCHCHANNEL, msg.params[0]);
		return;
	}
	if (!channel->hasMember(&client))
	{
		sendNumeric(server, client, ERR_NOTONCHANNEL, channel->getName());
		return;
	}

	// if -> only check ; else -> change modes
	if (msg.params.size() == 1)
		sendNumeric(server, client, RPL_CHANNELMODEIS, channel->getName() + " " + channel->getModes());
	else
	{
		if (!channel->hasOperator(&client))
		{
			sendNumeric(server, client, ERR_CHANOPRIVSNEEDED, channel->getName());
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
			sendNumeric(server, client, ERR_UNKNOWNMODE, msg.params[1]);
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
