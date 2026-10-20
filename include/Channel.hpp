#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <string>
# include <set>

class Client;

class Channel
{
	private:
		std::string			_name;
		std::string			_topic;
		std::set<Client*> 	_members;
		std::set<Client*>	_operators;
		std::set<Client*>	_guests;
		bool				_inviteOnly;
		bool				_topicRestricted;
		bool				_hasKey;
		std::string			_key;
		bool				_hasLimit;
		unsigned int		_limit;

		Channel();
		Channel(const Channel &other);
		Channel &operator=(const Channel &other);

	public:
		Channel(const std::string &name);
		~Channel();

		bool	hasMember(Client *client) const;
		void	broadcast(const std::string &msg, const Client *sender) const;
		

		addMember(const Client *member);
		removeMember(const Client *member);
		findMember(const Client &member);
		changeName(const std::string name);
		changeMode(const char mode);
};

#endif