#ifndef UTILSCHANNEL_HPP
# define UTILSCHANNEL_HPP

# include <sstream>

std::string toString(size_t n);
bool		toSize_t(const std::string &str, std::size_t &out);
bool		allDigits(const std::string &str);

#endif