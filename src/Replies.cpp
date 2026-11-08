/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Replies.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kanye <kanye@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 21:53:53 by kanye             #+#    #+#             */
/*   Updated: 2026/11/09 00:44:38 by kanye            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Replies.hpp"
#include "Server.hpp"
#include "Client.hpp"

//Standard text of each numeric
static const char*	defaultText(int code)
{
	switch (code)
	{
		case RPL_NOTOPIC:			return ("No topic is set");
		case RPL_ENDOFNAMES:		return ("End of /NAMES list");
		case ERR_NOSUCHNICK:		return ("No such nick/channel");
		case ERR_NOSUCHCHANNEL:		return ("No such channel");
		case ERR_CANNOTSENDTOCHAN:	return ("Cannot send to channel");
		case ERR_NOORIGIN:			return ("No origin specified");
		case ERR_NORECIPIENT:		return ("No recipient given");
		case ERR_NOTEXTTOSEND:		return ("No text to send");
		case ERR_INPUTTOOLONG:		return ("Input line was too long");
		case ERR_UNKNOWNCOMMAND:	return ("Unknown command");
		case ERR_NOMOTD:			return ("MOTD File is missing");
		case ERR_NONICKNAMEGIVEN:	return ("No nickname given");
		case ERR_ERRONEUSNICKNAME:	return ("Erroneous nickname");
		case ERR_NICKNAMEINUSE:		return ("Nickname is already in use");
		case ERR_USERNOTINCHANNEL:	return ("They aren't on that channel");
		case ERR_NOTONCHANNEL:		return ("You're not on that channel");
		case ERR_USERONCHANNEL:		return ("is already on channel");
		case ERR_NOTREGISTERED:		return ("You have not registered");
		case ERR_NEEDMOREPARAMS:	return ("Not enough parameters");
		case ERR_ALREADYREGISTRED:	return ("You may not reregister");
		case ERR_PASSWDMISMATCH:	return ("Password incorrect");
		case ERR_CHANNELISFULL:		return ("Cannot join channel (+l)");
		case ERR_UNKNOWNMODE:		return ("is unknown mode char to me");
		case ERR_INVITEONLYCHAN:	return ("Cannot join channel (+i)");
		case ERR_BADCHANNELKEY:		return ("Cannot join channel (+k)");
		case ERR_CHANOPRIVSNEEDED:	return ("You're not channel operator");
		default:					return ("");
	}
}

void	sendNumeric(Server& server, Client& client, int code, const std::string& params, const std::string& trailing)
{
	(void)server;
	std::string	line = ":" + std::string(SERVER_NAME) + " ";
	std::string	text = (trailing.empty() ? std::string(defaultText(code)) : trailing);

	line += static_cast<char>('0' + (code / 100) % 10);
	line += static_cast<char>('0' + (code / 10) % 10);
	line += static_cast<char>('0' + code % 10);
	line += " ";
	line += (client.hasNick() ? client.getNick() : std::string("*"));
	if (!params.empty())
		line += " " + params;
	if (!text.empty())
		line += " :" + text;
	line += "\r\n";
	client.queue(line);
}