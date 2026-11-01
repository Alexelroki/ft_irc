/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kanye <kanye@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/01 21:32:13 by kanye             #+#    #+#             */
/*   Updated: 2026/11/01 21:51:15 by kanye            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CommandHandler.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Message.hpp"
// #include "cmdChannel.hpp" // Aquí incluirás los comandos de Darío

void executeCommand(Server& server, Client& client, const Message& msg)
{
    // Fase 2: Comandos de Autenticación (Los harás tú)
    if (msg.command == "PASS")
    {
        // cmdPass(server, client, msg);
    }
    else if (msg.command == "NICK")
    {
        // cmdNick(server, client, msg);
    }
    else if (msg.command == "USER")
    {
        // cmdUser(server, client, msg);
    }
    // Fase 2: Comandos de Canales (De Darío)
    else if (msg.command == "JOIN")
    {
        // cmdJoin(server, client, msg);
    }
    else if (msg.command == "PRIVMSG")
    {
        // cmdPrivmsg(server, client, msg);
    }
    else if (msg.command == "QUIT")
    {
        // cmdQuit(server, client, msg);
        client.setClosing(); // Marca al cliente para que el bucle se rompa
    }
    else
    {
        // Comando desconocido: encolar respuesta de error "421 ERR_UNKNOWNCOMMAND"
        client.queue(":" + server.getName() + " 421 " + client.getNick() + " " + msg.command + " :Unknown command\r\n");
    }
}