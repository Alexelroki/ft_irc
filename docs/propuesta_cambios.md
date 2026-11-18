# Propuesta de cambios para ft_irc (previa a la evaluación)

> Revisión estática de `src/` e `include/`. **No se ha modificado ningún archivo de código.**
> Limitaciones: no se pudo leer el PDF del subject (falta `pdftoppm`), así que se ha contrastado con el subject estándar de ft_irc. Tampoco se ha compilado ni ejecutado nada; los hallazgos salen de leer el código y conviene confirmarlos con las pruebas de la sección E.

Formato de cada punto: **Prioridad · Archivo:función · Problema · Cambio propuesto**.

---

## A. Probables fallos en la evaluación

### A1. MODE solo admite un modo por comando
- **Prioridad:** alta
- **Archivo:** `src/cmdChannel.cpp` → `cmdMode`, `modeInvite`, `modeTopic`, `modeOperator`, `modeKey`, `modeLimit`
- **Problema:** `cmdMode` compara `msg.params[1]` con cadenas exactas (`"+i"`, `"-k"`...). Cualquier combinación (`MODE #c +it`, `+kl clave 5`, `-i+t`, `+o-t nick`) acaba en `ERR_UNKNOWNMODE` (472). Los evaluadores e irssi/HexChat envían a menudo modos combinados.
- **Cambio propuesto:**
  ```
  antes:  if (params[1] == "+i" || "-i") ... else if (== "+t" ...) ... else 472
  después:
    sign = '+'; argIdx = 2; applied = ""; appliedArgs = ""
    para cada c en params[1]:
        '+' / '-'  -> cambia sign
        'i','t'    -> aplica; si cambió algo, añade sign+c a applied
        'k','l','o'-> si (sign=='+' o c=='o') consume params[argIdx++]
                      (461 si falta); aplica; añade sign+c y el argumento
        otro       -> 472 con solo el carácter c; continúa
    si applied no vacío: un único broadcast
        ":prefix MODE #canal <applied> <appliedArgs>\r\n"
  ```
  Las funciones `modeX` pasan a recibir el argumento (o su índice) en lugar de leer `params[2]` fijo, y devuelven si han cambiado algo. Agrupar los signos consecutivos del mensaje difundido (`+it-l`) es opcional.

### A2. Envío parcial y EAGAIN pierden datos o expulsan al cliente
- **Prioridad:** alta
- **Archivo:** `src/Server.cpp` → `_flushClientQueue`
- **Problema:** `if (sent < 0) _closeClient(...)`. En un socket no bloqueante, `send` puede devolver -1 (buffer del kernel lleno) sin que el cliente esté roto: se le desconecta por error. Además, si `sent` es menor que `msg.size()`, el resto del mensaje se descarta y el cliente recibe líneas truncadas. El subject prohíbe mirar `errno` tras `send`, así que no se puede distinguir por ahí.
- **Cambio propuesto:**
  ```
  antes:  msg = pop(); sent = send(...); if (sent < 0) close;
  después:
    msg = front();                      // sin sacar aún
    sent = send(fd, msg, size, MSG_NOSIGNAL);
    si sent <= 0:  break;               // esperar al siguiente POLLOUT (el cierre real lo da POLLHUP/POLLERR o recv)
    si sent < size: sustituir el frente por msg.substr(sent); _outBytes -= sent; break;
    si no: pop();
  ```
  Requiere en `Client` un método para consumir parcialmente el primer mensaje (p. ej. `consumeFront(size_t n)`), ya que hoy solo existen `queue` y `popQueuedMessage`.

### A3. `send()` directo al cerrar, sin pasar por poll
- **Prioridad:** media-alta (el subject exige comprobar disponibilidad con poll antes de send)
- **Archivo:** `src/Server.cpp` → `_handleClientData` (bloque `if (client->isClosing())`), y `src/Client.cpp` → `queue`
- **Problema:** al cerrar (QUIT, contraseña incorrecta) se vacía la cola con `send` inmediato, sin POLLOUT. Además, `Client::queue` pone `_isClosing = true` si se desborda `MAX_OUT_BYTES`, pero ese cliente solo se cierra si luego envía datos (`_handleClientData`); un cliente lento que nunca escribe queda marcado y sin cerrar.
- **Cambio propuesto:**
  - Distinguir "cerrar tras vaciar la cola" (QUIT/ERROR) de "cerrar ya" (desbordamiento).
  - En `start()`, tras el flush de un cliente: `if (client->isClosing() && !client->hasQueuedMessages()) _closeClient(...)`.
  - En `_handleClientData`, al detectar `isClosing`, no enviar: salir del bucle de lectura y dejar que POLLOUT vacíe la cola. Para el caso de desbordamiento, cerrar directamente (descartando la cola).

### A4. `else if` en el bucle de poll y salto de fds
- **Prioridad:** media
- **Archivo:** `src/Server.cpp` → `start`
- **Problema:** `POLLIN` / `POLLOUT` / `POLLHUP|POLLERR` están encadenados con `else if`: si un cliente tiene POLLIN y POLLOUT a la vez solo se atiende la entrada; un cliente que inunda de datos puede retrasar indefinidamente su propia salida. Además, `_closeClient` borra `_pollFds[i]` y el bucle sigue con `i+1`, de modo que el fd que ocupó ese hueco se salta una vuelta (se recupera en el siguiente poll por ser level-triggered, pero es un fallo latente).
- **Cambio propuesto:**
  ```
  for (i = 0; i < size; )
      revents = _pollFds[i].revents
      if (revents == 0) { ++i; continue; }
      si es el socket servidor: _acceptClient(); ++i; continue;
      closed = false
      if (revents & POLLIN)               closed = _handleClientData(fd, i)   // devuelve true si cerró
      if (!closed && (revents & POLLOUT)) closed = _flushClientQueue(fd, i)
      if (!closed && (revents & (POLLHUP|POLLERR|POLLNVAL))) closed = _closeClient(fd, i)
      if (!closed) ++i;                   // si cerró, el siguiente ya está en la posición i
  ```
  Para esto `_handleClientData`, `_flushClientQueue` y `_closeClient` deben informar si cerraron el cliente.

---

## B. Funcionalidad que esperan los clientes reales

### B5. Listas separadas por comas
- **Prioridad:** media
- **Archivo:** `src/cmdChannel.cpp` → `cmdJoin`; `src/commandHandler.cpp` → `handlePrivmsg`; `cmdKick`
- **Problema:** `JOIN #a,#b` falla con 403 porque `isValidName` rechaza la coma; `PRIVMSG a,b :hola` busca un nick literal `a,b`.
- **Cambio propuesto:** añadir `splitCommas(const std::string&)` en `utils` y repetir la lógica actual por cada elemento (en JOIN, emparejando con la lista de claves).

### B6. `NOTICE` no existe
- **Prioridad:** media-baja
- **Archivo:** `src/commandHandler.cpp` → `executeCommand`
- **Problema:** responde 421. Algunos clientes/bots lo usan.
- **Cambio propuesto:** reutilizar `handlePrivmsg` con un parámetro `bool notice` (palabra del mensaje `NOTICE` y sin enviar errores al emisor).

### B7. Comandos que irssi/HexChat envían al conectar
- **Prioridad:** baja (no rompe, pero ensucia la sesión)
- **Archivo:** `src/commandHandler.cpp`, `src/cmdChannel.cpp`
- **Problema:** `WHO #c` → 421; `MODE #c b` → 472; `MODE nick` → 403; `NAMES` → 421.
- **Cambio propuesto:** respuestas mínimas: `WHO` → 315 (fin de lista), `NAMES` → 353+366, `MODE #c b` → 368, `MODE nick` ignorado.

### B8. Detalles de MODE
- **Prioridad:** baja
- **Archivo:** `src/cmdChannel.cpp` → `cmdMode`
- **Problema:** consultar `MODE #c` exige ser miembro (devuelve 442) cuando cualquiera puede consultarlo; el 472 incluye `+x` completo en lugar del carácter de modo.
- **Cambio propuesto:** mover la comprobación de pertenencia solo a la rama que modifica; en 472 enviar solo el carácter.

### B9. Mensajes de JOIN duplicado e INVITE
- **Prioridad:** baja
- **Archivo:** `src/cmdChannel.cpp` → `cmdJoin`, `cmdInvite`
- **Problema:** el 443 de JOIN lleva solo el canal (el formato es `nick #canal`); INVITE exige siempre operador, el RFC solo lo exige si el canal es +i (lo actual es defendible; basta con decidirlo y saber justificarlo en la evaluación).
- **Cambio propuesto:** en JOIN ignorar en silencio o enviar `nick #canal`; en INVITE documentar el criterio elegido.

---

## C. Robustez y limpieza

### C10. Copia superficial en `Server`
- **Archivo:** `include/Server.hpp`, `src/Server.cpp` → copy-ctor y `operator=`
- **Problema:** copian los mapas de `Channel*`/`Client*`, con riesgo de doble `delete` si alguna vez se copia un `Server`.
- **Cambio:** declararlos privados sin implementación (como ya hace `Channel`).

### C11. `std::tolower(char)` con valores negativos y código duplicado
- **Archivo:** `src/Server.cpp` → `findChannel`, `findClient`, `isNickInUse`, `insertMap`, `requestDeleteChannel`
- **Problema:** pasar un `char` negativo a `tolower` es comportamiento indefinido; el mismo bucle está copiado 5+ veces.
- **Cambio:** helper `std::string toLower(const std::string&)` en `utils` (con `unsigned char`) y usarlo en todos. Añadir `#include <cctype>` en `Server.cpp` y `commandHandler.cpp` (hoy compila por inclusiones transitivas).

### C12. `findClient` devuelve clientes sin registrar
- **Archivo:** `src/Server.cpp` → `findClient`
- **Problema:** PRIVMSG/INVITE/KICK pueden dirigirse a alguien a medio registro (con nick pero sin USER).
- **Cambio:** `if (!it->second->isAuthenticated()) continue;` (mantener `isNickInUse` sin ese filtro).

### C13. `test.sh`, `.gitignore`, README
- **Archivo:** `test.sh`, `.gitignore`, `README.md`
- **Cambios:**
  - `HOST=127.0.1.1` → `127.0.0.1` (la primera solo existe en Debian/WSL).
  - Quitar referencias personales ("parte de Kang") antes de entregar.
  - Añadir `.objs/` a `.gitignore`.
  - Ampliar `test.sh` con JOIN/KICK/INVITE/TOPIC/MODE (incluidos modos combinados).

### C14. NICK antes de PASS cierra la conexión
- **Archivo:** `src/commandHandler.cpp` → `handleNick`
- **Nota:** es estricto pero válido; irssi/HexChat envían PASS primero. Solo documentarlo para la defensa.

---

## D. Lo que está bien

- Solo funciones permitidas; `fcntl` únicamente con `F_SETFL, O_NONBLOCK`.
- Un único `poll()` para todo; sockets no bloqueantes; `MSG_NOSIGNAL`.
- Buffering de comandos parciales (`\n`), límites de línea (510), de buffer de entrada (4096) y de cola de salida (64 KB).
- Cierre limpio con Ctrl+C / Ctrl+\ y liberación de canales y clientes.
- Sin punteros colgantes al cerrar un cliente (se elimina de miembros, operadores e invitados, y se borran canales vacíos).
- Makefile correcto (`-Wall -Wextra -Werror -std=c++98`, dependencias `-MMD`, sin relink).

---

## E. Orden sugerido y verificación

**Orden:** A2 → A3 → A4 → A1 → B → C.

**Verificación (cuando se apliquen los cambios):**
1. `make re` con los flags del subject.
2. Compilar una copia con `-fsanitize=address,undefined` y pasar `valgrind --leak-check=full ./ircserv 6667 pass`.
3. `./test.sh 6667 pass` y los nuevos tests de canales/modos.
4. Manual con `nc -C 127.0.0.1 6667`: comandos partidos (`com^Dman^Dd`), `MODE #c +itk clave`, `+kl k 5`, `-i+t`, cliente suspendido con Ctrl+Z mientras otros hablan, Ctrl+C del servidor.
5. Cliente real (irssi o HexChat) con contraseña: join, modos, kick, invite, topic.
