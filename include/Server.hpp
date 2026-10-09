#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <vector>
# include <map>
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

class Server
{
	public:
		// Orthodox Canonical Form
		Server(const std::string& portStr, const std::string& passStr);
		Server(const Server& other);
		Server&						operator=(const Server& other);
		~Server();

		void						start();
		static void					signalHandler(int sigNum);

		// Getters
		int							getPort() const;
		const std::string&			getPassword() const;

	private:
		int							port_;
		std::string					password_;
		int							serverSocketFd_;
		std::vector<struct pollfd> 	pollFds_;
		std::map<int, std::string>	clientBuffers_;

		static bool					signalReceived_;

		Server();
		void						parseArgs_(const std::string& portStr, const std::string& passStr);
		void						initSocket_();
		void						acceptClient_();
		void						handleClientData_(int clientFd, size_t pollIndex);
		void						closeClient_(int clientFd, size_t pollIndex);
		void						cleanup_();
};

#endif
