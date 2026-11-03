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
		if (msg.params[1] == "+i")
		{
			if (!channel->isInviteOnly())
			{
				channel->changeInviteOnly(true);
				mode = "+i";
			}
		}
		else if (msg.params[1] == "-i")
		{
			if (channel->isInviteOnly())
			{
				channel->changeInviteOnly(false);
				mode = "-i";
			}
		}
		else if (msg.params[1] == "+t")
		{
			if (!channel->isTopicRestricted())
			{
				channel->changeTopicRestricted(true);
				mode = "+t";
			}
		}
		else if (msg.params[1] == "-t")
		{
			if (channel->isTopicRestricted())
			{
				channel->changeTopicRestricted(false);
				mode = "-t";
			}
		}
		else if (msg.params[1] == "+o" || msg.params[1] == "-o")
		{
			if (msg.params.size() == 2)
			{
				reply(server, client, "461", "MODE");
				return;
			}

			Client *target;
			target = server.findClient(msg.params[2]);
			if (target == NULL)
			{
				reply(server, client, "401", msg.params[2]);
				return;
			}
			if (!channel->hasMember(target))
			{
				reply(server, client, "441", target->getNick() + " " + channel->getName());
				return;
			}

			if (msg.params[1] == "+o" && !channel->hasOperator(target))
			{
				channel->insertOperator(target);
				mode = "+o";
				mode += " " + target->getNick();
			}
			else if (msg.params[1] == "-o" && channel->hasOperator(target))
			{
				channel->removeOperator(target);
				mode = "-o";
				mode += " " + target->getNick();
			}
		}
		else if (msg.params[1] == "+k")
		{
			if (msg.params.size() == 2)
			{
				reply(server, client, "461", "MODE");
				return;
			}
			if (!channel->hasKey() || !channel->acceptsKey(msg.params[2]))
			{
				if (!channel->enableKey(msg.params[2]))
				{
					reply(server, client, "475", channel->getName());
					return;
				}		
				mode = "+k";
				mode += " " + msg.params[2];
			}
		}
		else if (msg.params[1] == "-k")
		{
			if (channel->hasKey())
			{
				channel->disableKey();
				mode = "-k";
			}
		}
		else if (msg.params[1] == "+l")
		{
			if (msg.params.size() == 2)
			{
				reply(server, client, "461", "MODE");
				return;
			}

			if (!allDigits(msg.params[2]))
			{
				reply(server, client, "400", channel->getName());
				return;
			}

			size_t newLimit;
			if (!toSize_t(msg.params[2], newLimit))
			{
				reply(server, client, "400", channel->getName());
				return;
			}
			if (!channel->hasLimit() || newLimit != channel->getLimit())
			{
				if (!channel->enableLimit(newLimit))
				{
					reply(server, client, "400", channel->getName());
					return;
				}
				mode = "+l";
				mode += " " + toString(newLimit);
			}
		}
		else if (msg.params[1] == "-l")
		{
			if (channel->hasLimit())
			{
				channel->disableLimit();
				mode = "-l";
			}
		}
		else
		{
			reply(server, client, "472", channel->getName(), msg.params[1]);
			return;
		}

		if (mode != "")
		{
			std::string line = ":" + client.prefix() + " MODE " + channel->getName() + " " + mode;
			line += "\r\n";
			channel->broadcast(line, NULL);
		}
	}
}