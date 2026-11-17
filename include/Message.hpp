#ifndef MESSAGE_HPP
# define MESSAGE_HPP

#include <string>
#include <vector>
#include <cctype>

struct Message
{
	public:
		std::string                 prefix;
		std::string                 command; //UPPERCASE ALWAYS
		std::vector<std::string>    params;
};

Message	parseMessage(const std::string &rawCmd);

#endif