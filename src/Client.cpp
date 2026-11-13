/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kanye <kanye@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/01 11:20:33 by kanye             #+#    #+#             */
/*   Updated: 2026/11/09 00:51:36 by kanye            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Client.hpp"

// Max bytes allowed waiting in the out queue. Past this the client is too slow
static const size_t MAX_OUT_BYTES = 65536;

//ORTHODOX CANONICAL FORM
Client::Client(int fd)
    : _fd(fd),
      _nickname(""),
      _username(""),
      _realname(""),
      _host(""),
      _inBuffer(""),
      _outBytes(0),
      _quitReason(""),
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
      _outBytes(other._outBytes),
      _quitReason(other._quitReason),
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
        _outBytes = other._outBytes;
        _quitReason = other._quitReason;
        _isAuthenticated = other._isAuthenticated;
        _hasUser = other._hasUser;
        _hasPassed = other._hasPassed;
        _isClosing = other._isClosing;
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
 
//A message is complete when the buffer contains a '\n'
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

//If the message does not fit under MAX_OUT_BYTES it is dropped and closing
void Client::queue(const std::string& msg)
{
    if (_outBytes + msg.size() > MAX_OUT_BYTES)
    {
        _isClosing = true;
        return ;
    }
    _outQueue.push(msg);
    _outBytes += msg.size();
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
    _outBytes -= msg.size();
    return (msg);
}

size_t Client::getQueuedBytes() const
{
    return (_outBytes);
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

const std::string& Client::getQuitReason() const
{
    return (this->_quitReason);
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
 
void Client::setQuitReason(const std::string& reason)
{
    this->_quitReason = reason;
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

//PASS + NICK + USER
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