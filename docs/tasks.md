# Metodos que necesito de Server (A)

Server.hpp incluye Channel.hpp, y nunca al revés.

*Nombre del servidor*
std::string getName() const;

*Encontrar un canal*
Channel *findChannel(const std::string &name);

NULL si no existe. Normaliza el nombre por dentro, así que el handler pasa el texto tal cual

*Encontrar un cliente*
Client *findClient(const std::string &name);

NULL si no existe, y normaliza el nombre por dentro.

*Insertar un canal en la estructura*
void insertMap(Channel *channel);
suponiendo que es un std::map<string, Channel*>

Recibe el canal ya completo. Saca la clave de getName() y la normaliza. La propiedad pasa a Server cuando la inserción termina bien.

*Solicitud a server para borrar un canal*
void requestDeleteChannel(std::string name);

El nombre va por valor. Server quita la entrada del map antes del delete. Es lo último que hago con el canal.

*Nombre de canal o cliente valido*
bool isValidName(std::string &) const;

# Metodos que necesito de Client (B)

Client.hpp	Que no incluya Channel.hpp

*Nombre del cliente*
std::string getNick() const;

*Cola de mensajes del cliente*
void queue (const std::string &message);

Solo añade al outbuf. Yo le paso la línea completa con \r\n

*Prefijo del cliente*
std::string prefix() const;
Ejemplo: dario!user@host

¿Incluye el : inicial? Mi código escribe ":" + client.prefix()

*Clase Message* y *Parametros de message*
class Message {}

params es std::vector<std::string>, el trailing es el último param sin :, y command va en mayúsculas. ¿Se declara con struct o con class?

*PRIVMSG a un nick*
Quién distingue canal de usuario por el primer carácter. Hoy mi cmdPrivmsg solo cubre canales.

*Dispatcher*
Llama a void cmdX(Server &, Client &, const Message &) para JOIN, PART, PRIVMSG, KICK, INVITE, TOPIC y MODE, e incluye cmdChannel.hpp. El 421 y el 451 van centralizados, y el 461 lo hace cada handler.


# Conexiones con el resto del proyecto (A)

## nombre del servidor
Necesito un getName que devuelva un std::string con el nombre del servidor

## limpieza de clientes en canales
Vuestro plan dice que Server localiza los canales de un cliente con hasMember(client). Eso sirve para saber a quién avisar con QUIT, pero no para limpiar: Beto no es miembro de #42, así que hasMember daría false y #42 se saltaría, dejando el puntero colgante en _guests. Para limpiar, Server tiene que llamar a removeClient en todos los canales, sin filtrar. Apúntatelo para comentarlo con A

Server debe llamar a removeClient en todos los canales, sin filtrar con hasMember, y antes del delete Client. Si falla alguna de las dos cosas, aparece un puntero colgante.

## crear un canal
A: al crear un canal, el new Channel(name) lo hace Server, y hay que acordar con él qué ocurre si algo lanza una excepción a mitad.

## Reparto del mensaje
Conexión con A y B: Channel ahora llama a queue() de Client dentro del reparto. Así que Channel.cpp necesita #include "Client.hpp", tal como dijiste. Y habrá que acordar con B qué hace queue() cuando el outbuf pasa del tope: si solo marca al cliente para cierre, el bucle de reparto es seguro. Si borrara al cliente en ese momento, tendrías un problema justo en mitad del recorrido.

## Crear canal
Una nota de equipo: el orden B exige que el canal exista antes de estar en el map. Según tu contexto, el new Channel lo hace Server, así que hay que acordar con A cómo se hace. O el handler lo crea y le pide a Server que lo registre cuando está completo, o Server ofrece un método que crea y registra de golpe. Apúntalo en las decisiones abiertas con A.

## Normalizacion nombre canal y nick (Server)
La normalización vive en un único sitio y la usan la búsqueda y el registro de Server, así que un handler no puede olvidarla y un canal no puede registrarse con una clave que luego no se encuentre

Los nicks usan la misma regla, así que no es una regla de canales. Dónde vive exactamente (función suelta, método de Server...) lo decides con A, y queda en las decisiones abiertas del equipo.

## A revisar
A: los nombres de findChannel e insertMap, quién normaliza la clave, y el orden B del canal nuevo.

# Conexiones con el resto del proyecto (B)

## Envio de mensaje
Para entregar un mensaje es queue(), el que vimos antes. No tienes Client.hpp todavía, así que su firma exacta (si recibe un std::string, y por referencia o por valor) hay que confirmarla con B

## metodo getNick de Client
confirmar que getNick() existe en Client con ese nombre y devuelve std::string

## Mensaje
Declarado como struct o como clase?
Message. En el contexto aparece como Message { prefix, command, params[] }. Si B lo declaró con struct y tú lo adelantas con class, error

Pendientes que quedan anotados (no hace falta contestarlos hoy):

## A Revisar
B: confirmar que params es std::vector<std::string>, que getNick() y prefix() devuelven std::string, y cómo se declara Message (class o struct).
Equipo: el helper de numéricos y quién pone el \r\n; dónde vive isValidName; el # a secas; y si se permite JOIN #a,#b.

## Helper de numericos (cmdJoin)
vas a repetir ese formato en ocho sitios, y solo cambian el código, el parámetro y el texto. Es el mismo problema que justificó broadcast, y es el helper de numéricos que tienes pendiente con B. De momento no lo diseñes, primero escribe las líneas a mano para ver qué se repite.