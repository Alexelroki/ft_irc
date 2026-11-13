# Lo que necesito de Alex y Darío (parte de Kang)

Revisado sobre el zip `ft_irc_dc2463`: rama `develop/parser`, con `alex/socket-server` (commit `d9ff03b`) sin fusionar.
Aquí solo va lo que **todavía no está en el proyecto**. Mi parte (`Client`, `Parser`, `CommandHandler`, `Replies`) está terminada
y compila; lo que sigue bloquea que enlace, que se pueda probar o que funcione de verdad.

---

## Alex (Server)

### Bloqueantes: sin esto no compila, no enlaza o no se puede probar

1. **`bool isNickInUse(const std::string &nick, const Client *ignore) const;`**
   - La uso en `handleNick`. No existe ni en `develop/parser` ni en `alex/socket-server`.
   - Devuelve `true` si algún cliente distinto de `ignore` ya usa ese nick, **sin distinguir mayúsculas** (`Kang` = `kang`).
   - `ignore` es el propio cliente que cambia de nick, para que pueda cambiar solo las mayúsculas.
   - La normalización debe vivir dentro de `Server`, en un único sitio (como dice el README).

2. **`Client *findClient(const std::string &nick);`**
   - Hoy llamo a `findClientByNick`; la renombro a `findClient`, que es el nombre que ya usan Darío y tu rama.
   - En `alex/socket-server` es un stub que devuelve `NULL` con el comentario "hasta que Kang entregue Client con getNick()".
     `Client` ya está en `develop/parser`: hay que implementarlo de verdad (buscar en `clients_`, sin distinguir mayúsculas, `NULL` si no existe).

3. **Envío de datos (`send()` / `POLLOUT`).** Ahora mismo `Server` nunca llama a `send()`: nada de lo que se encola con `queue()` llega al cliente.
   - Después de procesar los comandos hay que vaciar la cola de **todos** los clientes que tengan algo pendiente, no solo la del que habló.
     Un PRIVMSG a un usuario encola en la cola del *destinatario*.
   - Cuando un cliente está marcado con `isClosing()`, vaciar su cola **antes** de `closeClient_`, para que llegue el `ERROR :Closing Link...`
     (QUIT, contraseña incorrecta, username inválido). Si se cerró por desbordar la cola, no hace falta.
   - Si `send()` manda solo una parte, `popQueuedMessage()` ya ha sacado el mensaje entero y no hay forma de devolver el resto.
     Si lo necesitas, **yo añado** a `Client` una función para mirar el primer mensaje sin sacarlo y otra para consumir solo N bytes. Dímelo.

4. **`setHost` en `acceptClient_`:** `client->setHost(inet_ntoa(clientAddr.sin_addr));`
   Hoy el host queda vacío y el prefijo sale como `nick!user@` (afecta también a los JOIN/PART/KICK de Darío).

5. **`getName()` como única fuente del nombre del servidor.** Existe en `alex/socket-server` (devuelve `"ircserv"`) pero falta fusionar, y esa rama
   ya no define `SERVER_NAME`. Cuando esté en `develop`, yo cambio mis usos de `SERVER_NAME` por `server.getName()`.

### Para que QUIT y NICK avisen a los canales

6. **`void broadcastToSharedChannels(Client &client, const std::string &line);`**
   - Envía `line` **una sola vez** a cada cliente distinto que comparta algún canal con `client`, **sin incluir a `client`**.
   - Hay que deduplicar: quien comparta dos canales con él debe recibir la línea una vez. `Channel::broadcast` de Darío va canal por canal
     y duplicaría, así que `Server` debe juntar los destinatarios en un `std::set<Client*>`.
   - La usaré yo en `handleNick` (`:viejo!user@host NICK :nuevo`).
   - `Server` la usará al cerrar un cliente para el QUIT (`:nick!user@host QUIT :Quit: motivo`), con `client.getQuitReason()`.
     Si el cliente cayó sin enviar QUIT el motivo viene vacío y `Server` pone el texto que prefiera (por ejemplo "Connection closed").
   - Para QUIT debe ejecutarse **antes** de sacar al cliente de los canales.

7. **Limpieza al cerrar un cliente (`closeClient_`).**
   - Llamar a `removeClient(client)` en **todos** los canales, sin filtrar con `hasMember`, y antes del `delete client`
     (si no, queda un puntero colgante en `_guests`). Esto ya lo dejó anotado Darío.
   - Quitar al cliente de `clients_` y liberarlo; pedir el borrado de los canales que queden vacíos.
   - En `alex/socket-server`, `closeClient_` solo cierra el fd y borra `clientBuffers_`; no toca canales ni libera el `Client`.

### Integración de ramas

8. **Fusionar `develop/parser` y `alex/socket-server`.** Divergen desde `ccaab86` y ambas tocan `Server.hpp` y `Server.cpp`.
   De mi lado hay que conservar: `Client*` en `clients_`, el bloque de `handleClientData_` (con el límite de 4096 ya corregido, `parseMessage` y `executeCommand`)
   y los includes de `Message.hpp`, `Client.hpp` y `CommandHandler.hpp` que la rama de Alex no tiene en `Server.hpp`.

9. **Makefile:** faltan en `SRCS` `Replies.cpp` (mío) y `cmdChannel.cpp` (de Darío). Sin ellos falla el enlazado con `sendNumeric` y los `cmd*` sin definir.

---

## Darío (canales)

1. **`cmdChannel.hpp`: cambiar `class Message;` por `struct Message;`.** `Message` es un `struct`. Con g++ no da error, pero clang lo avisa
   (`-Wmismatched-tags`) y con `-Werror` rompería la compilación.
2. **Numéricos unificados (recomendable):** usar `sendNumeric` de `include/Replies.hpp` en lugar de `reply()`. Así los textos coinciden con los míos
   (hoy el 401 dice cosas distintas) y no hay que repetir el formato. Para los errores de MODE, el "400" no es un numérico estándar;
   en `Replies.hpp` ya están `ERR_UNKNOWNMODE` (472) y `ERR_INVALIDMODEPARAM` (696).
3. **Saltos de línea:** casi todos sus `.cpp` están en CRLF y los míos en LF, y no hay `.gitattributes`. Conviene acordar uno
   para que los diffs y las fusiones no se llenen de ruido.

Los `cmd*` que llamo desde el dispatcher (`cmdJoin`, `cmdPart`, `cmdKick`, `cmdInvite`, `cmdPrivmsg`, `cmdTopic`, `cmdMode`) ya existen y tienen la firma
que espero. De Darío no me falta nada más.

---
