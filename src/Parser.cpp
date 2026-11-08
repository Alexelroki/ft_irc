/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kanye <kanye@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/01 15:31:14 by kanye             #+#    #+#             */
/*   Updated: 2026/11/09 00:52:31 by kanye            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Message.hpp"


static const size_t	MAX_PARAMS = 15;
static const size_t	MAX_LINE_LEN = 510;


//Helpers
static void	skipSpaces(std::string &input)
{
	size_t	start = input.find_first_not_of(' ');
 
	if (start == std::string::npos) //In case we could not find a letter
		input.clear();
	else
		input.erase(0, start);
}

//Extract the first word from the string and remove it from it
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
	
    //Preffix if it exist
	if (!input.empty() && input[0] == ':')
	{
		msg.prefix = takeWord(input).substr(1);
		skipSpaces(input);
	}
 
    //Command
	msg.command = takeWord(input);
	for (size_t i = 0; i < msg.command.size(); i++) //All to uppercase
		msg.command[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(msg.command[i])));
}


static void	parseParams(std::string &input, Message &msg)
{
	while (msg.params.size() < MAX_PARAMS)
	{
		skipSpaces(input);
		if (input.empty()) 
			break ;
		if (input[0] == ':')//Everything until the end of the line belongs to a single parameter
		{
			msg.params.push_back(input.substr(1));
			break ;
		}
		msg.params.push_back(takeWord(input));
	}
}

//We store everything in a Message struct
Message	parseMessage(const std::string &rawCmd)
{
	Message		msg;
	std::string	input = rawCmd;

	if (input.size() > MAX_LINE_LEN)//The remaining part is ignored if the message is too long
		input.resize(MAX_LINE_LEN);
	parseHead(input, msg);
	if (msg.command.empty())
		return (msg);
	parseParams(input, msg);
	return (msg);
}