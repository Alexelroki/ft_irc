#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <vector>
# include <map>
# include <set>
# include <iostream>
# include <stdexcept>
# include <cstdlib>
# include <cerrno>
# include <cstring>

# include <unistd.h>
# include <fcntl.h>
# include <poll.h>
# include <signal.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include "Message.hpp"
# include "Client.hpp"
# include "commandHandler.hpp"

# define SERVER_NAME "ft_irc"

class Channel;

class Server
{
	public:
		// Orthodox Canonical Form
		Server(const std::string& portStr, const std::string& passStr);
		Server(const Server& other);
		Server&							operator=(const Server& other);
		~Server();

		void							start();
		static void						signalHandler(int sigNum);

		// Getters
		int								getPort() const;
		const std::string&				getPassword() const;
		std::string						getName() const;

		// Channel and Client management
		Channel*						findChannel(const std::string& name);
		Client*							findClient(const std::string& nick);
		Client*							findClientByNick(const std::string& nick);
		bool							isNickInUse(const std::string& nick, const Client* exclude = NULL) const;
		void							insertMap(Channel* channel);
		void							requestDeleteChannel(std::string name);
		bool							isValidName(const std::string& name) const;
		void							broadcastToUserChannels(Client& sender, const std::string& msg, bool includeSender);

	private:
		int								_port;
		std::string						_password;
		int								_serverSocketFd;
		std::vector<struct pollfd>		_pollFds;

		// Containers for Channels and Clients
		std::map<std::string, Channel*>	_channels;
		std::map<int, Client*>			_clients;

		static bool						_signalReceived;

		Server();
		void							_parseArgs(const std::string& portStr, const std::string& passStr);
		void							_initSocket();
		void							_acceptClient();
		void							_handleClientData(int clientFd, size_t pollIndex);
		void							_flushClientQueue(int clientFd, size_t pollIndex);
		void							_closeClient(int clientFd, size_t pollIndex);
		void							_cleanup();
};

#endif
