*This project has been created as part of the 42 curriculum by albarrei, kanye, dponce-g.*

# ft_irc

## Description

**ft_irc** is an IRC (Internet Relay Chat) server written in **C++98**. It runs as a single process, accepts many simultaneous TCP clients and lets them authenticate, choose a nickname, talk privately, join channels and manage them as channel operators. A standard IRC client can connect to it as it would to any other IRC server.

The goal of the project is to understand how a network server works at a low level: sockets, non-blocking I/O, multiplexing with `poll()`, message framing over a byte stream, and the design of a text protocol. No external library and no Boost are used.

### Features

- Password-protected server (`PASS`), nickname (`NICK`) and user (`USER`) registration.
- Private messages between users and messages to channels (`PRIVMSG`).
- Channels with regular users and **operators**.
- Operator commands: `KICK`, `INVITE`, `TOPIC` and `MODE` with these channel modes:

| Mode | Meaning |
|------|---------|
| `i` | Invite-only channel |
| `t` | `TOPIC` can only be changed by operators |
| `k` | Channel key (password) |
| `o` | Give / take operator privilege |
| `l` | User limit |

- Also supported: `PART`, `QUIT`, `PING`/`PONG` and a minimal `CAP` (no capability is offered).
- Partial packets are reassembled: a command can arrive in several pieces and it is only executed when the line is complete.
- Nicknames and channel names are case-insensitive (`#Dev` and `#dev` are the same channel).

## Instructions

### Requirements

A Linux (or macOS) machine with a C++ compiler (`c++`) and `make`. The code compiles with `-Wall -Wextra -Werror -std=c++98`.

### Build

```bash
make        # builds ./ircserv
make clean  # removes the object files
make fclean # removes the object files and the executable
make re     # fclean + all
```

### Run

```bash
./ircserv <port> <password>
```

- `port`: TCP port to listen on, between 1024 and 65535.
- `password`: connection password required from every client (cannot be empty).

Example:

```bash
./ircserv 6667 mypassword
```

Stop the server with `Ctrl+C`: it closes every descriptor and frees all the memory before exiting.

### Connect

With a real IRC client (the server password is sent as the connection password):

```bash
hexchat -c 127.0.0.1 -p 6667 -w mypassword -n mynick
```

Or by hand with `nc`. The `-C` option sends `\r\n` as line ending:

```bash
nc -C 127.0.0.1 6667
PASS mypassword
NICK alice
USER alice 0 * :Alice Example
JOIN #42
PRIVMSG #42 :hello everyone
```

To check that fragmented commands are rebuilt correctly, type part of a command and send it with `Ctrl+D`, then finish it:

```bash
nc -C 127.0.0.1 6667
PA^DSS my^Dpassword
```

### Supported commands

| Command | Syntax | Notes |
|---------|--------|-------|
| `PASS` | `PASS <password>` | Must be sent before `NICK` and `USER`. A wrong password closes the connection. |
| `NICK` | `NICK <nickname>` | Up to 30 characters. Can be changed after registration (the change is announced to the channels you share). |
| `USER` | `USER <username> <mode> <unused> :<realname>` | Completes the registration. |
| `QUIT` | `QUIT [:<reason>]` | Disconnects and tells the channels you were in. |
| `PING` | `PING <token>` | Answered with `PONG`. |
| `CAP` | `CAP LS` / `LIST` / `REQ` | Minimal support: no capability is offered. |
| `PRIVMSG` | `PRIVMSG <nick\|#channel> :<text>` | To a user or to a channel you are in. |
| `JOIN` | `JOIN <#channel> [<key>]` | Creates the channel if it does not exist; the creator becomes operator. |
| `PART` | `PART <#channel> [:<reason>]` | Leaves the channel. An empty channel is destroyed. |
| `KICK` | `KICK <#channel> <nick> [:<reason>]` | Operators only. |
| `INVITE` | `INVITE <nick> <#channel>` | Operators only. The invitation is used (and removed) when the guest joins. |
| `TOPIC` | `TOPIC <#channel> [:<topic>]` | Without a topic argument it shows the current topic. With mode `+t`, only operators can change it. |
| `MODE` | `MODE <#channel> [<+\|-><mode> [<parameter>]]` | Without arguments it shows the current modes. Changing modes requires being an operator. |

`MODE` examples:

```
MODE #42 +i               invite-only on
MODE #42 -t               topic free for everybody
MODE #42 +k secret        set the key
MODE #42 +l 20            limit of 20 users
MODE #42 +o bob           give operator to bob
```

Every reply follows the usual IRC format (`:ft_irc <numeric> <nick> <params> :<text>`), for example `403` (no such channel), `442` (you are not on that channel), `461` (not enough parameters), `471`/`473`/`475` (channel full / invite-only / bad key) and `482` (you are not a channel operator).

## Technical choices

- **One `poll()` for everything.** The listening socket and every client socket live in the same `poll()` set. All descriptors are non-blocking (`fcntl(fd, F_SETFL, O_NONBLOCK)`), no `fork` is used, and the server never reads or writes a socket outside `poll()`.
- **Output queues.** Command handlers never call `send()`. They only append text to the output queue of each recipient (`Client::queue()`); the server asks `poll()` for `POLLOUT` only when a client has data waiting and sends it from there.
- **Message framing.** TCP is a byte stream, so each client has an input buffer. A command is parsed only when a full line has been received, which also handles commands that arrive in pieces or several commands in one packet. Lines are limited to 510 bytes.
- **Layers.** `Server` owns the sockets, the clients and the channels; the parser turns a line into a `Message { prefix, command, params }`; `executeCommand` dispatches it; the handlers (`commandHandler.cpp` for registration and private messages, `cmdChannel.cpp` for channel commands) validate, change the state and reply. `Channel` protects its own invariants (for example, an operator is always a member, and the key and the limit are never invalid).
- **Single place per shared rule.** Numeric replies are built in one function (`sendNumeric`), case conversion lives in `toLower`/`toUpper` in `utils`, and name normalization happens inside `Server` lookups.
- **Ownership.** `Server` owns every `Client` and `Channel`; channels keep plain pointers to their members and never delete them. When a client disconnects it is announced first, then removed from every channel, and finally deleted. `Server` is not copyable (copying it would duplicate the ownership of sockets and heap objects).
- **Operators.** The creator of a channel is its first operator. If the last operator leaves (`PART`, `KICK` or disconnection) and members remain, one of them is promoted and the channel is told with a `MODE +o`, so that clients display the same state as the server. Removing the operator status with `MODE -o` is a deliberate action and is never undone automatically.
- **Limits.** Nicknames: 30 characters. Channel names: start with `#` or `&`, up to 50 characters, no spaces, commas, `:` or control characters. Topic: truncated to 100 bytes. User limit (`+l`): from 1 to 250. Username: 18 characters. Real name: 50 characters.
- **Error safety.** Allocation failures are not swallowed: a handler that cannot finish cleans what it created and rethrows, and the server disconnects the client that caused it instead of crashing.

## Project structure

```
.
├── Makefile
├── include/    Channel.hpp Client.hpp Server.hpp Message.hpp
│               cmdChannel.hpp commandHandler.hpp replies.hpp utils.hpp
└── src/        main.cpp Server.cpp Client.cpp Channel.cpp parser.cpp
                commandHandler.cpp cmdChannel.cpp replies.cpp utils.cpp
```

## Known limitations

- `MODE` handles **one mode change per command** (`MODE #42 +i`, then `MODE #42 +t`); combined forms such as `MODE #42 +it` are not supported yet.
- Not implemented: `NOTICE`, `WHO`, `NAMES`, `LIST`, `WHOIS`, comma-separated target lists (`JOIN #a,#b`), server-to-server links and user modes.
- `NICK` sent before a correct `PASS` closes the connection. Clients that send `PASS` first (irssi, HexChat, WeeChat) are not affected.

## Resources

### References

- RFC 1459 – Internet Relay Chat Protocol: https://datatracker.ietf.org/doc/html/rfc1459
- RFC 2810, 2811, 2812, 2813 – IRC architecture, channel management, client protocol and server protocol: https://datatracker.ietf.org/doc/html/rfc2812
- Modern IRC Client Protocol documentation: https://modern.ircdocs.horse/
- Beej's Guide to Network Programming: https://beej.us/guide/bgnet/
- Linux manual pages: `poll(2)`, `socket(2)`, `bind(2)`, `listen(2)`, `accept(2)`, `recv(2)`, `send(2)`, `fcntl(2)`, `setsockopt(2)`, `signal(2)`
- cppreference – `std::tolower` (the argument must be representable as `unsigned char`): https://en.cppreference.com/w/cpp/string/byte/tolower
- The 42 ft_irc subject.

### Use of AI

AI assistants (Claude and Gemini) were used as a **tutor and reviewer**, not as a code generator for the project:

- **Learning and explanations:** C++98 concepts needed for the project (ownership and RAII, references vs. pointers, `const` correctness, exception safety with `new`, undefined behaviour of `std::tolower` with negative `char`, non-copyable classes in C++98) and how the IRC protocol and its numeric replies work.
- **Design discussions:** structure, validation order of each command, and which numeric reply fits each error.
- **Code review:** the team's code was reviewed to find duplicated code, memory-ownership problems, protocol mistakes and edge cases; each proposed change was discussed and applied by hand by the team.
- **Test planning:** checklists of `nc` and IRC-client scenarios (fragmented input, abrupt disconnections, operator leaving a channel, mode and invitation cases).

All the code was written, tested and is understood by the members of the team, who can explain every part of it. AI suggestions were always checked against the subject, the RFCs and real behaviour before being used.