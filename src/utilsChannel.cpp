#include "utilsChannel.hpp"

std::string toString(size_t n)
{
	std::ostringstream oss;
	oss << n;
	return (oss.str());
}

bool toSize_t(const std::string &str, std::size_t &out)
{
	std::istringstream	iss(str);

	iss >> out;
	if (iss.fail())
		return (false);
	return (true);
}

bool	allDigits(const std::string &str)
{
	if (str.empty())
		return (false);
	if (str.find_first_not_of("0123456789") != std::string::npos)
		return (false);
	return (true);
}