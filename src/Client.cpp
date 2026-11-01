/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kanye <kanye@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/01 11:20:33 by kanye             #+#    #+#             */
/*   Updated: 2026/11/02 00:46:48 by kanye            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Client.hpp"


//ORTHODOX CANONICAL FORM
Client::Client(int fd) //New
    : _fd(fd),
      _nickname(""),
      _username(""),
      _realname(""),
      _host(""),
      _inBuffer(""),
      _isAuthenticated(false),
      _hasUser(false),
      _hasPassed(false),
      _isClosing(false)
{
}


Client::Client(const Client& other) //Copy
    : _fd(other._fd),
      _nickname(other._nickname),
      _username(other._username),
      _realname(other._realname),
      _host(other._host),
      _inBuffer(other._inBuffer),
      _outQueue(other._outQueue),
      _isAuthenticated(other._isAuthenticated),
      _hasUser(other._hasUser),
      _hasPassed(other._hasPassed),
      _isClosing(other._isClosing)
{
}

Client& Client::operator=(const Client& other) //Copy assigment
{
    if (this != &other)
    {
        _fd = other._fd;
        _nickname = other._nickname;
        _username = other._username;
        _realname = other._realname;
        _host = other._host;
        _inBuffer = other._inBuffer;
        _outQueue = other._outQueue;
        _isAuthenticated = other._isAuthenticated;
        _hasUser = other._hasUser;
        _hasPassed = other._hasPassed;
    }
    return (*this);
}

Client::~Client()
{
}


//HELPERS FUNCTIONS

int Client::getFd() const
{
    return (_fd);
}

void Client::appendBuffer(const std::string& data)
{
    _inBuffer += data;
}
 
//A message is complete when the buffer contains a '\n' (works for "\r\n" too)
bool Client::hasCompleteMessage() const
{
    return (_inBuffer.find('\n') != std::string::npos);
}
 
//Return first complete line
std::string Client::extractMessage()
{
    std::string::size_type  pos = _inBuffer.find('\n');
 
    if (pos == std::string::npos)
        return ("");
 
    std::string line = _inBuffer.substr(0, pos);
    _inBuffer.erase(0, pos + 1);
 
    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);
    return (line);
}


//SEND DATA FUNCTIONS

void Client::queue(const std::string& msg)
{
    _outQueue.push(msg);
}

bool Client::hasQueuedMessages() const
{
    return (!_outQueue.empty());
}

std::string Client::popQueuedMessage()
{
    if (_outQueue.empty())
        return ("");
    std::string msg = _outQueue.front();
    _outQueue.pop();
    return (msg);
}


//GETTERS
const std::string& Client::getNick() const
{
    return this->_nickname;
}
 
const std::string& Client::getUsername() const
{
    return (this->_username);
}

const std::string& Client::getRealname() const
{
    return (this->_realname);
}

const std::string& Client::getHost() const
{
    return (this->_host);
}

//"nick!user@host"
std::string Client::prefix() const
{
    return (this->_nickname + "!" + this->_username + "@" + this->_host);
}



//SETTERS
void Client::setNick(const std::string& nickname)
{
    this->_nickname = nickname;
}
 
void Client::setUsername(const std::string& username)
{
    this->_username = username;
}
 
void Client::setRealname(const std::string& realname)
{
    this->_realname = realname;
}
 
void Client::setHost(const std::string& host)
{
    this->_host = host;
}
 
void Client::setPassed()
{
    this->_hasPassed = true;
}
 
void Client::setUserDone()
{
    this->_hasUser = true;
}
 
void Client::setAuthenticated()
{
    this->_isAuthenticated = true;
}
 

//AUTHENTICATION FUNCTIONS
bool Client::hasNick() const
{
    return (!this->_nickname.empty());
}

bool Client::hasUser() const
{
    return (this->_hasUser);
}

bool Client::isAuthenticated() const
{
    return (this->_isAuthenticated);
}

bool Client::hasPassed() const
{
    return (this->_hasPassed);
}

//PASS + NICK + USER are all done and the welcome (001) has not been sent yet.

bool Client::isReadyForWelcome() const
{
    return (this->_hasPassed && hasNick() && this->_hasUser && !this->_isAuthenticated);
}



bool Client::isClosing() const
{
    return (_isClosing);
}

void Client::setClosing()
{
    _isClosing = true;
}

size_t Client::getBufferSize() const
{
    return (this->_inBuffer.size());
}