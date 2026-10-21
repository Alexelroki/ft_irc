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

void	Channel::insertClient(Client *client)
{
	_members.insert(client);
	_guests.erase(client);
}

void	Channel::removeClient(Client *client)
{
	_members.erase(client);
	_operators.erase(client);
	_guests.erase(client);
}

bool	Channel::isEmpty() const
{
	return (_members.empty());
}

bool	Channel::hasOperator(Client *client) const
{
	if (_operators.find(client) != _operators.end())
		return (true);
	else
		return (false);
}

bool	Channel::insertOperator(Client *client)
{
	if (!hasMember(client))
		return (false);
	_operators.insert(client);
	return (true);
}

void	Channel::removeOperator(Client *client)
{
	_operators.erase(client);
}

bool	Channel::insertGuest(Client *client)
{
	if (hasMember(client))
		return (false);
	_guests.insert(client);
	return (true);
}

bool	Channel::hasGuest(Client *client) const
{
	if (_guests.find(client) != _guests.end())
		return (true);
	else
		return (false);
}

const std::string	&Channel::getTopic() const
{
	return (_topic);
}

void	Channel::changeTopic(const std::string &topic)
{
	_topic = topic;
}

bool	Channel::isInviteOnly() const
{
	return (_inviteOnly);
}

void	Channel::changeInviteOnly(bool mode)
{
	_inviteOnly = mode;
}

bool	Channel::isTopicRestricted() const
{
	return (_topicRestricted);
}

void	Channel::changeTopicRestricted(bool mode)
{
	_topicRestricted = mode;
}

bool	Channel::acceptsKey(const std::string &key) const
{
	if (!_hasKey)
		return (true);
	if (_key == key)
		return (true);
	return (false);
}

bool	Channel::enableKey(const std::string &key)
{
	if (key.empty())
		return (false);
	_key = key;
	_hasKey = true;
	return (true);
}

void	Channel::disableKey()
{
	_hasKey = false;
	_key.clear();
}

bool	Channel::isFull() const
{
	if (_hasLimit && _members.size() >= _limit)
		return (true);
	return (false);
}

bool	Channel::enableLimit(size_t limit)
{
	if (limit == 0)
		return (false);
	_limit = limit;
	_hasLimit = true;
	return (true);
}

void	Channel::disableLimit()
{
	_hasLimit = false;
	_limit = 0;
}