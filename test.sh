#!/bin/bash
# Pruebas de autenticacion y comandos basicos de ft_irc (parte de Kang).
# Uso:   1) arranca el servidor:  ./ircserv 6667 pass
#        2) en otra terminal:     ./test.sh [puerto] [password]
# Solo usa bash (/dev/tcp), no hace falta nc.

HOST=127.0.1.1
PORT=${1:-6667}
PASS=${2:-pass}
PASSED=0
FAILED=0
T=0.4
[ "${BASH_VERSINFO[0]}" -lt 4 ] && T=1      # bash 3 (macOS) no admite read -t fraccionario
ID=$$                                        # para que los nicks no choquen entre ejecuciones
trap '' PIPE                                  # escribir en una conexion cerrada por el servidor no debe matar el script

# ---------- utilidades ----------
open_fd()  { eval "exec $1<>/dev/tcp/$HOST/$PORT" 2>/dev/null || { echo "No puedo conectar a $HOST:$PORT. Esta arrancado el servidor?"; exit 2; }; }
close_fd() { eval "exec $1>&-"; }
send()     { printf '%b' "$2" >&"$1" 2>/dev/null; }   # send FD 'texto con \r\n' (si el servidor ya cerro, no pasa nada)
# read_fd FD: devuelve todo lo recibido hasta que no llega nada mas (sin \r).
read_fd()
{
    local line out=""
    while IFS= read -r -t "$T" -u "$1" line; do out+="${line%$'\r'}"$'\n'; done
    printf '%s' "$out"
}
# conexion_cerrada FD: true si el servidor la cerro (EOF). Si solo hay silencio, false.
conexion_cerrada() { IFS= read -r -t "$T" -u "$1" _; [ $? -eq 1 ]; }
ok()   { PASSED=$((PASSED + 1)); echo "  OK    $1"; }
bad()  { FAILED=$((FAILED + 1)); echo "  FALLO $1"; [ -n "$2" ] && printf '        recibido: %s\n' "$(printf '%s' "$2" | tr '\n' '|')"; }
has()  { if printf '%s' "$2" | grep -qF -- "$3"; then ok "$1"; else bad "$1" "$2"; fi; }
hasnt(){ if printf '%s' "$2" | grep -qF -- "$3"; then bad "$1" "$2"; else ok "$1"; fi; }
count_is() { local n; n=$(printf '%s' "$2" | grep -cF -- "$3"); if [ "$n" -eq "$4" ]; then ok "$1"; else bad "$1 (esperado $4, hay $n)" "$2"; fi; }
closed()    { if conexion_cerrada "$1"; then ok "$2"; else bad "$2 (la conexion sigue abierta)"; fi; }
stays_open(){ if conexion_cerrada "$1"; then bad "$2 (el servidor cerro la conexion)"; else ok "$2"; fi; }
# register FD NICK: registro completo y descarta la bienvenida
register() { send "$1" "PASS $PASS\r\nNICK $2\r\nUSER u$2 0 * :Real $2\r\n"; read_fd "$1" >/dev/null; }
title()    { echo; echo "[$1]"; }

# ---------- comprobacion previa ----------
open_fd 3
send 3 "PASS $PASS\r\nNICK pre${ID}\r\nUSER pre 0 * :pre\r\n"
out=$(read_fd 3)
close_fd 3
if ! printf '%s' "$out" | grep -qF " 001 pre${ID} "; then
    echo "No llega la bienvenida (001) en un registro normal. Recibido: '$(printf '%s' "$out" | tr '\n' '|')'"
    echo "Comprueba: 1) la contrasena (segundo argumento)  2) que el servidor ya envia datos al cliente (cola de envio con send/POLLOUT)."
    exit 2
fi

# ---------- pruebas ----------
title "1. Registro correcto"
open_fd 3; N="a${ID}"
send 3 "PASS $PASS\r\nNICK $N\r\nUSER user 0 * :Real Name\r\n"
out=$(read_fd 3)
has "001 con el prefijo nick!user@host" "$out" " 001 $N :Welcome to the IRC Network $N!user@"
has "002" "$out" " 002 $N "
has "003" "$out" " 003 $N "
has "004" "$out" " 004 $N "
stays_open 3 "la conexion sigue abierta tras registrarse"
close_fd 3

title "2. Registro con las lineas por separado y en otro orden (PASS, USER, NICK)"
open_fd 3; N="b${ID}"
send 3 "PASS $PASS\r\n"; sleep 0.2; send 3 "USER user 0 * :Real\r\n"; sleep 0.2; send 3 "NICK $N\r\n"
out=$(read_fd 3); has "001 al llegar la ultima pieza" "$out" " 001 $N "; close_fd 3

title "3. Contrasena incorrecta"
open_fd 3
send 3 "PASS mal\r\nNICK c${ID}\r\nUSER u 0 * :r\r\n"
out=$(read_fd 3)
count_is "un unico 464" "$out" " 464 " 1
has "ERROR Closing Link" "$out" "ERROR :Closing Link:"
hasnt "no hay bienvenida" "$out" " 001 "
closed 3 "el servidor cierra la conexion"
close_fd 3

title "4. NICK y USER sin PASS"
open_fd 3; send 3 "NICK d${ID}\r\nUSER u 0 * :r\r\n"; out=$(read_fd 3)
has "464" "$out" " 464 "; closed 3 "cierra la conexion"; close_fd 3

title "5. Comandos antes de registrarse"
open_fd 3
send 3 "PRIVMSG alguien :hola\r\n"; out=$(read_fd 3); has "PRIVMSG sin registrar -> 451" "$out" " 451 * :You have not registered"
send 3 "JOIN #x\r\n";              out=$(read_fd 3); has "JOIN sin registrar -> 451" "$out" " 451 "
send 3 "FOO\r\n";                  out=$(read_fd 3); has "comando desconocido sin registrar -> 451" "$out" " 451 "
send 3 "PING abc\r\n";             out=$(read_fd 3); has "PING si esta permitido" "$out" "PONG ft_irc :abc"
close_fd 3

title "6. NICK"
open_fd 3; open_fd 4; A="e${ID}"
register 3 "$A"
send 4 "PASS $PASS\r\n"; read_fd 4 >/dev/null
send 4 "NICK\r\n";                 out=$(read_fd 4); has "sin nick -> 431" "$out" " 431 * :No nickname given"
send 4 "NICK 1malo\r\n";           out=$(read_fd 4); has "nick que empieza por numero -> 432" "$out" " 432 * 1malo :Erroneous nickname"
send 4 "NICK mal@nick\r\n";        out=$(read_fd 4); has "nick con @ -> 432" "$out" " 432 "
send 4 "NICK $A\r\n";              out=$(read_fd 4); has "nick repetido -> 433" "$out" " 433 * $A :Nickname is already in use"
send 4 "NICK $(echo "$A" | tr a-z A-Z)\r\n"; out=$(read_fd 4); has "nick repetido en mayusculas -> 433" "$out" " 433 "
send 4 "NICK $(printf 'z%.0s' {1..40})\r\n"; out=$(read_fd 4); has "nick de 40 caracteres -> 432" "$out" " 432 "
close_fd 4

title "7. USER"
open_fd 4; send 4 "PASS $PASS\r\n"; read_fd 4 >/dev/null
send 4 "USER solo dos\r\n";        out=$(read_fd 4); has "pocos parametros -> 461" "$out" " 461 * USER :Not enough parameters"
send 4 "USER ma@l 0 * :x\r\n";     out=$(read_fd 4); has "username con @ -> ERROR" "$out" "ERROR :Closing Link:"; closed 4 "y cierra"
close_fd 4
open_fd 4; send 4 "PASS $PASS\r\nNICK f${ID}\r\nUSER abcdefghijklmnopqrstuvwxyz 0 * :x\r\n"; out=$(read_fd 4)
has "username de 26 caracteres se recorta, no se rechaza" "$out" "f${ID}!abcdefghijklmnopqr@"; close_fd 4

title "8. Ya registrado"
send 3 "PASS $PASS\r\n";            out=$(read_fd 3); has "PASS otra vez -> 462" "$out" " 462 $A :You may not reregister"
send 3 "USER x 0 * :y\r\n";        out=$(read_fd 3); has "USER otra vez -> 462" "$out" " 462 "
send 3 "FOO bar\r\n";              out=$(read_fd 3); has "comando desconocido -> 421 con el nombre" "$out" " 421 $A FOO :Unknown command"
send 3 "foo\r\n";                  out=$(read_fd 3); has "comando en minusculas se normaliza a mayusculas" "$out" " 421 $A FOO "

title "9. Cambio de nick ya registrado"
send 3 "NICK ${A}x\r\n";           out=$(read_fd 3); has "eco de NICK con el prefijo antiguo" "$out" ":$A!u$A@"; has "con el nick nuevo" "$out" " NICK :${A}x"
A="${A}x"

title "10. PING, PONG y CAP"
send 3 "PING\r\n";                 out=$(read_fd 3); has "PING sin parametro -> 409" "$out" " 409 $A :No origin specified"
send 3 "PONG x\r\n";               out=$(read_fd 3); [ -z "$out" ] && ok "PONG no responde" || bad "PONG no responde" "$out"
open_fd 4
send 4 "CAP LS 302\r\n";           out=$(read_fd 4); has "CAP LS -> lista vacia" "$out" "CAP * LS :"
send 4 "CAP REQ :multi-prefix\r\n";out=$(read_fd 4); has "CAP REQ -> NAK" "$out" "NAK :multi-prefix"
send 4 "CAP END\r\n";              out=$(read_fd 4); [ -z "$out" ] && ok "CAP END no responde" || bad "CAP END no responde" "$out"
send 4 "CAP\r\n";                  out=$(read_fd 4); has "CAP sin parametros -> 461" "$out" " 461 * CAP :Not enough parameters"
close_fd 4

title "11. PRIVMSG entre usuarios"
open_fd 4; B="g${ID}"; register 4 "$B"
send 3 "PRIVMSG $B :hola que tal\r\n"; out=$(read_fd 4)
has "B recibe el mensaje con el prefijo de A" "$out" ":$A!u${A%x}@"; has "con el texto completo" "$out" "PRIVMSG $B :hola que tal"
out=$(read_fd 3); [ -z "$out" ] && ok "A no recibe eco" || bad "A no recibe eco" "$out"
send 3 "PRIVMSG $(echo "$B" | tr a-z A-Z) :mayus\r\n"; out=$(read_fd 4); has "el destinatario se busca sin distinguir mayusculas" "$out" "PRIVMSG $B :mayus"
send 3 "PRIVMSG nadie${ID} :x\r\n"; out=$(read_fd 3); has "destino inexistente -> 401" "$out" " 401 $A nadie${ID} :No such nick/channel"
send 3 "PRIVMSG $B\r\n";           out=$(read_fd 3); has "sin texto -> 412" "$out" " 412 $A :No text to send"
send 3 "PRIVMSG\r\n";              out=$(read_fd 3); has "sin destinatario -> 411" "$out" " 411 $A :No recipient given (PRIVMSG)"
send 3 "JOIN #prueba${ID}\r\n";    out=$(read_fd 3); hasnt "JOIN llega al codigo de canales (no es 421/451)" "$out" " 421 "
close_fd 4

title "12. QUIT"
open_fd 4; C="h${ID}"; register 4 "$C"
send 4 "QUIT :hasta luego\r\n";    out=$(read_fd 4)
has "ERROR con el motivo" "$out" "Closing Link: 127.0.0.1 (Quit: hasta luego)"; closed 4 "cierra la conexion"; close_fd 4
open_fd 4; D="i${ID}"; register 4 "$D"
send 4 "QUIT\r\n";                 out=$(read_fd 4); has "QUIT sin motivo" "$out" "(Quit: Client Quit)"; close_fd 4
open_fd 4; send 4 "QUIT\r\n";      out=$(read_fd 4); has "QUIT sin haberse registrado" "$out" "ERROR :Closing Link:"; close_fd 4
open_fd 4; E="j${ID}"; register 4 "$E"
send 4 "QUIT\r\nPRIVMSG $A :no debe llegar\r\n"; read_fd 4 >/dev/null; out=$(read_fd 3)
hasnt "lo que llega tras QUIT en el mismo paquete se ignora" "$out" "no debe llegar"; close_fd 4
send 3 "PRIVMSG $C :x\r\n";        out=$(read_fd 3); has "el nick del que salio queda libre (401)" "$out" " 401 "

title "13. Entradas raras (el servidor no debe caerse)"
open_fd 4; F="k${ID}"; register 4 "$F"
send 4 "NI"; sleep 0.3; send 4 "CK ${F}2\r\nPI"; sleep 0.3; send 4 "NG trozo\r\n"; out=$(read_fd 4)
has "comando partido en tres paquetes" "$out" " NICK :${F}2"; has "PING partido" "$out" "PONG ft_irc :trozo"
send 4 "PING solo-lf\n";           out=$(read_fd 4); has "linea terminada solo en \\n" "$out" "PONG ft_irc :solo-lf"
send 4 "PING a\r\nPING b\r\nPING c\r\n"; out=$(read_fd 4); count_is "tres comandos en un paquete" "$out" "PONG" 3
send 4 "PRIVMSG x :$(printf 'a%.0s' {1..600})\r\nPING vivo\r\n"; out=$(read_fd 4); has "linea de 600 bytes: el servidor sigue vivo" "$out" "PONG ft_irc :vivo"
send 4 "\r\n\r\n   \r\n:solo\r\nPING tras-vacias\r\n"; out=$(read_fd 4); has "lineas vacias o sin comando se ignoran" "$out" "PONG ft_irc :tras-vacias"
send 4 "PING    muchos     espacios\r\n"; out=$(read_fd 4); has "espacios multiples" "$out" "PONG ft_irc :muchos"
close_fd 4
open_fd 4; send 4 "PASS pa"; close_fd 4                      # linea sin terminar y cierre brusco
open_fd 4; send 4 "PASS $PASS\r\nNICK l${ID}\r\n"; close_fd 4  # cierre en mitad del registro
open_fd 4; register 4 "m${ID}"; send 4 "PING sigo\r\n"; out=$(read_fd 4)
has "tras esas desconexiones el servidor atiende a clientes nuevos" "$out" "PONG ft_irc :sigo"; close_fd 4

close_fd 3
echo; echo "================================"
echo " Correctas: $PASSED   Fallidas: $FAILED"
echo "================================"
[ "$FAILED" -eq 0 ]