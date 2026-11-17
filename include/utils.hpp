#ifndef UTILS_HPP
# define UTILS_HPP

# include <sstream>
# include <string>
# include <cstddef>

std::string toString(size_t n);
bool		toSize_t(const std::string &str, size_t &out);
bool		allDigits(const std::string &str);

#endif