#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT 3128	/* El puerto donde se conectar�, servidor */
#define BACKLOG 10	/* Tama�o de la cola de conexiones recibidas */

int	conectar (int, char **);

int	Open_conection (struct sockaddr_in *); /* Funci�n que crea la conexi�n*/

int	Aceptar_pedidos (int, struct sockaddr_in *);	/* Funci�n que acepta una conexi�n entrante*/
