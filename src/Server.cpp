#include "Server.hpp"

bool			Server::signalReceived_ = false;

void			Server::signalHandler(int sigNum)
{
	(void)sigNum;
	Server::signalReceived_ = true;
}

Server::Server()
	: port_(0), password_(""), serverSocketFd_(-1), pollFds_(), clientBuffers_()
{
}

Server::Server(const std::string& portStr, const std::string& passStr)
	: port_(0), password_(""), serverSocketFd_(-1), pollFds_(), clientBuffers_()
{
	parseArgs_(portStr, passStr);
	initSocket_();
}

void			Server::parseArgs_(const std::string& portStr, const std::string& passStr)
{
	if (portStr.empty())
		throw std::runtime_error("Error: Port cannot be empty.");

	for (size_t i = 0; i < portStr.length(); ++i)
	{
		if (!std::isdigit(portStr[i]))
			throw std::runtime_error("Error: Port must contain only numeric digits.");
	}

	errno = 0;
	char*			endPtr = NULL;
	long			portVal = std::strtol(portStr.c_str(), &endPtr, 10);

	if (errno == ERANGE || portVal < 1024 || portVal > 65535)
		throw std::runtime_error("Error: Port out of valid range (1024 - 65535).");

	if (passStr.empty())
		throw std::runtime_error("Error: Password cannot be empty.");

	this->port_ = static_cast<int>(portVal);
	this->password_ = passStr;
}

void			Server::initSocket_()
{
	// 1. Create socket
	this->serverSocketFd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (this->serverSocketFd_ < 0)
		throw std::runtime_error("Error: Failed to create server socket.");

	// 2. Allow quick reuse of port/address
	int			opt = 1;
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
	struct		sockaddr_in	serverAddr;
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

void			Server::start()
{
	std::cout << "Server listening on port " << this->port_ << "...\n";

	while (!Server::signalReceived_)
	{
		int		ret = poll(&this->pollFds_[0], this->pollFds_.size(), -1);
		if (ret < 0)
		{
			if (Server::signalReceived_)
				break;
			throw std::runtime_error("Error: poll() system call failed.");
		}

		// Iterate backwards or by index to safely remove disconnected descriptors
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
			else if (this->pollFds_[i].revents & (POLLHUP | POLLERR | POLLNVAL))
			{
				closeClient_(this->pollFds_[i].fd, i);
			}
		}
	}
}

void			Server::acceptClient_()
{
	struct sockaddr_in	clientAddr;
	socklen_t			clientLen = sizeof(clientAddr);

	int					clientFd = accept(this->serverSocketFd_, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientLen);
	if (clientFd < 0)
		return ;

	// Set incoming client socket to non-blocking
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
	this->clientBuffers_[clientFd] = "";

	std::cout << "[+] New connection from fd " << clientFd << " (" 
	<< inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << ")\n";
}

void			Server::handleClientData_(int clientFd, size_t pollIndex)
{
	char			buffer[1024];
	std::memset(buffer, 0, sizeof(buffer));

	ssize_t			bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
	if (bytesRead <= 0)
	{
		closeClient_(clientFd, pollIndex);
		return;
	}

	// Accumulate received bytes in client's buffer
	this->clientBuffers_[clientFd].append(buffer, bytesRead);

	// Process all full commands terminated with \r\n or \n
	std::string&	clientBuf = this->clientBuffers_[clientFd];
	size_t			pos;
	while ((pos = clientBuf.find("\n")) != std::string::npos)
	{
		std::string		command = clientBuf.substr(0, pos);
		if (!command.empty() && command[command.length() - 1] == '\r')
			command.erase(command.length() - 1);

		clientBuf.erase(0, pos + 1);

		if (!command.empty())
		{
			std::cout << "[fd " << clientFd << "] Recv command: \"" << command << "\"\n";
			// TODO: Here Kang's parser will be called:
			// parseAndExecute(clientFd, command);
		}
	}
}

void			Server::closeClient_(int clientFd, size_t pollIndex)
{
	std::cout << "[-] Client disconnected on fd " << clientFd << "\n";
	close(clientFd);
	this->clientBuffers_.erase(clientFd);
	this->pollFds_.erase(this->pollFds_.begin() + pollIndex);
}

void			Server::cleanup_()
{
	std::cout << "\nShutting down server and closing open descriptors...\n";
	for (size_t i = 0; i < this->pollFds_.size(); ++i)
	{
		if (this->pollFds_[i].fd >= 0)
			close(this->pollFds_[i].fd);
	}
	this->pollFds_.clear();
	this->clientBuffers_.clear();
}

Server::Server(const Server& other)
	:	port_(other.port_),
		password_(other.password_),
		serverSocketFd_(other.serverSocketFd_),
		pollFds_(other.pollFds_),
		clientBuffers_(other.clientBuffers_) {}

Server&			Server::operator=(const Server& other)
{
	if (this != &other)
	{
		this->port_ = other.port_;
		this->password_ = other.password_;
		this->serverSocketFd_ = other.serverSocketFd_;
		this->pollFds_ = other.pollFds_;
		this->clientBuffers_ = other.clientBuffers_;
	}
	return (*this);
}

Server::~Server()
{
	cleanup_();
}

int					Server::getPort() const
{
	return (this->port_);
}

const std::string&	Server::getPassword() const
{
	return (this->password_);
}
