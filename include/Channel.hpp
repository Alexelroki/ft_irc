#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <string>
# include <set>
# include <cstddef>

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
		size_t				_limit;

		Channel();
		Channel(const Channel &other);
		Channel &operator=(const Channel &other);

	public:
		Channel(const std::string &name);
		~Channel();

		bool				hasMember(Client *client) const;
		void				broadcast(const std::string &msg, const Client *sender) const;
		void				insertClient(Client *client);
		void				removeClient(Client *client);
		bool				isEmpty() const;
		bool				hasOperator(Client *client) const;
		bool				insertOperator(Client *client);
		void				removeOperator(Client *client);
		bool				insertGuest(Client *client);
		bool				hasGuest(Client *client) const;
		const std::string	&getTopic() const;
		void				changeTopic(const std::string &topic);
		bool				isInviteOnly() const;
		void				changeInviteOnly();
		bool				isTopicRestricted() const;
		void				changeTopicRestricted(bool mode);
		bool				acceptsKey(const std::string &key) const;
		bool				enableKey(const std::string &key);
		void				disableKey();
		bool				isFull() const;
		bool				enableLimit(size_t limit);
		void				disableLimit();
};

#endif