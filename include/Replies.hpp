/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Replies.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kanye <kanye@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 21:53:58 by kanye             #+#    #+#             */
/*   Updated: 2026/11/09 00:43:05 by kanye            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REPLIES_HPP
# define REPLIES_HPP

# include <string>

class Server;
class Client;

//Numeric replies used by the server
enum e_reply
{
	RPL_WELCOME			= 1,
	RPL_YOURHOST		= 2,
	RPL_CREATED			= 3,
	RPL_MYINFO			= 4,
	RPL_CHANNELMODEIS	= 324,
	RPL_NOTOPIC			= 331,
	RPL_TOPIC			= 332,
	RPL_INVITING		= 341,
	RPL_NAMREPLY		= 353,
	RPL_ENDOFNAMES		= 366,

	ERR_NOSUCHNICK		= 401,
	ERR_NOSUCHCHANNEL	= 403,
	ERR_CANNOTSENDTOCHAN= 404,
	ERR_NOORIGIN		= 409,
	ERR_NORECIPIENT		= 411,
	ERR_NOTEXTTOSEND	= 412,
	ERR_INPUTTOOLONG	= 417,
	ERR_UNKNOWNCOMMAND	= 421,
	ERR_NOMOTD			= 422,
	ERR_NONICKNAMEGIVEN	= 431,
	ERR_ERRONEUSNICKNAME= 432,
	ERR_NICKNAMEINUSE	= 433,
	ERR_USERNOTINCHANNEL= 441,
	ERR_NOTONCHANNEL	= 442,
	ERR_USERONCHANNEL	= 443,
	ERR_NOTREGISTERED	= 451,
	ERR_NEEDMOREPARAMS	= 461,
	ERR_ALREADYREGISTRED= 462,
	ERR_PASSWDMISMATCH	= 464,
	ERR_CHANNELISFULL	= 471,
	ERR_UNKNOWNMODE		= 472,
	ERR_INVITEONLYCHAN	= 473,
	ERR_BADCHANNELKEY	= 475,
	ERR_CHANOPRIVSNEEDED= 482,
	ERR_INVALIDMODEPARAM= 696
};


void	sendNumeric(Server& server, Client& client, int code, const std::string& params = "", const std::string& trailing = "");

#endif