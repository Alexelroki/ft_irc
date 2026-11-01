/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Message.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kanye <kanye@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/01 15:31:20 by kanye             #+#    #+#             */
/*   Updated: 2026/11/01 15:37:35 by kanye            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MESSAGE_HPP
# define MESSAGE_HPP

#include <string>
#include <vector>
#include <cctype>

struct Message
{
    public:
        std::string                 prefix;
        std::string                 command;//UPPERCASE ALWAYS
        std::vector<std::string>    params;
};

Message	parseMessage(const std::string &rawCmd);


#endif