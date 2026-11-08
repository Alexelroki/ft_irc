/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kanye <kanye@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/01 21:32:01 by kanye             #+#    #+#             */
/*   Updated: 2026/11/08 21:52:36 by kanye            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef COMMANDHANDLER_HPP
# define COMMANDHANDLER_HPP

class Server;
class Client;
struct Message;

void executeCommand(Server& server, Client& client, const Message& msg);

#endif