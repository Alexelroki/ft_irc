
# Conexiones con el resto del proyecto (A)

## limpieza de clientes en canales

Vuestro plan dice que Server localiza los canales de un cliente con hasMember(client). Eso sirve para saber a quién avisar con QUIT, pero no para limpiar: Beto no es miembro de #42, así que hasMember daría false y #42 se saltaría, dejando el puntero colgante en _guests. Para limpiar, Server tiene que llamar a removeClient en todos los canales, sin filtrar. Apúntatelo para comentarlo con A

Server debe llamar a removeClient en todos los canales, sin filtrar con hasMember, y antes del delete Client. Si falla alguna de las dos cosas, aparece un puntero colgante.

## crear un canal
A: al crear un canal, el new Channel(name) lo hace Server, y hay que acordar con él qué ocurre si algo lanza una excepción a mitad.

## Reparto del mensaje
Conexión con A y B: Channel ahora llama a queue() de Client dentro del reparto. Así que Channel.cpp necesita #include "Client.hpp", tal como dijiste. Y habrá que acordar con B qué hace queue() cuando el outbuf pasa del tope: si solo marca al cliente para cierre, el bucle de reparto es seguro. Si borrara al cliente en ese momento, tendrías un problema justo en mitad del recorrido.

# Conexiones con el resto del proyecto (B)

## Envio de mensaje
Para entregar un mensaje es queue(), el que vimos antes. No tienes Client.hpp todavía, así que su firma exacta (si recibe un std::string, y por referencia o por valor) hay que confirmarla con B

## metodo getNick de Client
confirmar que getNick() existe en Client con ese nombre y devuelve std::string