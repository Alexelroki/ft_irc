/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kanye <kanye@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/01 21:32:13 by kanye             #+#    #+#             */
/*   Updated: 2026/11/02 01:11:37 by kanye            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CommandHandler.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Message.hpp"
// #include "cmdChannel.hpp"    // Darío: cmdJoin, cmdPart, cmdKick, cmdInvite, cmdTopic, cmdMode, cmdPrivmsg


static void sendNumeric(Server& server, Client& client, int code,
                const std::string& params = "", const std::string& trailing = "")
{
    (void)server;
    std::string line = ":" + std::string(SERVER_NAME) + " ";

    line += static_cast<char>('0' + (code / 100) % 10);
    line += static_cast<char>('0' + (code / 10) % 10);
    line += static_cast<char>('0' + code % 10);
    line += " ";
    line += (client.hasNick() ? client.getNick() : std::string("*"));
    if (!params.empty())
        line += " " + params;
    if (!trailing.empty())
        line += " :" + trailing;
    line += "\r\n";
    client.queue(line);
}

// Nick: 1..30 chars, starts with a letter or one of []\`_^{|}, then letters, digits, '-' or those specials.
static bool isValidNick(const std::string& nick)
{
    static const std::string    special = "[]\\`_^{|}";

    if (nick.empty() || nick.size() > 30)
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

static std::string  toUpper(const std::string& s)
{
    std::string res(s);

    for (size_t i = 0; i < res.size(); ++i)
        res[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(res[i])));
    return (res);
}

//REGISTRATION

static void sendWelcome(Server& server, Client& client)
{
    std::string s_name = SERVER_NAME;
    sendNumeric(server, client, 1, "", "Welcome to the IRC Network " + client.prefix());
    sendNumeric(server, client, 2, "", "Your host is " + s_name + ", running version 1.0");
    sendNumeric(server, client, 3, "", "This server was created when it was started");
    sendNumeric(server, client, 4, s_name + " 1.0 o itkol");
    sendNumeric(server, client, 422, "", "MOTD File is missing");
}

// Called after NICK and after USER: registration finishes when the last of
// PASS (correct), NICK and USER arrives.
static void tryRegister(Server& server, Client& client)
{
    if (client.isReadyForWelcome())
    {
        client.setAuthenticated();
        sendWelcome(server, client);
    }
    else if (!client.isAuthenticated() && client.hasNick() && client.hasUser())
    {
        // NICK and USER are done but PASS was missing or wrong
        sendNumeric(server, client, 464, "", "Password incorrect");
        client.queue("ERROR :Closing Link: " + client.getHost() + " (Password incorrect)\r\n");
        client.setClosing();
    }
}

static void handlePass(Server& server, Client& client, const Message& msg)
{
    if (client.isAuthenticated())
    {
        sendNumeric(server, client, 462, "", "You may not reregister");
        return ;
    }
    if (msg.params.empty())
    {
        sendNumeric(server, client, 461, "PASS", "Not enough parameters");
        return ;
    }
    if (msg.params[0] == server.getPassword())
        client.setPassed();
    else
        sendNumeric(server, client, 464, "", "Password incorrect");
}

static void handleNick(Server& server, Client& client, const Message& msg)
{
    if (msg.params.empty() || msg.params[0].empty())
    {
        sendNumeric(server, client, 431, "", "No nickname given");
        return ;
    }

    const std::string&  nick = msg.params[0];

    if (!isValidNick(nick))
    {
        sendNumeric(server, client, 432, nick, "Erroneous nickname");
        return ;
    }
    if (server.isNickInUse(nick, &client))
    {
        sendNumeric(server, client, 433, nick, "Nickname is already in use");
        return ;
    }

    if (client.isAuthenticated())
    {
        if (nick == client.getNick())
            return ;
        // The user sees their own change. TODO (channels): send the same line
        // to everyone who shares a channel with them (once each).
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
        sendNumeric(server, client, 462, "", "You may not reregister");
        return ;
    }
    if (msg.params.size() < 4 || msg.params[0].empty())
    {
        sendNumeric(server, client, 461, "USER", "Not enough parameters");
        return ;
    }
    client.setUsername(msg.params[0]);
    client.setRealname(msg.params[3]);
    client.setUserDone();
    tryRegister(server, client);
}

static void handleQuit(Server& server, Client& client, const Message& msg)
{
    (void)server;
    std::string reason = "Client Quit";

    if (!msg.params.empty())
        reason = msg.params[0];
    client.queue("ERROR :Closing Link: " + client.getHost() + " (Quit: " + reason + ")\r\n");
    // TODO (channels): the QUIT line for the other users is sent when the
    // server removes the client (Client has no place to keep the reason yet).
    client.setClosing();
}

// No capabilities supported: answer LS/LIST with an empty list, NAK any REQ.
static void handleCap(Server& server, Client& client, const Message& msg)
{
    if (msg.params.empty())
    {
        sendNumeric(server, client, 461, "CAP", "Not enough parameters");
        return ;
    }

    std::string sub = toUpper(msg.params[0]);
    std::string target = client.hasNick() ? client.getNick() : std::string("*");
    std::string s_name = SERVER_NAME;

    if (sub == "LS" || sub == "LIST")
        client.queue(":" + s_name + " CAP " + target + " " + sub + " :\r\n");
    else if (sub == "REQ")
        client.queue(":" + s_name + " CAP " + target + " NAK :"
            + (msg.params.size() > 1 ? msg.params[1] : std::string("")) + "\r\n");
    // CAP END and anything else: nothing to answer
}

static void handlePing(Server& server, Client& client, const Message& msg)
{
    if (msg.params.empty())
    {
        sendNumeric(server, client, 409, "", "No origin specified");
        return ;
    }
    std::string s_name = SERVER_NAME;
    client.queue(":" + s_name + " PONG " + s_name + " :" + msg.params[0] + "\r\n");
}

// PRIVMSG   

static void handlePrivmsg(Server& server, Client& client, const Message& msg)
{
    if (msg.params.empty() || msg.params[0].empty())
    {
        sendNumeric(server, client, 411, "", "No recipient given (PRIVMSG)");
        return ;
    }
    if (msg.params.size() < 2 || msg.params[1].empty())
    {
        sendNumeric(server, client, 412, "", "No text to send");
        return ;
    }

    const std::string&  target = msg.params[0];

    // To a channel: Darío's code
    if (target[0] == '#' || target[0] == '&')
    {
        // cmdPrivmsg(server, client, msg);     // <-- connect when Channel is merged
        sendNumeric(server, client, 403, target, "No such channel");
        return ;
    }

    // To a user
    Client* dest = server.findClientByNick(target);
    if (dest == NULL)
    {
        sendNumeric(server, client, 401, target, "No such nick/channel");
        return ;
    }
    dest->queue(":" + client.prefix() + " PRIVMSG " + dest->getNick() + " :" + msg.params[1] + "\r\n");
}

static void notConnectedYet(Server& server, Client& client, const Message& msg)
{
    sendNumeric(server, client, 421, msg.command, "Unknown command");
}

void    executeCommand(Server& server, Client& client, const Message& msg)
{
    const std::string&  cmd = msg.command;

    // 1. Allowed before registration
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
    // 2. Everything else needs a finished registration
    else if (!client.isAuthenticated())
        sendNumeric(server, client, 451, "", "You have not registered");
    else if (cmd == "PRIVMSG")
        handlePrivmsg(server, client, msg);
    else
        sendNumeric(server, client, 421, cmd, "Unknown command");
}