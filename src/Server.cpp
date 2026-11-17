#include "Server.hpp"
#include "Channel.hpp"
#include <algorithm>

bool					Server::_signalReceived = false;

void					Server::signalHandler(int sigNum)
{
	(void)sigNum;
	Server::_signalReceived = true;
}

Server::Server()
	: _port(0), _password(""), _serverSocketFd(-1), _pollFds(), _channels(), _clients() {}

Server::Server(const std::string& portStr, const std::string& passStr)
	: _port(0), _password(""), _serverSocketFd(-1), _pollFds(), _channels(), _clients()
{
	_parseArgs(portStr, passStr);
	_initSocket();
}

void					Server::_parseArgs(const std::string& portStr, const std::string& passStr)
{
	if (portStr.empty())
		throw std::runtime_error("Error: Port cannot be empty.");

	for (size_t i = 0; i < portStr.length(); ++i)
	{
		if (!std::isdigit(portStr[i]))
			throw std::runtime_error("Error: Port must contain only numeric digits.");
	}

	errno = 0;
	char*				endPtr = NULL;
	long				portVal = std::strtol(portStr.c_str(), &endPtr, 10);

	if (errno == ERANGE || portVal < 1024 || portVal > 65535)
		throw std::runtime_error("Error: Port out of valid range (1024 - 65535).");

	if (passStr.empty())
		throw std::runtime_error("Error: Password cannot be empty.");

	this->_port = static_cast<int>(portVal);
	this->_password = passStr;
}

void					Server::_initSocket()
{
	// 1. Create socket
	this->_serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->_serverSocketFd < 0)
		throw std::runtime_error("Error: Failed to create server socket.");

	// 2. Allow quick reuse of port/address
	int					opt = 1;
	if (setsockopt(this->_serverSocketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		close(this->_serverSocketFd);
		throw std::runtime_error("Error: Failed to set SO_REUSEADDR on socket.");
	}

	// 3. Set non-blocking mode
	if (fcntl(this->_serverSocketFd, F_SETFL, O_NONBLOCK) < 0)
	{
		close(this->_serverSocketFd);
		throw std::runtime_error("Error: Failed to set non-blocking flag on server socket.");
	}

	// 4. Bind address and port
	struct sockaddr_in	serverAddr;
	std::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(this->_port);

	if (bind(this->_serverSocketFd, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0)
	{
		close(this->_serverSocketFd);
		throw std::runtime_error("Error: Failed to bind socket to port.");
	}

	// 5. Listen for incoming connections
	if (listen(this->_serverSocketFd, SOMAXCONN) < 0)
	{
		close(this->_serverSocketFd);
		throw std::runtime_error("Error: Failed to listen on socket.");
	}

	// Add server socket to poll set
	struct pollfd	serverPollFd;
	serverPollFd.fd = this->_serverSocketFd;
	serverPollFd.events = POLLIN;
	serverPollFd.revents = 0;
	this->_pollFds.push_back(serverPollFd);
}

void					Server::start()
{
	std::cout << "Server listening on port " << this->_port << "...\n";

	while (!Server::_signalReceived)
	{
		// Activate POLLOUT only for clients that have queued messages waiting to be sent
		for (size_t i = 1; i < this->_pollFds.size(); ++i)
		{
			int fd = this->_pollFds[i].fd;
			std::map<int, Client*>::iterator	it = this->_clients.find(fd);
			if (it != this->_clients.end() && it->second->hasQueuedMessages())
				this->_pollFds[i].events = POLLIN | POLLOUT;
			else
				this->_pollFds[i].events = POLLIN;
		}

		int		ret = poll(&this->_pollFds[0], this->_pollFds.size(), -1);
		if (ret < 0)
		{
			if (Server::_signalReceived)
				break;
			throw std::runtime_error("Error: poll() system call failed.");
		}

		for (size_t i = 0; i < this->_pollFds.size(); ++i)
		{
			if (this->_pollFds[i].revents == 0)
				continue ;

			if (this->_pollFds[i].revents & POLLIN)
			{
				if (this->_pollFds[i].fd == this->_serverSocketFd)
					_acceptClient();
				else
					_handleClientData(this->_pollFds[i].fd, i);
			}
			else if (this->_pollFds[i].revents & POLLOUT)
			{
				_flushClientQueue(this->_pollFds[i].fd, i);
			}
			else if (this->_pollFds[i].revents & (POLLHUP | POLLERR | POLLNVAL))
			{
				_closeClient(this->_pollFds[i].fd, i);
			}
		}
	}
}

void					Server::_acceptClient()
{
	struct sockaddr_in	clientAddr;
	socklen_t			clientLen = sizeof(clientAddr);

	int					clientFd = accept(this->_serverSocketFd, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientLen);
	if (clientFd < 0)
		return ;

	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0)
	{
		close(clientFd);
		return ;
	}

	struct pollfd		clientPollFd;
	clientPollFd.fd = clientFd;
	clientPollFd.events = POLLIN;
	clientPollFd.revents = 0;

	this->_pollFds.push_back(clientPollFd);
	this->_clients[clientFd] = new Client(clientFd);
	this->_clients[clientFd]->setHost(inet_ntoa(clientAddr.sin_addr));

	std::cout << "[+] New connection from fd " << clientFd << " (" 
	<< inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << ")\n";
}

void					Server::_handleClientData(int clientFd, size_t pollIndex)
{
	char				buffer[1024];
	std::memset(buffer, 0, sizeof(buffer));

	ssize_t				bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
	if (bytesRead <= 0)
	{
		_closeClient(clientFd, pollIndex);
		return;
	}

	Client* client = this->_clients[clientFd];
	client->appendBuffer(std::string(buffer, bytesRead));
	if (client->getBufferSize() > 4096)
		client->setClosing();

	while (!client->isClosing() && client->hasCompleteMessage())
	{
		std::string command = client->extractMessage();

		if (!command.empty())
		{
			std::cout << "[fd " << clientFd << "] Recv command: \"" << command << "\"\n";
			Message msg = parseMessage(command);
			if (msg.command.empty())
				continue;
			executeCommand(*this, *client, msg);
			if (client->isClosing())
				break;
		}
	}

	if (client->isClosing())
	{
		// Flush remaining responses (like QUIT/ERROR) before closing socket
		while (client->hasQueuedMessages())
		{
			std::string out = client->popQueuedMessage();
			send(clientFd, out.c_str(), out.size(), MSG_NOSIGNAL);
		}
		_closeClient(clientFd, pollIndex);
	}
}

void					Server::_flushClientQueue(int clientFd, size_t pollIndex)
{
	std::map<int, Client*>::iterator	it = this->_clients.find(clientFd);
	if (it == this->_clients.end())
		return ;

	Client*	client = it->second;
	while (client->hasQueuedMessages())
	{
		std::string	msg = client->popQueuedMessage();
		ssize_t sent = send(clientFd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
		if (sent < 0)
		{
			_closeClient(clientFd, pollIndex);
			return ;
		}
	}
}

void					Server::_closeClient(int clientFd, size_t pollIndex)
{
	std::cout << "[-] Client disconnected on fd " << clientFd << "\n";

	std::map<int, Client*>::iterator	cit = this->_clients.find(clientFd);
	if (cit != this->_clients.end())
	{
		Client* client = cit->second;

		if (client->isAuthenticated())
		{
			std::string	reason = client->getQuitReason().empty()
				? "Remote host closed connection" : client->getQuitReason();
			std::string	quitMsg = ":" + client->prefix() + " QUIT :" + reason + "\r\n";
			broadcastToUserChannels(*client, quitMsg, false);
		}

		// Remove client from all channels (members, operators, guests) before deleting
		for (std::map<std::string, Channel*>::iterator chIt = this->_channels.begin();
			 chIt != this->_channels.end(); )
		{
			Channel* ch = chIt->second;
			ch->removeClient(client);
			if (ch->isEmpty())
			{
				delete ch;
				this->_channels.erase(chIt++);
			}
			else
				++chIt;
		}
		delete client;
		this->_clients.erase(cit);
	}

	close(clientFd);
	this->_pollFds.erase(this->_pollFds.begin() + pollIndex);
}

void					Server::_cleanup()
{
	std::cout << "\nShutting down server and closing open descriptors...\n";
	for (size_t i = 0; i < this->_pollFds.size(); ++i)
	{
		if (this->_pollFds[i].fd >= 0)
			close(this->_pollFds[i].fd);
	}
	this->_pollFds.clear();

	for (std::map<std::string, Channel*>::iterator	it = this->_channels.begin();
		 it != this->_channels.end(); ++it)
	{
		delete it->second;
	}
	this->_channels.clear();

	for (std::map<int, Client*>::iterator	it = this->_clients.begin();
		 it != this->_clients.end(); ++it)
	{
		delete it->second;
	}
	this->_clients.clear();
}

Server::Server(const Server& other)
	:	_port(other._port),
		_password(other._password),
		_serverSocketFd(other._serverSocketFd),
		_pollFds(other._pollFds),
		_channels(other._channels),
		_clients(other._clients) {}

Server&					Server::operator=(const Server& other)
{
	if (this != &other)
	{
		this->_port = other._port;
		this->_password = other._password;
		this->_serverSocketFd = other._serverSocketFd;
		this->_pollFds = other._pollFds;
		this->_channels = other._channels;
		this->_clients = other._clients;
	}
	return (*this);
}

Server::~Server()
{
	_cleanup();
}

int						Server::getPort() const
{
	return (this->_port);
}

const std::string&		Server::getPassword() const
{
	return (this->_password);
}

std::string				Server::getName() const
{
	return (SERVER_NAME);
}

Channel*				Server::findChannel(const std::string& name)
{
	std::string	key = name;
	for (size_t i = 0; i < key.length(); ++i)
		key[i] = std::tolower(key[i]);

	std::map<std::string, Channel*>::iterator	it = this->_channels.find(key);
	if (it != this->_channels.end())
		return (it->second);
	return (NULL);
}

Client*					Server::findClient(const std::string& nick)
{
	std::string	target = nick;
	for (size_t i = 0; i < target.length(); ++i)
		target[i] = std::tolower(target[i]);

	for (std::map<int, Client*>::iterator	it = this->_clients.begin();
		 it != this->_clients.end(); ++it)
	{
		std::string current = it->second->getNick();
		for (size_t i = 0; i < current.length(); ++i)
			current[i] = std::tolower(current[i]);
		if (current == target)
			return (it->second);
	}
	return (NULL);
}

bool					Server::isNickInUse(const std::string& nick, const Client* exclude) const
{
	std::string	target = nick;
	for (size_t i = 0; i < target.length(); ++i)
		target[i] = std::tolower(target[i]);

	for (std::map<int, Client*>::const_iterator it = this->_clients.begin();
		 it != this->_clients.end(); ++it)
	{
		if (it->second == exclude)
			continue ;
		std::string	current = it->second->getNick();
		for (size_t i = 0; i < current.length(); ++i)
			current[i] = std::tolower(current[i]);
		if (current == target)
			return (true);
	}
	return (false);
}

void					Server::insertMap(Channel* channel)
{
	if (!channel)
		return ;
	std::string	key = channel->getName();
	for (size_t i = 0; i < key.length(); ++i)
		key[i] = std::tolower(key[i]);
	this->_channels[key] = channel;
}

void					Server::requestDeleteChannel(std::string name)
{
	std::string key = name;
	for (size_t i = 0; i < key.length(); ++i)
		key[i] = std::tolower(key[i]);

	std::map<std::string, Channel*>::iterator	it = this->_channels.find(key);
	if (it != this->_channels.end())
	{
		delete it->second;
		this->_channels.erase(it);
	}
}

bool					Server::isValidName(const std::string& name) const
{
	if (name.empty())
		return (false);
	if (name[0] == '#' || name[0] == '&')
	{
		if (name.length() < 2 || name.length() > 50)
			return (false);
		if (name.find_first_of(" ,\a\r\n:") != std::string::npos)
			return (false);
		return (true);
	}
	return (false);
}

void					Server::broadcastToUserChannels(Client& sender, const std::string& msg, bool includeSender)
{
	std::set<Client*>	recipients;

	for (std::map<std::string, Channel*>::iterator it = this->_channels.begin(); it != this->_channels.end(); ++it)
	{
		Channel*	ch = it->second;
		if (ch->hasMember(&sender))
		{
			const std::set<Client*>&	members = ch->getMembers();
			recipients.insert(members.begin(), members.end());
		}
	}

	if (includeSender)
		recipients.insert(&sender);
	else
		recipients.erase(&sender);
	for(std::set<Client*>::iterator it = recipients.begin(); it != recipients.end(); ++it)
		(*it)->queue(msg);
}
