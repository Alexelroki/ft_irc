/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kanye <kanye@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/01 11:20:54 by kanye             #+#    #+#             */
/*   Updated: 2026/11/02 00:46:33 by kanye            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <queue>

class Client {
private:
    int                     _fd;
    std::string             _nickname;
    std::string             _username;
    std::string             _realname;
    std::string             _host;        

    std::string             _inBuffer;//DATA FROM THE CLIENT
    std::queue<std::string> _outQueue;//MESSAGES THE SERVER WANTS TO SEND TO THE CLIENT

    //Autentication flags
    bool                    _isAuthenticated;
    bool                    _hasUser;
    bool                    _hasPassed;


    bool                    _isClosing;

public:

    //Orthodox Canonical Form
    Client(int fd);
    Client(const Client& other);
    Client& operator=(const Client& other);
    ~Client();

    //Helpers
    int                 getFd() const;
    void                appendBuffer(const std::string& data); //Add new data to the client's existing buffer
    bool                hasCompleteMessage() const;
    std::string         extractMessage();
    size_t              getBufferSize() const;

    //Methods created to send data
    void                queue(const std::string& msg);
    bool                hasQueuedMessages() const;
    std::string         popQueuedMessage();

    //Getters and Setters
    const std::string&  getNick() const;
    const std::string&  getUsername() const;
    const std::string&  getRealname() const;
    const std::string&  getHost() const;
    std::string         prefix() const; 

    //Setters
    void                setNick(const std::string& nickname);
    void                setUsername(const std::string& username);
    void                setRealname(const std::string& realname);
    void                setHost(const std::string& host);
    void                setPassed();
    void                setUserDone();
    void                setAuthenticated();

    
    void                setClosing();
    bool                isClosing() const;


    //Autentication
    bool                hasNick() const;
    bool                hasUser() const;
    bool                isAuthenticated() const;
    bool                hasPassed() const;
    bool                isReadyForWelcome() const;
};

#endif