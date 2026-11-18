#include "utils.hpp"
#include <cctype>

std::string toString(size_t n)
{
	std::ostringstream oss;
	oss << n;
	return (oss.str());
}

bool toSize_t(const std::string &str, size_t &out)
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

std::string		toLower(const std::string &s)
{
	std::string res(s);
	size_t		i;

	i = 0;
	while (i < res.size())
	{
		res[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(res[i])));
		i++;
	}
	return (res);
}

std::string		toUpper(const std::string &s)
{
	std::string res(s);
	size_t		i;

	i = 0;
	while (i < res.size())
	{
		res[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(res[i])));
		i++;
	}
	return (res);
}
