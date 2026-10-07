#include <iostream>

int	main(int argc, char** argv)
{
	if (argc != 3)
	{
		std::cerr << "Usage: ./ircserv <port> <password>\n";
		return (1);
	}
	std::cout << "IRC Server starting on port " << argv[1] << "...\n";
	return (0);
}

