/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kanye <kanye@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/01 21:32:13 by kanye             #+#    #+#             */
/*   Updated: 2026/11/09 00:52:04 by kanye            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CommandHandler.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Message.hpp"
#include "Replies.hpp"
#include "cmdChannel.hpp"  


static const size_t	USERNAME_MAX = 18;
static const size_t	REALNAME_MAX = 50;
static const size_t NICKNAME_MAX = 30;


static bool isValidNick(const std::string& nick)
{
    static const std::string    special = "[]\\`_^{|}";

    if (nick.empty() || nick.size() > NICKNAME_MAX)
        return (false);
    for (size_t i = 0; i < nick.size(); ++i)
    {
        unsigned char   c = static_cast<unsigned char>(nick[i]);
        bool            isSpecial = (special.find(nick[i]) != std::string::npos);

        if (i == 0)
        {
            if (!std::isalpha(c) && !isSpecial)
                return (false);
        }
        else if (!std::isalnum(c) && !isSpecial && c != '-')
            return (false);
    }
    return (true);
}


static bool isValidUsername(const std::string& user)
{
    if (user.empty())
        return (false);
    for (size_t i = 0; i < user.size(); ++i)
    {
        unsigned char   c = static_cast<unsigned char>(user[i]);

        if (c <= 32 || c == 127 || c == '@')
            return (false);
    }
    return (true);
}

//Converts the whole string to upper
static std::string  toUpper(const std::string& s)
{
    std::string res(s);

    for (size_t i = 0; i < res.size(); ++i)
        res[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(res[i])));
    return (res);
}

//Tells the client why it is being disconnected and marks it to be closed.
static void closeLink(Client& client, const std::string& reason)
{
    client.queue("ERROR :Closing Link: " + client.getHost() + " (" + reason + ")\r\n");
    client.setClosing();
}



//REGISTRATION

static void sendWelcome(Server& server, Client& client)
{
    std::string s_name = SERVER_NAME;
    sendNumeric(server, client, RPL_WELCOME, "", "Welcome to the IRC Network " + client.prefix());
    sendNumeric(server, client, RPL_YOURHOST, "", "Your host is " + s_name + ", running version 1.0");
    sendNumeric(server, client, RPL_CREATED, "", "This server was created when it was started");
    sendNumeric(server, client, RPL_MYINFO, s_name + " 1.0 o itkol");
    sendNumeric(server, client, ERR_NOMOTD);
}

//Check the registration
static void tryRegister(Server& server, Client& client)
{
    if (client.isReadyForWelcome())
    {
        client.setAuthenticated();
        sendWelcome(server, client);
    }
}

static void handlePass(Server& server, Client& client, const Message& msg)
{
    if (client.isAuthenticated())
    {
        sendNumeric(server, client, ERR_ALREADYREGISTRED);
        return ;
    }
    if (msg.params.empty())
    {
        sendNumeric(server, client, ERR_NEEDMOREPARAMS, "PASS");
        return ;
    }
    if (msg.params[0] == server.getPassword())
    {
        client.setPassed();
        return ;
    }
    sendNumeric(server, client, ERR_PASSWDMISMATCH);
    closeLink(client, "Password incorrect");
}

static void handleNick(Server& server, Client& client, const Message& msg)
{
    if (!client.isAuthenticated() && !client.hasPassed())
    {
        client.queue(":" + std::string(SERVER_NAME) + " 400 :You must send a password first\r\n");
        return ;
    }

    if (msg.params.empty() || msg.params[0].empty())
    {
        sendNumeric(server, client, ERR_NONICKNAMEGIVEN);
        return ;
    }

    const std::string&  nick = msg.params[0];

    if (!isValidNick(nick))
    {
        sendNumeric(server, client, ERR_ERRONEUSNICKNAME, nick);
        return ;
    }
    if (server.isNickInUse(nick, &client))
    {
        sendNumeric(server, client, ERR_NICKNAMEINUSE, nick);
        return ;
    }

    if (client.isAuthenticated())
    {
        if (nick == client.getNick())
            return ;
        client.queue(":" + client.prefix() + " NICK :" + nick + "\r\n");
        client.setNick(nick);
        return ;
    }
    client.setNick(nick);
    tryRegister(server, client);
}

static void handleUser(Server& server, Client& client, const Message& msg)
{
    if (client.isAuthenticated())
    {
        sendNumeric(server, client, ERR_ALREADYREGISTRED);
        return ;
    }

    if (!client.hasPassed())
    {
        client.queue(":" + std::string(SERVER_NAME) + " 400 :You must send a password first\r\n");
        return ;
    }

    if (msg.params.size() < 4 || msg.params[0].empty())
    {
        sendNumeric(server, client, ERR_NEEDMOREPARAMS, "USER");
        return ;
    }
    if (!isValidUsername(msg.params[0]))
    {
        closeLink(client, "Invalid username");
        return ;
    }
    client.setUsername(msg.params[0].substr(0, USERNAME_MAX));
    client.setRealname(msg.params[3].substr(0, REALNAME_MAX));
    client.setUserDone();
    tryRegister(server, client);
}

//Can provide a message of the reason of leaving
static void handleQuit(Server& server, Client& client, const Message& msg)
{
    (void)server;
    std::string reason = "Client Quit";

    if (!msg.params.empty())
        reason = msg.params[0];
    client.setQuitReason(reason);
    closeLink(client, "Quit: " + reason);
}

//No capabilities supported
static void handleCap(Server& server, Client& client, const Message& msg)
{
    if (msg.params.empty())
    {
        sendNumeric(server, client, ERR_NEEDMOREPARAMS, "CAP");
        return ;
    }

    std::string sub = toUpper(msg.params[0]);
    std::string target = client.hasNick() ? client.getNick() : std::string("*");
    std::string s_name = SERVER_NAME;

    if (sub == "LS" || sub == "LIST")
        client.queue(":" + s_name + " CAP " + target + " " + sub + " :\r\n");
    else if (sub == "REQ")
        client.queue(":" + s_name + " CAP " + target + " NAK :" + (msg.params.size() > 1 ? msg.params[1] : std::string("")) + "\r\n");
}

static void handlePing(Server& server, Client& client, const Message& msg)
{
    if (msg.params.empty())
    {
        sendNumeric(server, client, ERR_NOORIGIN);
        return ;
    }
    std::string s_name = SERVER_NAME;
    client.queue(":" + s_name + " PONG " + s_name + " :" + msg.params[0] + "\r\n");
}

//PRIVMSG(send private message to another user or a specific channel)
static void handlePrivmsg(Server& server, Client& client, const Message& msg)
{
    if (msg.params.empty() || msg.params[0].empty())
    {
        sendNumeric(server, client, ERR_NORECIPIENT, "", "No recipient given (PRIVMSG)");
        return ;
    }
    if (msg.params.size() < 2 || msg.params[1].empty())
    {
        sendNumeric(server, client, ERR_NOTEXTTOSEND);
        return ;
    }

    const std::string&  target = msg.params[0]; //to who we want to send the message, so first we need to check if it is a channel or user
    if (target[0] == '#' || target[0] == '&')
    {
        cmdPrivmsg(server, client, msg);
        return ;
    }

    Client* dest = server.findClientByNick(target);
    if (dest == NULL)
    {
        sendNumeric(server, client, ERR_NOSUCHNICK, target);
        return ;
    }
    dest->queue(":" + client.prefix() + " PRIVMSG " + dest->getNick() + " :" + msg.params[1] + "\r\n");
}

void    executeCommand(Server& server, Client& client, const Message& msg)
{
    const std::string&  cmd = msg.command;

    if (client.isClosing())
        return ;

    //BEFORE REGISTRATION
    if (cmd == "CAP")
        handleCap(server, client, msg);
    else if (cmd == "PASS")
        handlePass(server, client, msg);
    else if (cmd == "NICK")
        handleNick(server, client, msg);
    else if (cmd == "USER")
        handleUser(server, client, msg);
    else if (cmd == "QUIT")
        handleQuit(server, client, msg);
    else if (cmd == "PING")
        handlePing(server, client, msg);
    else if (cmd == "PONG")
        return ;
        
    //AFTER REGISTRATION
    else if (!client.isAuthenticated())
        sendNumeric(server, client, ERR_NOTREGISTERED);
    else if (cmd == "PRIVMSG")
        handlePrivmsg(server, client, msg);
    else if (cmd == "JOIN")
        cmdJoin(server, client, msg);
    else if (cmd == "PART")
        cmdPart(server, client, msg);
    else if (cmd == "KICK")
        cmdKick(server, client, msg);
    else if (cmd == "INVITE")
        cmdInvite(server, client, msg);
    else if (cmd == "TOPIC")
        cmdTopic(server, client, msg);
    else if (cmd == "MODE")
        cmdMode(server, client, msg);
    else
        sendNumeric(server, client, ERR_UNKNOWNCOMMAND, cmd);
}