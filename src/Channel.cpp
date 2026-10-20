#include "Channel.hpp"
#include "Client.hpp"

Channel::Channel(const std::string &name)
	: _name(name), _inviteOnly(false), _topicRestricted(false), _hasKey(false), _hasLimit(false), _limit(0)
	{}

Channel::~Channel() {}

bool	Channel::hasMember(Client *client) const
{
	if (_members.find(client) != _members.end())
		return (true);
	else
		return (false);
}

void	Channel::broadcast(const std::string &msg, const Client *sender) const
{
	std::set<Client*>::const_iterator it;

	it = _members.begin();
	while (it != _members.end())
	{
		if (*it != sender)
			(*it)->queue(msg);
		it++;
	}
}
