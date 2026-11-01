/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kanye <kanye@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/01 15:31:14 by kanye             #+#    #+#             */
/*   Updated: 2026/11/01 20:39:14 by kanye            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Message.hpp"


static const size_t	MAX_PARAMS = 15;


//Helpers
static void	skipSpaces(std::string &input)
{
	size_t	start = input.find_first_not_of(' ');
 
	if (start == std::string::npos) //In case we could not find a letter
		input.clear();
	else
		input.erase(0, start);
}

//It also modifies the original input message deleting the word
static std::string	takeWord(std::string &input)
{
	size_t		pos = input.find(' ');
	std::string	word;
 
	if (pos == std::string::npos)
	{
		word = input;
		input.clear();
	}
	else
	{
		word = input.substr(0, pos);
		input.erase(0, pos + 1);
	}
	return (word);
}

//Split part
static void	parseHead(std::string &input, Message &msg)
{
	skipSpaces(input);
	
    //Preffix
	if (!input.empty() && input[0] == ':')
	{
		msg.prefix = takeWord(input).substr(1);
		skipSpaces(input);
	}
 
    //Command
	msg.command = takeWord(input);
	for (size_t i = 0; i < msg.command.size(); i++)
		msg.command[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(msg.command[i])));
}


static void	parseParams(std::string &input, Message &msg)
{
	while (msg.params.size() < MAX_PARAMS)
	{
		skipSpaces(input);
		if (input.empty()) //No more to process
			break ;
		if (input[0] == ':')//Everything until the end of the line belongs to a single parameter
		{
			msg.params.push_back(input.substr(1));
			break ;
		}
		msg.params.push_back(takeWord(input));
	}
}


Message	parseMessage(const std::string &rawCmd)
{
	Message		msg;
	std::string	input = rawCmd;

	parseHead(input, msg);
	if (msg.command.empty())
		return (msg);
	parseParams(input, msg);
	return (msg);
}