#include "Server.hpp"
#include "Channel.hpp"
#include <algorithm>

bool					Server::signalReceived_ = false;

void					Server::signalHandler(int sigNum)
{
	(void)sigNum;
	Server::signalReceived_ = true;
}

Server::Server()
	: port_(0), password_(""), serverSocketFd_(-1), pollFds_(), channels_(), clients_() {}

Server::Server(const std::string& portStr, const std::string& passStr)
	: port_(0), password_(""), serverSocketFd_(-1), pollFds_(), channels_(), clients_()
{
	parseArgs_(portStr, passStr);
	initSocket_();
}

void					Server::parseArgs_(const std::string& portStr, const std::string& passStr)
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

	this->port_ = static_cast<int>(portVal);
	this->password_ = passStr;
}

void					Server::initSocket_()
{
	// 1. Create socket
	this->serverSocketFd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (this->serverSocketFd_ < 0)
		throw std::runtime_error("Error: Failed to create server socket.");

	// 2. Allow quick reuse of port/address
	int					opt = 1;
	if (setsockopt(this->serverSocketFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		close(this->serverSocketFd_);
		throw std::runtime_error("Error: Failed to set SO_REUSEADDR on socket.");
	}

	// 3. Set non-blocking mode
	if (fcntl(this->serverSocketFd_, F_SETFL, O_NONBLOCK) < 0)
	{
		close(this->serverSocketFd_);
		throw std::runtime_error("Error: Failed to set non-blocking flag on server socket.");
	}

	// 4. Bind address and port
	struct sockaddr_in	serverAddr;
	std::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(this->port_);

	if (bind(this->serverSocketFd_, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0)
	{
		close(this->serverSocketFd_);
		throw std::runtime_error("Error: Failed to bind socket to port.");
	}

	// 5. Listen for incoming connections
	if (listen(this->serverSocketFd_, SOMAXCONN) < 0)
	{
		close(this->serverSocketFd_);
		throw std::runtime_error("Error: Failed to listen on socket.");
	}

	// Add server socket to poll set
	struct pollfd	serverPollFd;
	serverPollFd.fd = this->serverSocketFd_;
	serverPollFd.events = POLLIN;
	serverPollFd.revents = 0;
	this->pollFds_.push_back(serverPollFd);
}

void					Server::start()
{
	std::cout << "Server listening on port " << this->port_ << "...\n";

	while (!Server::signalReceived_)
	{
		// Activate POLLOUT only for clients that have queued messages waiting to be sent
		for (size_t i = 1; i < this->pollFds_.size(); ++i)
		{
			int fd = this->pollFds_[i].fd;
			std::map<int, Client*>::iterator	it
 = this->clients_.find(fd);
			if (it != this->clients_.end() && it->second->hasQueuedMessages())
				this->pollFds_[i].events = POLLIN | POLLOUT;
			else
				this->pollFds_[i].events = POLLIN;
		}

		int		ret = poll(&this->pollFds_[0], this->pollFds_.size(), -1);
		if (ret < 0)
		{
			if (Server::signalReceived_)
				break;
			throw std::runtime_error("Error: poll() system call failed.");
		}

		for (size_t i = 0; i < this->pollFds_.size(); ++i)
		{
			if (this->pollFds_[i].revents == 0)
				continue ;

			if (this->pollFds_[i].revents & POLLIN)
			{
				if (this->pollFds_[i].fd == this->serverSocketFd_)
					acceptClient_();
				else
					handleClientData_(this->pollFds_[i].fd, i);
			}
			else if (this->pollFds_[i].revents & POLLOUT)
			{
				flushClientQueue_(this->pollFds_[i].fd, i);
			}
			else if (this->pollFds_[i].revents & (POLLHUP | POLLERR | POLLNVAL))
			{
				closeClient_(this->pollFds_[i].fd, i);
			}
		}
	}
}

void					Server::acceptClient_()
{
	struct sockaddr_in	clientAddr;
	socklen_t			clientLen = sizeof(clientAddr);

	int					clientFd = accept(this->serverSocketFd_, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientLen);
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

	this->pollFds_.push_back(clientPollFd);
	this->clients_[clientFd] = new Client(clientFd);
	this->clients_[clientFd]->setHost(inet_ntoa(clientAddr.sin_addr));

	std::cout << "[+] New connection from fd " << clientFd << " (" 
	<< inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << ")\n";
}

void					Server::handleClientData_(int clientFd, size_t pollIndex)
{
	char				buffer[1024];
	std::memset(buffer, 0, sizeof(buffer));

	ssize_t				bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
	if (bytesRead <= 0)
	{
		closeClient_(clientFd, pollIndex);
		return;
	}

	Client* client = this->clients_[clientFd];
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
		closeClient_(clientFd, pollIndex);
	}
}

void					Server::flushClientQueue_(int clientFd, size_t pollIndex)
{
	std::map<int, Client*>::iterator	it = this->clients_.find(clientFd);
	if (it == this->clients_.end())
		return ;

	Client*	client = it->second;
	while (client->hasQueuedMessages())
	{
		std::string	msg = client->popQueuedMessage();
		ssize_t sent = send(clientFd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
		if (sent < 0)
		{
			closeClient_(clientFd, pollIndex);
			return ;
		}
	}
}

void					Server::closeClient_(int clientFd, size_t pollIndex)
{
	std::cout << "[-] Client disconnected on fd " << clientFd << "\n";

	std::map<int, Client*>::iterator	cit = this->clients_.find(clientFd);
	if (cit != this->clients_.end())
	{
		Client* client = cit->second;
		// Remove client from all channels (members, operators, guests) before deleting
		for (std::map<std::string, Channel*>::iterator chIt = this->channels_.begin();
			 chIt != this->channels_.end(); )
		{
			Channel* ch = chIt->second;
			ch->removeClient(client);
			if (ch->isEmpty())
			{
				delete ch;
				this->channels_.erase(chIt++);
			}
			else
				++chIt;
		}
		delete client;
		this->clients_.erase(cit);
	}

	close(clientFd);
	this->pollFds_.erase(this->pollFds_.begin() + pollIndex);
}

void					Server::cleanup_()
{
	std::cout << "\nShutting down server and closing open descriptors...\n";
	for (size_t i = 0; i < this->pollFds_.size(); ++i)
	{
		if (this->pollFds_[i].fd >= 0)
			close(this->pollFds_[i].fd);
	}
	this->pollFds_.clear();

	for (std::map<std::string, Channel*>::iterator	it = this->channels_.begin();
		 it != this->channels_.end(); ++it)
	{
		delete it->second;
	}
	this->channels_.clear();

	for (std::map<int, Client*>::iterator	it = this->clients_.begin();
		 it != this->clients_.end(); ++it)
	{
		delete it->second;
	}
	this->clients_.clear();
}

Server::Server(const Server& other)
	:	port_(other.port_),
		password_(other.password_),
		serverSocketFd_(other.serverSocketFd_),
		pollFds_(other.pollFds_),
		channels_(other.channels_),
		clients_(other.clients_) {}

Server&					Server::operator=(const Server& other)
{
	if (this != &other)
	{
		this->port_ = other.port_;
		this->password_ = other.password_;
		this->serverSocketFd_ = other.serverSocketFd_;
		this->pollFds_ = other.pollFds_;
		this->channels_ = other.channels_;
		this->clients_ = other.clients_;
	}
	return (*this);
}

Server::~Server()
{
	cleanup_();
}

int						Server::getPort() const
{
	return (this->port_);
}

const std::string&		Server::getPassword() const
{
	return (this->password_);
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

	std::map<std::string, Channel*>::iterator	it = this->channels_.find(key);
	if (it != this->channels_.end())
		return (it->second);
	return (NULL);
}

Client*					Server::findClient(const std::string& nick)
{
	std::string	target = nick;
	for (size_t i = 0; i < target.length(); ++i)
		target[i] = std::tolower(target[i]);

	for (std::map<int, Client*>::iterator	it = this->clients_.begin();
		 it != this->clients_.end(); ++it)
	{
		std::string current = it->second->getNick();
		for (size_t i = 0; i < current.length(); ++i)
			current[i] = std::tolower(current[i]);
		if (current == target)
			return (it->second);
	}
	return (NULL);
}

Client*					Server::findClientByNick(const std::string& nick)
{
	return (findClient(nick));
}

bool					Server::isNickInUse(const std::string& nick, const Client* exclude) const
{
	std::string	target = nick;
	for (size_t i = 0; i < target.length(); ++i)
		target[i] = std::tolower(target[i]);

	for (std::map<int, Client*>::const_iterator it = this->clients_.begin();
		 it != this->clients_.end(); ++it)
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
	this->channels_[key] = channel;
}

void					Server::requestDeleteChannel(std::string name)
{
	std::string key = name;
	for (size_t i = 0; i < key.length(); ++i)
		key[i] = std::tolower(key[i]);

	std::map<std::string, Channel*>::iterator	it = this->channels_.find(key);
	if (it != this->channels_.end())
	{
		delete it->second;
		this->channels_.erase(it);
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
	return (true);
}
